# c-ssz developer Makefile
# Wraps cmake commands for convenience

BUILD_DIR    := build
COVERAGE_DIR := build/coverage

.PHONY: test test-asan test-tsan test-coverage fixtures bench \
        docker-32bit docker-gcc docker-msan docker-valgrind docker-abi-check \
        static-analysis frama-c-eva cbmc codeql duvet fuzz \
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
	fi

bench:
	cmake -S . -B $(BUILD_DIR) -DSSZ_BUILD_BENCH=ON
	cmake --build $(BUILD_DIR) --parallel

# ── Docker builds ────────────────────────────────────────────────

docker-32bit:
	docker build -f docker/Dockerfile.32bit -t c-ssz-32bit .
	docker run --rm c-ssz-32bit

docker-gcc:
	docker build -f docker/Dockerfile.32bit -t c-ssz-gcc .
	docker run --rm c-ssz-gcc

# ── Cleanup ──────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR)

# ── Aggregate targets ────────────────────────────────────────────

check: test test-asan
	@echo "All native checks passed."

check-all: check docker-32bit
	@echo "All checks (native + docker) passed."
