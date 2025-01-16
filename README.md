# SimpleSerializeC

SimpleSerializeC is a C library implementing the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md). [SSZ](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md) is a serialization format designed for simplicity, efficiency, and compatibility with Merkleization.

## Features

- **Serialization**: Serialize fixed-size and variable-size data types, including integers, booleans, vectors, and lists.
- **Deserialization**: Deserialize SSZ-encoded data back into native C structures.
- **Utility Functions**: Helper functions for bit manipulation, offset handling, and power-of-two calculations.

> **Note**: Support for Merkleization is not yet implemented but will be added in a future release. Additionally, benchmarks to evaluate performance will also be introduced in upcoming updates.

## Getting Started

### Prerequisites
- A C compiler (e.g., GCC, Clang) supporting C99 or later.
- `make` (optional, for building tests and examples).

### Building the Library
The project includes a `Makefile` to simplify the build process. The `Makefile` provides the following targets:

- **`all`**: Builds the static library (`libssz.a`) and all test binaries.
- **`$(LIB_DIR)/libssz.a`**: Compiles the source files in `src/` and archives them into a static library.
- **`$(BIN_DIR)/<test_binary>`**: Compiles individual test files in `tests/` and links them with the static library.
- **`test`**: Builds everything and runs all test binaries.
- **`clean`**: Removes all build artifacts, including object files, binaries, and the static library.

### Build Instructions
1. Clone the repository and navigate to the project directory.
2. Run `make` to build the library and test binaries.
3. Run `make test` to execute the test suite and verify functionality.

### Example Commands
- Build the library and tests:
  ```bash
  make
  ```
- Run the test suite:
  ```bash
  make test
  ```
- Clean up build artifacts:
  ```bash
  make clean
  ```

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.
