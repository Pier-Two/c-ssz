CC ?= gcc
CFLAGS = -Wall -Wextra -O3 -g
INCLUDE_FLAGS = -Iinclude -Ibench
OPENSSL_CFLAGS = -I/opt/homebrew/Cellar/openssl@3/3.4.0/include
OPENSSL_LIBS = -L/opt/homebrew/Cellar/openssl@3/3.4.0/lib -lssl -lcrypto
SNAPPY_LIBS = -L/opt/homebrew/Cellar/snappy/1.2.1/lib -lsnappy
CFLAGS += $(OPENSSL_CFLAGS)
override LDFLAGS := $(OPENSSL_LIBS) $(SNAPPY_LIBS)
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

SSZ_LDFLAGS = -L$(LIB_DIR) -lssz

all: $(STATIC_LIB) $(TEST_BINARIES)

$(STATIC_LIB): $(LIB_OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/bench/%.o: $(BENCH_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/bench
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -MMD -MP -c $< -o $@

-include $(OBJ_DIR)/*.d
-include $(OBJ_DIR)/bench/*.d

$(BIN_DIR)/%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

SINGLE_TEST := $(filter-out test,$(MAKECMDGOALS))

.PHONY: all test clean run-tests bench

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

clean:
	@echo "Removing object files from $(OBJ_DIR)"
	@find $(OBJ_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing binaries from $(BIN_DIR)"
	@find $(BIN_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing library files from $(LIB_DIR)"
	@find $(LIB_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	@echo "Removing .dSYM directories from $(BIN_DIR)"
	@find $(BIN_DIR) -type d -name '*.dSYM' -exec rm -rf {} +

run-tests: test

$(BIN_DIR)/bench_ssz_%: $(BENCH_DIR)/bench_ssz_%.c $(STATIC_LIB) $(BENCH_COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $< $(BENCH_COMMON_OBJECTS) -o $@ $(LDFLAGS) $(SSZ_LDFLAGS)

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

test_%:
	@: