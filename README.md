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
- A C compiler (e.g., GCC, Clang) supporting C99 or later. (Note: The library is written in C99 and should compile on any platform with a C99 compiler; it has been tested on MacOS, Linux, and Windows.)
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
2. Run a specific benchmark (e.g., `attestation`):
   ```bash
   make bench attestation
   ```

## Attributions

This project incorporates a SHA-256 implementation sourced from [mincrypt](https://android.googlesource.com/platform/system/core/+/669ecc2f5e80ff924fa20ce7445354a7c5bcfd98/libmincrypt), which is originally part of the Android Open Source Project. The mincrypt code is licensed under a BSD 3-Clause License by Google Inc. 

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or additional benchmarks.
