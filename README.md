# SimpleSerializeC

SimpleSerializeC is a C library implementing the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md). SSZ is a serialization format designed for simplicity and efficiency.

## Features

### Supported SSZ Types

The following table lists the SSZ types currently supported by the library for both serialization and deserialization.

| Type        | Serialization Support | Deserialization Support |
|-------------|-----------------------|-------------------------|
| `uint8`     | ✅                    | ✅                      |
| `uint16`    | ✅                    | ✅                      |
| `uint32`    | ✅                    | ✅                      |
| `uint64`    | ✅                    | ✅                      |
| `uint128`   | ✅                    | ✅                      |
| `uint256`   | ✅                    | ✅                      |
| `boolean`   | ✅                    | ✅                      |
| `bitvector` | ✅                    | ✅                      |
| `bitlist`   | ✅                    | ✅                      |
| `vector`    | ✅                    | ✅                      |
| `list`      | ✅                    | ✅                      |

## Merklelization

The library includes functions to compute Merkle roots for SSZ serialized data. For detailed usage, please refer to the API in the header file [`include/ssz_merkle.h`](include/ssz_merkle.h).

## Getting Started

### Prerequisites
- A C compiler (e.g., GCC, Clang) supporting C99 or later.
- `make` (optional, for building tests, benchmarks, and examples).

### External Libraries

- **OpenSSL (version 3.x or later)**: This library is required for SHA256 hashing. Ensure that the OpenSSL development package, including headers and binaries (`-lssl` and `-lcrypto`), is installed on your system. 

- **Snappy**: A compression library required by the project. Ensure that both the library and its development headers are installed. 

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
2. Run a specific benchmark (e.g., `attestation`):
   ```bash
   make bench attestation
   ```

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or additional benchmarks.
