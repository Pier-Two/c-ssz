# SimpleSerializeC

SimpleSerializeC is a C library implementing the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md). [SSZ](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md) is a serialization format designed for simplicity and efficiency.

## Features

### Supported SSZ Types

The following table lists the SSZ types currently supported by the library for both serialization and deserialization.

| Type                  | Serialization Support | Deserialization Support |
|-----------------------|-----------------------|--------------------------|
| `uint8`              | ✅                    | ✅                       |
| `uint16`             | ✅                    | ✅                       |
| `uint32`             | ✅                    | ✅                       |
| `uint64`             | ✅                    | ✅                       |
| `uint128`            | ✅                    | ✅                       |
| `uint256`            | ✅                    | ✅                       |
| `boolean`            | ✅                    | ✅                       |
| `bitvector`          | ✅                    | ✅                       |
| `bitlist`            | ✅                    | ✅                       |
| `vector`             | ✅                    | ✅                       |
| `list`               | ✅                    | ✅                       |

> **Note**: Support for Merkleization is not yet implemented but will be added in the coming weeks.

## Getting Started

### Prerequisites
- A C compiler (e.g., GCC, Clang) supporting C99 or later.
- `make` (optional, for building tests, benchmarks, and examples).

### Building the Library
The project includes a `Makefile` to simplify the build process. The `Makefile` provides the following targets:

- **`all`**: Builds the static library (`libssz.a`), test binaries, and benchmark binaries.
- **`test`**: Builds everything and runs all test binaries.
- **`bench`**: Builds and runs all benchmark binaries.
- **`clean`**: Removes all build artifacts, including object files, binaries, and the static library.

### Running Benchmarks
The library includes benchmarks to evaluate the performance of SSZ serialization and deserialization. 

To run benchmarks:
1. Run all benchmarks:
   ```bash
   make bench
   ```
3. Run a specific benchmark (e.g., `attestation`):
   ```bash
   make bench attestation
   ```

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or additional benchmarks.
