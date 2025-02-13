ifeq ($(OS),Windows_NT)
	override CC = gcc
else
	CC ?= gcc
endif

CFLAGS = -Wall -Wextra -O3 -g
INCLUDE_FLAGS = -Iinclude -Ibench
LDFLAGS =

AR = ar
ARFLAGS = rcs

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
LIB_DIR = lib
BENCH_DIR = bench

LIB_SOURCES = \
	$(SRC_DIR)/ssz_deserialize.c \
	$(SRC_DIR)/ssz_serialize.c \
	$(SRC_DIR)/ssz_utils.c \
	$(SRC_DIR)/ssz_merkle.c \
	$(SRC_DIR)/crypto/mincrypt/sha256.c \
	$(BENCH_DIR)/yaml_parser.c

LIB_OBJECTS = \
	$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, \
		$(filter $(SRC_DIR)/%.c, $(LIB_SOURCES))) \
	$(patsubst $(BENCH_DIR)/%.c, $(OBJ_DIR)/bench/%.o, \
		$(filter $(BENCH_DIR)/%.c, $(LIB_SOURCES)))

STATIC_LIB = $(LIB_DIR)/libssz.a

TEST_SOURCES := $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINARIES := $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/%, $(TEST_SOURCES))

BENCH_SOURCES := $(wildcard $(BENCH_DIR)/bench_ssz_*.c)
BENCH_COMMON_SOURCES = $(BENCH_DIR)/bench.c
BENCH_COMMON_OBJECTS = $(patsubst $(BENCH_DIR)/%.c, $(OBJ_DIR)/bench/%.o, $(BENCH_COMMON_SOURCES))

BENCH_BASENAMES := $(patsubst bench_ssz_%.c, %, $(notdir $(BENCH_SOURCES)))
SUB_BENCHES := $(BENCH_BASENAMES)

ifeq ($(OS),Windows_NT)
	IS_WINDOWS = 1
else
	IS_WINDOWS = 0
endif

ifeq ($(IS_WINDOWS),1)
	SSZ_LDFLAGS = -L$(LIB_DIR) -lssz
else
	SSZ_LDFLAGS = -L$(LIB_DIR) -lssz -lm
endif

SNAPPY_DECODE_SRC = $(TEST_DIR)/snappy_decode.c
SNAPPY_DECODE_OBJ = $(OBJ_DIR)/tests/snappy_decode.o

ifeq ($(IS_WINDOWS),1)
	SHELL = cmd.exe
endif

ifeq ($(IS_WINDOWS),1)
	# Replace forward slashes with backslashes for Windows.
	MKDIR_P = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
else
	MKDIR_P = mkdir -p $1
endif

# Targets
all: $(STATIC_LIB) $(TEST_BINARIES) $(BIN_DIR)/snappy_decode

$(STATIC_LIB): $(LIB_OBJECTS)
	@$(call MKDIR_P,$(LIB_DIR))
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/bench/%.o: $(BENCH_DIR)/%.c
	@$(call MKDIR_P,$(OBJ_DIR)/bench)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/snappy_decode.o: $(SNAPPY_DECODE_SRC)
	@$(call MKDIR_P,$(OBJ_DIR)/tests)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

-include $(OBJ_DIR)/*.d
-include $(OBJ_DIR)/bench/*.d
-include $(OBJ_DIR)/tests/*.d

$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(STATIC_LIB) $(SNAPPY_DECODE_OBJ)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< $(STATIC_LIB) $(SNAPPY_DECODE_OBJ) -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

$(BIN_DIR)/snappy_decode: $(SNAPPY_DECODE_SRC)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< -o $@ $(LDFLAGS)

SINGLE_TEST := $(filter-out test,$(MAKECMDGOALS))

ifeq ($(IS_WINDOWS),1)
test: $(STATIC_LIB)
	@if "%SINGLE_TEST%"=="" ( \
	  echo Building all test binaries... && \
	  $(MAKE) $(TEST_BINARIES) && \
	  echo Running all tests: && \
	  for %%t in ($(TEST_BINARIES)) do ( \
	    echo Running %%t... && \
	    "%%t" \
	  ) \
	) else ( \
	  echo Building test $(SINGLE_TEST)... && \
	  $(MAKE) $(BIN_DIR)\$(SINGLE_TEST) && \
	  echo Running test $(SINGLE_TEST)... && \
	  .\$(BIN_DIR)\$(SINGLE_TEST) \
	)
else
test: $(STATIC_LIB)
	@if [ -z "$(SINGLE_TEST)" ]; then \
	  echo "Building all test binaries..."; \
	  $(MAKE) $(TEST_BINARIES); \
	  echo "Running all tests:"; \
	  for testbin in $(TEST_BINARIES); do \
	    echo "Running $$testbin..."; \
	    $$testbin; \
	  done; \
	else \
	  echo "Building test $(SINGLE_TEST)..."; \
	  $(MAKE) $(BIN_DIR)/$(SINGLE_TEST); \
	  echo "Running test $(SINGLE_TEST)..."; \
	  ./$(BIN_DIR)/$(SINGLE_TEST); \
	fi
endif

ifeq ($(IS_WINDOWS),1)
$(BIN_DIR)/bench_ssz_%: $(BENCH_DIR)/bench_ssz_%.c $(STATIC_LIB) $(BENCH_COMMON_OBJECTS)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< $(BENCH_COMMON_OBJECTS) -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)
	
bench: $(STATIC_LIB) $(BENCH_COMMON_OBJECTS)
	@set second=$(word 2, $(MAKECMDGOALS)) && \
	if "%second%"=="" ( \
	  echo No <type> provided, running ALL benchmarks... & \
	  for %%b in ($(SUB_BENCHES)) do ( \
	    echo Building bench_ssz_%%b... & \
	    $(MAKE) --no-print-directory $(BIN_DIR)/bench_ssz_%%b & \
	    echo Running bench_ssz_%%b... & \
	    .\$(BIN_DIR)\bench_ssz_%%b \
	  ) \
	) else ( \
	  echo Building only bench_ssz_%second%... & \
	  $(MAKE) --no-print-directory $(BIN_DIR)/bench_ssz_%second% & \
	  echo Running bench_ssz_%second%... & \
	  .\$(BIN_DIR)\bench_ssz_%second% \
	)
else
$(BIN_DIR)/bench_ssz_%: $(BENCH_DIR)/bench_ssz_%.c $(STATIC_LIB) $(BENCH_COMMON_OBJECTS)
	@$(call MKDIR_P,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< $(BENCH_COMMON_OBJECTS) -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

bench: $(STATIC_LIB) $(BENCH_COMMON_OBJECTS)
	@ second="$(word 2, $(MAKECMDGOALS))"; \
	if [ -z "$$second" ]; then \
	  echo "No <type> provided, running ALL benchmarks..."; \
	  for b in $(SUB_BENCHES); do \
	    echo "Building bench_ssz_$$b..."; \
	    $(MAKE) --no-print-directory $(BIN_DIR)/bench_ssz_$$b; \
	    echo "Running bench_ssz_$$b..."; \
	    ./$(BIN_DIR)/bench_ssz_$$b; \
	  done; \
	elif echo "$(SUB_BENCHES)" | grep -qw "$$second"; then \
	  echo "Building bench_ssz_$$second..."; \
	  $(MAKE) --no-print-directory $(BIN_DIR)/bench_ssz_$$second; \
	  echo "Running bench_ssz_$$second..."; \
	  ./$(BIN_DIR)/bench_ssz_$$second; \
	else \
	  echo "Argument '$$second' not recognized, running ALL benchmarks..."; \
	  for b in $(SUB_BENCHES); do \
	    echo "Building bench_ssz_$$b..."; \
	    $(MAKE) --no-print-directory $(BIN_DIR)/bench_ssz_$$b; \
	    echo "Running bench_ssz_$$b..."; \
	    ./$(BIN_DIR)/bench_ssz_$$b; \
	  done; \
	fi
endif

test_%:
	@:

.PHONY: all test clean run-tests bench

ifeq ($(IS_WINDOWS),1)
clean:
	@echo Removing object files from $(OBJ_DIR)
	@if exist $(OBJ_DIR) (for /R $(OBJ_DIR) %%F in (*) do del "%%F")
	@echo Removing binaries from $(BIN_DIR)
	@if exist $(BIN_DIR) (for /R $(BIN_DIR) %%F in (*) do del "%%F")
	@echo Removing library files from $(LIB_DIR)
	@if exist $(LIB_DIR) (for /R $(LIB_DIR) %%F in (*) do del "%%F")
	@echo Removing .dSYM directories from $(BIN_DIR)
	@for /d %%D in ($(BIN_DIR)\*.dSYM) do rd /s /q "%%D"
else
clean:
	@echo "Removing object files from $(OBJ_DIR)"
	@find $(OBJ_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing binaries from $(BIN_DIR)"
	@find $(BIN_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing library files from $(LIB_DIR)"
	@find $(LIB_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing .dSYM directories from $(BIN_DIR)"
	@find $(BIN_DIR) -type d -name '*.dSYM' -exec rm -rf {} +
endif