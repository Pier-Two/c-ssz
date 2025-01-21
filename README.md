# SimpleSerializeC

SimpleSerializeC is a C library implementing the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md). [SSZ](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md) is a serialization format designed for simplicity, efficiency, and compatibility with Merkleization.

## Features

- **Serialization**: Serialize fixed-size and variable-size data types, including integers, booleans, vectors, and lists.
- **Deserialization**: Deserialize SSZ-encoded data back into native C structures.
- **Utility Functions**: Helper functions for bit manipulation, offset handling, and power-of-two calculations.
- **Benchmarks**: Measure the performance of serialization and deserialization for various SSZ data types.

> **Note**: Support for Merkleization is not yet implemented but will be added in a future release.

## Getting Started

### Prerequisites
- A C compiler (e.g., GCC, Clang) supporting C99 or later.
- `make` (optional, for building tests, benchmarks, and examples).

### Building the Library
The project includes a `Makefile` to simplify the build process. The `Makefile` provides the following targets:

- **`all`**: Builds the static library (`libssz.a`), test binaries, and benchmark binaries.
- **`$(LIB_DIR)/libssz.a`**: Compiles the source files in `src/` and archives them into a static library.
- **`$(BIN_DIR)/<test_binary>`**: Compiles individual test files in `tests/` and links them with the static library.
- **`$(BIN_DIR)/bench_ssz_<type>`**: Compiles individual benchmark files in `bench/` and links them with the static library.
- **`test`**: Builds everything and runs all test binaries.
- **`bench`**: Builds and runs all benchmark binaries.
- **`clean`**: Removes all build artifacts, including object files, binaries, and the static library.

### Running Benchmarks
The library includes benchmarks to evaluate the performance of SSZ serialization and deserialization. Benchmarks are implemented in the following files:
- `bench/bench_ssz_serialize.c`: Benchmarks for serialization of various SSZ data types.
- `bench/bench_ssz_deserialize.c`: Benchmarks for deserialization of various SSZ data types.
- `bench/bench_ssz_attestation.c`: Benchmarks for serialization and deserialization of the `Attestation` container.

To run benchmarks:
1. Build the benchmark binaries:
   ```bash
   make bench
   ```
2. Run all benchmarks:
   ```bash
   make bench
   ```
3. Run a specific benchmark (e.g., `attestation`):
   ```bash
   make bench attestation
   ```

### Build Instructions
1. Clone the repository and navigate to the project directory.
2. Run `make` to build the library, test binaries, and benchmark binaries.
3. Run `make test` to execute the test suite and verify functionality.
4. Run `make bench` to execute all benchmarks and evaluate performance.

### Example Commands
- Build the library, tests, and benchmarks:
  ```bash
  make
  ```
- Run the test suite:
  ```bash
  make test
  ```
- Run all benchmarks:
  ```bash
  make bench
  ```
- Run a specific benchmark:
  ```bash
  make bench <type>
  ```
- Clean up build artifacts:
  ```bash
  make clean
  ```

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or additional benchmarks.
