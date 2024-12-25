#ifndef SSZ_MERKLEIZATION_H
#define SSZ_MERKLEIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * This function takes an array of BYTES_PER_CHUNK-sized chunks and Merkleizes them.
 * The 'chunk_count' parameter is the number of input chunks. If 'limit' is non-zero,
 * it enforces a maximum number of chunks for padding and will return an error if
 * 'chunk_count' exceeds that limit. Internally, it will pad to the next power of two
 * (or 'limit' if specified), then build the Merkle tree and return the resulting
 * 32-byte root in 'out_root'.
 */
ssz_error_t ssz_merkleize(
    const uint8_t *chunks,
    size_t chunk_count,
    size_t limit,
    uint8_t *out_root);

/**
 * Packs an array of basic-type values into BYTES_PER_CHUNK-sized chunks. It writes
 * the final chunk-aligned data into 'out_chunks'. The total number of chunks is
 * returned in 'out_chunk_count'. The function zero-pads to a multiple of
 * BYTES_PER_CHUNK if necessary. The 'value_size' is the size in bytes of each
 * basic-type element, and 'value_count' is how many such elements there are.
 * This routine is essential for transforming arrays of integers or bytes into a
 * form suitable for Merkleization.
 */
ssz_error_t ssz_pack(
    const uint8_t *values,
    size_t value_size,
    size_t value_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Packs bits (for bitvector or bitlist, excluding the trailing bit for bitlists)
 * into BYTES_PER_CHUNK-sized chunks. The function first packs the bits into an array
 * of bytes in little-endian bit ordering, then it zero-pads to the chunk boundary,
 * and finally it splits into BYTES_PER_CHUNK-sized chunks suitable for Merkleization.
 * The resulting chunks are written to 'out_chunks', and the total chunk count is
 * written to 'out_chunk_count'.
 */
ssz_error_t ssz_pack_bits(
    const bool *bits,
    size_t bit_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Combines a Merkle root with a length value (of type uint256 in little-endian format)
 * to produce a new root, in accordance with the SSZ spec's 'mix_in_length'. Typically,
 * this is used to finalize the root of a list or bitlist. The length is provided as
 * a 64-bit value for convenience, but the function will zero-pad it to 256 bits
 * internally as needed before hashing.
 */
ssz_error_t ssz_mix_in_length(
    const uint8_t *root,
    uint64_t length,
    uint8_t *out_root);

/**
 * Similar to mix_in_length, but instead of length, we mix in a union selector. The spec
 * treats the selector the same way (a 256-bit value in little-endian), so we zero-pad
 * the single-byte selector to 256 bits and compute a new hash with the given 'root'.
 * The resulting mixed-in root is written to 'out_root'.
 */
ssz_error_t ssz_mix_in_selector(
    const uint8_t *root,
    uint8_t selector,
    uint8_t *out_root);

#endif /* SSZ_MERKLEIZATION_H */
