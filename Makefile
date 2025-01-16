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

# Source files for the library
LIB_SOURCES = ssz_deserialization.c ssz_serialization.c ssz_utils.c ssz_merkleization.c

# Convert each .c in LIB_SOURCES to the corresponding .o in obj/
LIB_OBJECTS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(LIB_SOURCES))

# Test files. 
TEST_SOURCES = test_ssz_serialization.c test_ssz_deserialization.c

# Convert each test .c file to a final binary in bin/
TEST_BINARIES = $(patsubst %.c, $(BIN_DIR)/%, $(TEST_SOURCES))

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

# Rules to build each test binary
$(BIN_DIR)/%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lssz

# Test target that builds everything and runs the tests
test: all
	@echo "Running test_ssz_serialization..."
	@./$(BIN_DIR)/test_ssz_serialization
	@echo "Running test_ssz_deserialization..."
	@./$(BIN_DIR)/test_ssz_deserialization

# Clean up everything
clean:
	find $(OBJ_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	find $(BIN_DIR) -type f ! -name '.emptydir' -exec rm -f {} +
	find $(LIB_DIR) -type f ! -name '.emptydir' -exec rm -f {} +

# Provide a helpful alias if someone tries to type 'make run-tests'
run-tests: test

# Phony targets
.PHONY: all test clean run-tests