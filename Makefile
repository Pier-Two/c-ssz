# Set the compiler. You can override this via environment variable if you like, e.g. CC=clang make
CC ?= gcc

# Compiler flags: warnings, optimization, debug, etc. Feel free to adjust as needed.
CFLAGS = -Wall -Wextra -O3 -g -Iinclude

# Archive tool for building static libraries
AR = ar
ARFLAGS = rcs

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
LIB_DIR = lib

# Criterion flags. Adjust if your Criterion is installed in a different prefix.
CRITERION_CFLAGS ?= -I/opt/homebrew/opt/criterion/include
CRITERION_LDFLAGS ?= -L/opt/homebrew/opt/criterion/lib -lcriterion

# Source files for the library
LIB_SOURCES = ssz_deserialize.c ssz_serialize.c ssz_utils.c ssz_merkle.c

# Convert each .c in LIB_SOURCES to the corresponding .o in obj/
LIB_OBJECTS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(LIB_SOURCES))

# Test files 
TEST_SOURCES = test_ssz_serialize.c test_ssz_deserialize.c
# Convert each test .c file to a final binary in bin/
TEST_BINARIES = $(patsubst %.c, $(BIN_DIR)/%, $(TEST_SOURCES))

# Criterion-based test files
CRITERION_TEST_SOURCES = test_ssz_serialize_criterion.c test_ssz_deserialize_criterion.c
CRITERION_TEST_BINARIES = $(patsubst %.c, $(BIN_DIR)/criterion_%, $(CRITERION_TEST_SOURCES))

# The final static library we'll produce
STATIC_LIB = $(LIB_DIR)/libssz.a

# Default target
all: $(STATIC_LIB) $(TEST_BINARIES)

# Rule to build the static library
$(STATIC_LIB): $(LIB_OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

# Pattern rule to compile any .c file in src to an .o in obj
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rules to build each legacy test binary
$(BIN_DIR)/%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lssz

# Rules to build each Criterion-based test binary
$(BIN_DIR)/criterion_%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(CRITERION_CFLAGS) $< -o $@ -L$(LIB_DIR) -lssz $(CRITERION_LDFLAGS)

# Test target that builds everything and runs the legacy tests
test: all
	@echo "Running test_ssz_serialize..."
	@./$(BIN_DIR)/test_ssz_serialize
	@echo "Running test_ssz_deserialize..."
	@./$(BIN_DIR)/test_ssz_deserialize

# Criterion test target that builds and runs the Criterion test binaries
criterion-tests: all $(CRITERION_TEST_BINARIES)
	@echo "Running Criterion tests..."
	@for testbin in $(CRITERION_TEST_BINARIES); do \
		echo "Running $$testbin..."; \
		./$$testbin; \
	done

# Clean up everything
clean:
	find $(OBJ_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	find $(BIN_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	find $(LIB_DIR) -type f ! -name '.emptydir' -exec rm -f {} +

# Provide a helpful alias if someone tries to type 'make run-tests'
run-tests: test

# Phony targets
.PHONY: all test clean run-tests criterion-tests