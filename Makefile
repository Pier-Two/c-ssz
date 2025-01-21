CC ?= gcc
CFLAGS = -Wall -Wextra -O3 -g
INCLUDE_FLAGS = -Iinclude
LDFLAGS =
AR = ar
ARFLAGS = rcs

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
LIB_DIR = lib
BENCH_DIR = bench

CRITERION_CFLAGS ?= -I/opt/homebrew/opt/criterion/include
CRITERION_LDFLAGS ?= -L/opt/homebrew/opt/criterion/lib -lcriterion

LIB_SOURCES = ssz_deserialize.c ssz_serialize.c ssz_utils.c ssz_merkle.c
LIB_OBJECTS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(LIB_SOURCES))

STATIC_LIB = $(LIB_DIR)/libssz.a

TEST_SOURCES = test_ssz_serialize.c test_ssz_deserialize.c
TEST_BINARIES = $(patsubst %.c, $(BIN_DIR)/%, $(TEST_SOURCES))

CRITERION_TEST_SOURCES = test_ssz_serialize_criterion.c test_ssz_deserialize_criterion.c
CRITERION_TEST_BINARIES = $(patsubst %.c, $(BIN_DIR)/criterion_%, $(CRITERION_TEST_SOURCES))

BENCH_SOURCES := $(wildcard $(BENCH_DIR)/bench_ssz_*.c)

BENCH_COMMON_SOURCES = $(BENCH_DIR)/benchmark.c
BENCH_COMMON_OBJECTS = $(patsubst $(BENCH_DIR)/%.c, $(OBJ_DIR)/bench/%.o, $(BENCH_COMMON_SOURCES))

BENCH_EXTRA_SOURCES = yaml_parser.c
BENCH_EXTRA_OBJECTS = $(patsubst %.c, $(OBJ_DIR)/bench/%.o, $(BENCH_EXTRA_SOURCES))

BENCH_BASENAMES := $(patsubst bench_ssz_%.c,%, $(notdir $(BENCH_SOURCES)))
SUB_BENCHES := $(BENCH_BASENAMES)

SSZ_LDFLAGS = -L$(LIB_DIR) -lssz

all: $(STATIC_LIB) $(TEST_BINARIES)

$(STATIC_LIB): $(LIB_OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

-include $(OBJ_DIR)/*.d

$(BIN_DIR)/%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

$(BIN_DIR)/criterion_%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $(CRITERION_CFLAGS) $< -o $@ $(LDFLAGS) $(SSZ_LDFLAGS) $(CRITERION_LDFLAGS)

test: all
	@echo "Running test_ssz_serialize..."
	@./$(BIN_DIR)/test_ssz_serialize
	@echo "Running test_ssz_deserialize..."
	@./$(BIN_DIR)/test_ssz_deserialize

criterion-tests: all $(CRITERION_TEST_BINARIES)
	@echo "Running Criterion tests..."
	@for testbin in $(CRITERION_TEST_BINARIES); do \
		echo "Running $$testbin..."; \
		./$$testbin; \
	done

clean:
	@echo "Removing object files from $(OBJ_DIR)"
	@find $(OBJ_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing binaries from $(BIN_DIR)"
	@find $(BIN_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing library files from $(LIB_DIR)"
	@find $(LIB_DIR) -type f ! -name '.emptydir' -exec rm -f {} +

run-tests: test

$(OBJ_DIR)/bench/%.o: $(BENCH_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/bench
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

-include $(OBJ_DIR)/bench/*.d

$(BIN_DIR)/bench_ssz_%: $(BENCH_DIR)/bench_ssz_%.c $(STATIC_LIB) $(BENCH_COMMON_OBJECTS) $(BENCH_EXTRA_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< $(BENCH_COMMON_OBJECTS) $(BENCH_EXTRA_OBJECTS) -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

bench: all
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
	  echo "Building only bench_ssz_$$second..."; \
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

$(SUB_BENCHES):
	@:

.PHONY: all test clean run-tests criterion-tests bench $(SUB_BENCHES)