# c-ssz developer Makefile
# Wraps cmake commands for convenience

BUILD_DIR    := build
COVERAGE_DIR := build/coverage
FUZZ_COVERAGE_DIR := build/fuzz-coverage

CLANG         ?= clang
CLANG_TIDY    ?= $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)
LLVM_PROFDATA ?= llvm-profdata
LLVM_COV      ?= llvm-cov

SSZ_FUZZ_SOURCES       := $(wildcard fuzz/fuzz_*.c)
SSZ_LIBRARY_SOURCES    := $(wildcard src/*.c)
SSZ_FUZZ_INCLUDE_FLAGS := -Iinclude -Iexternal/aws-lc/include
SSZ_FUZZ_BUILD_FLAGS   := -std=c99 -Wall -Wextra -O1 -g
SSZ_FUZZ_SAN_FLAGS     := -fsanitize=fuzzer,address
SSZ_FUZZ_COV_FLAGS     := -fprofile-instr-generate -fcoverage-mapping
# fuzz-coverage compiles directly without CMake; AWS-LC libcrypto.a must be pre-built.
SSZ_FUZZ_AWSLC_CRYPTO_LIB ?= external/aws-lc/build/crypto/libcrypto.a
SSZ_FUZZ_PLATFORM_DEFS :=

ifeq ($(shell uname -s),Linux)
SSZ_FUZZ_PLATFORM_DEFS += -D_DEFAULT_SOURCE
endif

.PHONY: test test-asan test-tsan test-coverage fixtures bench \
        docker-32bit docker-gcc docker-msan docker-valgrind docker-abi-check \
        static-analysis frama-c-eva cbmc codeql duvet fuzz fuzz-coverage \
        clean

# ── Native builds ────────────────────────────────────────────────

test:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR) --parallel
	ctest --test-dir $(BUILD_DIR) --output-on-failure

test-asan:
	cmake -S . -B $(BUILD_DIR) -DSSZ_SANITIZERS=address,undefined
	cmake --build $(BUILD_DIR) --parallel
	ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=0:print_stacktrace=1 \
	  ctest --test-dir $(BUILD_DIR) --output-on-failure

test-tsan:
	cmake -S . -B $(BUILD_DIR) -DSSZ_SANITIZERS=thread
	cmake --build $(BUILD_DIR) --parallel
	ctest --test-dir $(BUILD_DIR) --output-on-failure

test-coverage:
	cmake -S . -B $(COVERAGE_DIR) -DCMAKE_BUILD_TYPE=Debug \
	  -DCMAKE_C_FLAGS='--coverage' -DCMAKE_EXE_LINKER_FLAGS='--coverage'
	cmake --build $(COVERAGE_DIR) --parallel
	ctest --test-dir $(COVERAGE_DIR) --output-on-failure
	lcov --capture --directory $(COVERAGE_DIR) \
	  --output-file $(COVERAGE_DIR)/coverage.info --ignore-errors mismatch
	lcov --remove $(COVERAGE_DIR)/coverage.info \
	  '/usr/*' '*/tests/*' '*/bench/*' '*/lib/*' \
	  --output-file $(COVERAGE_DIR)/coverage-filtered.info --ignore-errors unused
	genhtml $(COVERAGE_DIR)/coverage-filtered.info \
	  --output-directory $(COVERAGE_DIR)/html
	lcov --summary $(COVERAGE_DIR)/coverage-filtered.info

fixtures:
	@set -e; \
	if command -v uv >/dev/null 2>&1; then \
	  echo "Generating consensus-spec fixtures with uv"; \
	  cd external/consensus-specs; \
	  uv sync --all-extras; \
	  uv run python -m pysetup.generate_specs --all-forks; \
	  uv run --extra generator --no-editable --reinstall-package=eth-consensus-specs \
	    python -m tests.generators.main --output ../../tests/fixtures --runners ssz_generic; \
	  uv run --extra generator --no-editable --reinstall-package=eth-consensus-specs \
	    python -m tests.generators.main --output ../../tests/fixtures --runners ssz_static; \
	  uv run python ../../tests/spec/generate_ssz_static_schema.py \
	    --fixtures-root ../../tests/fixtures \
	    --pyspec-root tests/core/pyspec \
	    --output-header ../../tests/spec/generated/ssz_static_schema_generated.h \
	    --output-source ../../tests/spec/generated/ssz_static_schema_generated.c; \
	else \
	  echo "uv not found; using python3 + pip fallback"; \
	  cd external/consensus-specs; \
	  python3 -m venv .venv-spec; \
	  . .venv-spec/bin/activate; \
	  python -m pip install --upgrade pip; \
	  python -m pip install -e .[generator]; \
	  python -m pysetup.generate_specs --all-forks; \
	  python -m tests.generators.main --output ../../tests/fixtures --runners ssz_generic; \
	  python -m tests.generators.main --output ../../tests/fixtures --runners ssz_static; \
	  python ../../tests/spec/generate_ssz_static_schema.py \
	    --fixtures-root ../../tests/fixtures \
	    --pyspec-root tests/core/pyspec \
	    --output-header ../../tests/spec/generated/ssz_static_schema_generated.h \
	    --output-source ../../tests/spec/generated/ssz_static_schema_generated.c; \
	fi

bench:
	cmake -S . -B $(BUILD_DIR) -DSSZ_BUILD_BENCH=ON
	cmake --build $(BUILD_DIR) --parallel

fuzz:
	CC=clang cmake -S . -B $(BUILD_DIR) -DSSZ_BUILD_FUZZ=ON
	cmake --build $(BUILD_DIR) --parallel
	@set -e; \
	mkdir -p $(BUILD_DIR)/fuzz-artifacts; \
	found=0; \
	for fuzzer in $(BUILD_DIR)/fuzz_*; do \
	  if [ ! -x "$$fuzzer" ]; then \
	    continue; \
	  fi; \
	  found=1; \
	  echo "--- Running $$(basename "$$fuzzer") ---"; \
	  "$$fuzzer" -max_total_time=10 -artifact_prefix=$(BUILD_DIR)/fuzz-artifacts/; \
	done; \
	if [ "$$found" -eq 0 ]; then \
	  echo "No fuzz executables found in $(BUILD_DIR)."; \
	  exit 1; \
	fi

fuzz-coverage:
	@set -e; \
	if [ -z "$(SSZ_FUZZ_SOURCES)" ]; then \
	  echo "No fuzz sources found in fuzz/."; \
	  exit 1; \
	fi; \
	if ! command -v $(CLANG) >/dev/null 2>&1; then \
	  echo "$(CLANG) is required for fuzz-coverage."; \
	  exit 1; \
	fi; \
	if ! command -v $(LLVM_PROFDATA) >/dev/null 2>&1; then \
	  echo "$(LLVM_PROFDATA) is required for fuzz-coverage."; \
	  exit 1; \
	fi; \
	if ! command -v $(LLVM_COV) >/dev/null 2>&1; then \
	  echo "$(LLVM_COV) is required for fuzz-coverage."; \
	  exit 1; \
	fi; \
	if [ ! -f "$(SSZ_FUZZ_AWSLC_CRYPTO_LIB)" ]; then \
	  echo "Pre-built AWS-LC libcrypto.a not found at $(SSZ_FUZZ_AWSLC_CRYPTO_LIB)."; \
	  exit 1; \
	fi; \
	rm -rf $(FUZZ_COVERAGE_DIR); \
	mkdir -p $(FUZZ_COVERAGE_DIR)/bin $(FUZZ_COVERAGE_DIR)/profiles \
	  $(FUZZ_COVERAGE_DIR)/artifacts $(FUZZ_COVERAGE_DIR)/html; \
	for harness in $(SSZ_FUZZ_SOURCES); do \
	  name=$$(basename "$$harness" .c); \
	  echo "--- Building $$name ---"; \
	  $(CLANG) $(SSZ_FUZZ_BUILD_FLAGS) $(SSZ_FUZZ_PLATFORM_DEFS) \
	    $(SSZ_FUZZ_SAN_FLAGS) $(SSZ_FUZZ_COV_FLAGS) \
	    $(SSZ_FUZZ_INCLUDE_FLAGS) \
	    "$$harness" $(SSZ_LIBRARY_SOURCES) \
	    "$(SSZ_FUZZ_AWSLC_CRYPTO_LIB)" -lm -o $(FUZZ_COVERAGE_DIR)/bin/$$name; \
	done; \
	found=0; \
	for fuzzer in $(FUZZ_COVERAGE_DIR)/bin/fuzz_*; do \
	  if [ ! -x "$$fuzzer" ]; then \
	    continue; \
	  fi; \
	  found=1; \
	  name=$$(basename "$$fuzzer"); \
	  echo "--- Running $$name ---"; \
	  LLVM_PROFILE_FILE="$(FUZZ_COVERAGE_DIR)/profiles/$${name}.profraw" \
	    "$$fuzzer" -max_total_time=10 -artifact_prefix=$(FUZZ_COVERAGE_DIR)/artifacts/; \
	done; \
	if [ "$$found" -eq 0 ]; then \
	  echo "No fuzz executables found in $(FUZZ_COVERAGE_DIR)/bin."; \
	  exit 1; \
	fi; \
	$(LLVM_PROFDATA) merge -sparse $(FUZZ_COVERAGE_DIR)/profiles/*.profraw \
	  -o $(FUZZ_COVERAGE_DIR)/fuzz.profdata; \
	set --; \
	for fuzzer in $(FUZZ_COVERAGE_DIR)/bin/fuzz_*; do \
	  if [ ! -x "$$fuzzer" ]; then \
	    continue; \
	  fi; \
	  if [ "$$#" -eq 0 ]; then \
	    set -- "$$fuzzer"; \
	  else \
	    set -- "$$@" -object "$$fuzzer"; \
	  fi; \
	done; \
	if [ "$$#" -eq 0 ]; then \
	  echo "No fuzz executables available for llvm-cov."; \
	  exit 1; \
	fi; \
	$(LLVM_COV) show "$$@" \
	  -instr-profile=$(FUZZ_COVERAGE_DIR)/fuzz.profdata \
	  --format=html \
	  --output-dir=$(FUZZ_COVERAGE_DIR)/html \
	  src/*.c; \
	$(LLVM_COV) report "$$@" \
	  -instr-profile=$(FUZZ_COVERAGE_DIR)/fuzz.profdata \
	  src/*.c

# ── Static analysis ─────────────────────────────────────────────

static-analysis:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(BUILD_DIR) --parallel
	@echo "--- clang-tidy (informational) ---"
	-@c_files=$$(git ls-files 'src/*.c' 'include/*.h'); \
	if [ -z "$$c_files" ]; then \
	  echo "No C source files found."; \
	else \
	  $(CLANG_TIDY) -p $(BUILD_DIR) $$c_files; \
	fi
	@echo "--- cppcheck (informational) ---"
	-cppcheck \
	  --enable=warning,style,performance,portability \
	  --std=c11 \
	  --project=$(BUILD_DIR)/compile_commands.json \
	  --suppress=missingIncludeSystem \
	  -i external

# ── Docker builds ────────────────────────────────────────────────

docker-32bit:
	docker build --platform linux/amd64 -f docker/Dockerfile.32bit -t c-ssz-32bit .
	docker run --platform linux/amd64 --rm c-ssz-32bit

docker-gcc:
	docker build --platform linux/amd64 -f docker/Dockerfile.gcc -t c-ssz-gcc .
	docker run --platform linux/amd64 --rm c-ssz-gcc

# ── Cleanup ──────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR)

# ── Aggregate targets ────────────────────────────────────────────

check: test test-asan
	@echo "All native checks passed."

check-all: check docker-32bit
	@echo "All checks (native + docker) passed."
