# C-SSZ

C-SSZ is a C library implementing the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md). SSZ is a serialization format designed for simplicity and efficiency.

## Features

### Supported SSZ Types

The following table lists the types currently supported for both serialization and deserialization:

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
| `unions`    | 🧪 (Experimental)     | 🧪 (Experimental)       |

For further details regarding the public API, please refer to the header files [`ssz_serialize.h`](include/ssz_serialize.h) and [`ssz_deserialize.h`](include/ssz_deserialize.h).

### Merklelization

The library includes functions for computing a Merkle root over contiguous chunks of data. For detailed usage, please refer to the API in the header file [`ssz_merkle.h`](include/ssz_merkle.h).

### Performance

The necessary public functions in this library have been benchmarked where the detailed results can be found in this project's [wiki section](https://github.com/Pier-Two/SimpleSerializeC/wiki/Performance). These results can be replicated by using commands listed in the [Running Benchmarks](#running-benchmarks) section. 

## Getting Started

### Prerequisites
- A C compiler (e.g., GCC, Clang) supporting C99 or later. (Note: The library is written in C99 and should compile on any platform with a C99 compiler; it has been tested on MacOS, Linux, and Windows.)
- `make` (for building the library, tests and benchmarks).

### Building the Library
The project includes a `Makefile` to simplify the build process. The `Makefile` provides the following targets:

- **`all`**: Builds the static library (`libssz.a`), test binaries, and benchmark binaries.
- **`test`**: Builds everything and runs all test binaries.
- **`bench`**: Builds and runs all benchmark binaries.
- **`clean`**: Removes all build artifacts, including object files, binaries, and the static library.

### Running Benchmarks
The library includes benchmarks to evaluate the performance of SSZ serialization and deserialization. Here are the commands supported for benchmarking:

1. Run all benchmarks:
```bash
make bench
```

2. Run a specific benchmark (e.g., `serialize`):
```bash
make bench serialize
```

### Running Tests
The library includes test binaries that verify the functionality of SSZ serialization and deserialization. To ensure compliance with the [SSZ specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md), it implements all the [generic test vectors](https://github.com/ethereum/consensus-specs/tree/dev/tests/generators/ssz_generic). Below are the commands supported for testing:

1. Run all tests:
```bash
make test
```

2. Run a specific test (e.g., `serialize`):
```bash
make test serialize
``` 

## Attributions

This project incorporates a SHA-256 implementation sourced from [libmincrypt](https://android.googlesource.com/platform/system/core/+/669ecc2f5e80ff924fa20ce7445354a7c5bcfd98/libmincrypt), which is originally part of the Android Open Source Project. 

## License
This project includes code from an external source, which is covered by its respective license. 

### MIT License 
This project is licensed under the MIT License. See the [`LICENSE-MIT`](LICENSE-MIT) file for details.

### BSD-3-Clause Licensed Code
This project implements the SHA256 implementation from [libmincrypt](https://android.googlesource.com/platform/system/core/+/669ecc2f5e80ff924fa20ce7445354a7c5bcfd98/libmincrypt), which is licensed under the BSD-3-Clause License. See the [`LICENSE-BSD`](LICENSE-BSD) file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements, bug fixes, or additional benchmarks.
