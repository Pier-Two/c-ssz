# c-ssz

[![CI](https://github.com/Pier-Two/c-ssz/actions/workflows/ci.yml/badge.svg)](https://github.com/Pier-Two/c-ssz/actions/workflows/ci.yml)
![Test Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/uink45/78fd2e9ece2119db6cea0321a08e36e7/raw/test-coverage.json)
![Fuzz Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/uink45/78fd2e9ece2119db6cea0321a08e36e7/raw/fuzz-coverage.json)

A C99 implementation of the Simple Serialize (SSZ) [specification](https://github.com/ethereum/consensus-specs/blob/dev/ssz/simple-serialize.md) and [Merkle proof formats](https://github.com/ethereum/consensus-specs/blob/dev/ssz/merkle-proofs.md).

## Features

### Serialization and Deserialization

Full support for all SSZ types defined in the specification:

| Type | Serialization | Deserialization |
|------|:---:|:---:|
| `uint8` / `uint16` / `uint32` / `uint64` / `uint128` / `uint256` | ✅ | ✅ |
| `boolean` | ✅ | ✅ |
| `bitvector` | ✅ | ✅ |
| `bitlist` | ✅ | ✅ |
| `vector` (fixed and variable element) | ✅ | ✅ |
| `list` (fixed and variable element) | ✅ | ✅ |
| `container` | ✅ | ✅ |
| `union` | ✅ | ✅ |
| `compatible union` (EIP-8016) | ✅ | ✅ |

See [`ssz_serialize.h`](include/ssz_serialize.h) and [`ssz_deserialize.h`](include/ssz_deserialize.h).

### Merkleization

Complete hash tree root computation for all SSZ types, including:

- Standard `merkleize` with chunk limits
- Progressive merkleization for `ProgressiveList`, `ProgressiveBitlist`, and `ProgressiveContainer` (EIP-7916, EIP-7495)
- `mix_in_length`, `mix_in_selector`, `mix_in_active_fields`

All merkleization functions use an iterative implementation with caller-provided scratch buffers. See [`ssz_merkle.h`](include/ssz_merkle.h).

### Merkle Proofs

Generalized index computation, single and multi Merkle proof verification:

- `ssz_get_generalized_index` with full path navigation
- `ssz_calculate_merkle_root` / `ssz_calculate_multi_merkle_root`
- `ssz_verify_merkle_proof` / `ssz_verify_merkle_multiproof`
- `ssz_get_branch_indices` / `ssz_get_path_indices` / `ssz_get_helper_indices`

See [`ssz_proof.h`](include/ssz_proof.h).

### Incremental Merkle Cache

A persistent Merkle tree cache that supports incremental updates. When leaves change, only the affected paths from modified leaves to the root are rehashed. The cache uses zero dynamic allocation -- all memory is caller-provided via typed buffers.

- `ssz_merkle_cache_requirements` / `ssz_merkle_cache_bind` / `ssz_merkle_cache_migrate_into`
- Leaf updates, range zeroing, packed byte sync, bitvector/bitlist sync
- Token-based composite change detection with batch root computation
- Bounded and exact tree modes

See [`ssz_merkle_cache.h`](include/ssz_merkle_cache.h).

### SHA-256

Hardware-accelerated SHA-256 via [AWS-LC](https://github.com/aws/aws-lc) with batch hashing support for Merkle tree computation. See [`ssz_hash.h`](include/ssz_hash.h).

### Safety and Compliance

- MISRA C:2012 compliant (checked in CI via cppcheck)
- CERT C compliant (checked in CI via clang-tidy)
- Frama-C Eva static analysis
- CBMC formal verification harnesses
- GCC `-fanalyzer` and CodeQL
- AddressSanitizer, UndefinedBehaviorSanitizer
- libFuzzer harnesses for all modules
- ABI stability checks

## Getting Started

### Prerequisites

- A C compiler supporting C99 (GCC, Clang, MSVC)
- CMake 3.16 or later

### Building

```bash
cmake -S . -B build
cmake --build build
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SSZ_BUILD_TESTS` | `ON` | Build unit and spec tests |
| `SSZ_BUILD_BENCH` | `OFF` | Build micro-benchmarks |
| `SSZ_BUILD_FUZZ` | `OFF` | Build libFuzzer harnesses (requires Clang) |
| `SSZ_USE_SYSTEM_CRYPTO` | `OFF` | Use system libcrypto instead of AWS-LC |

### Running Tests

```bash
cmake -S . -B build -DSSZ_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite includes unit tests for all modules and the full set of [SSZ generic test vectors](https://github.com/ethereum/consensus-specs/tree/dev/tests/generators/ssz_generic) from the consensus specs.

### Running Benchmarks

```bash
cmake -S . -B build -DSSZ_BUILD_BENCH=ON
cmake --build build
./build/bench_serialize
./build/bench_deserialize
./build/bench_hash
./build/bench_merkle
./build/bench_merkle_cache
```

## Project Structure

```
c-ssz/
├── include/          Public headers
│   ├── ssz.h         Master include
│   ├── ssz_types.h   Type definitions (ssz_chunk_t, ssz_error_t, etc.)
│   ├── ssz_serialize.h
│   ├── ssz_deserialize.h
│   ├── ssz_merkle.h
│   ├── ssz_merkle_cache.h
│   ├── ssz_proof.h
│   └── ssz_hash.h
├── src/              Implementation
├── tests/            Unit and spec tests
├── bench/            Micro-benchmarks
├── fuzz/             libFuzzer harnesses
└── external/         Submodules (aws-lc, consensus-specs)
```

## License

This project is licensed under the MIT License. See [`LICENSE-MIT`](LICENSE-MIT) for details.
