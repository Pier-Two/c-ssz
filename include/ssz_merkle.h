#ifndef SSZ_MERKLE_H
#define SSZ_MERKLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Merkleizes an array of BYTES_PER_CHUNK-sized chunks.
 * Pads the input to the next power of two (or 'limit' if specified), builds the Merkle tree,
 * and writes the resulting 32-byte root to 'out_root'.
 * 
 * @param chunks Pointer to the input chunks.
 * @param chunk_count The number of input chunks.
 * @param limit The maximum number of chunks for padding. If 0, no limit is enforced.
 * @param out_root Pointer to the output buffer for the Merkle root.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_merkleize(
    const uint8_t *chunks,
    size_t chunk_count,
    size_t limit,
    uint8_t *out_root);

/**
 * Packs an array of basic-type values into BYTES_PER_CHUNK-sized chunks.
 * Zero-pads the data to a multiple of BYTES_PER_CHUNK if necessary and writes the
 * chunk-aligned data to 'out_chunks'. The total number of chunks is returned in 'out_chunk_count'.
 * 
 * @param values Pointer to the input values.
 * @param value_size The size in bytes of each basic-type element.
 * @param value_count The number of elements in the input array.
 * @param out_chunks Pointer to the output buffer for the packed chunks.
 * @param out_chunk_count Pointer to store the total number of chunks.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_pack(
    const uint8_t *values,
    size_t value_size,
    size_t value_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Packs bits into BYTES_PER_CHUNK-sized chunks.
 * Packs the bits into an array of bytes in little-endian bit ordering, zero-pads to the
 * chunk boundary, and splits into BYTES_PER_CHUNK-sized chunks. The resulting chunks
 * are written to 'out_chunks', and the total chunk count is written to 'out_chunk_count'.
 * 
 * @param bits Pointer to the input bit array.
 * @param bit_count The number of bits to pack.
 * @param out_chunks Pointer to the output buffer for the packed chunks.
 * @param out_chunk_count Pointer to store the total number of chunks.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_pack_bits(
    const bool *bits,
    size_t bit_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Combines a Merkle root with a length value to produce a new root.
 * The length is provided as a 64-bit value and zero-padded to 256 bits internally
 * before hashing, in accordance with the SSZ spec's 'mix_in_length'.
 * 
 * @param root Pointer to the input Merkle root.
 * @param length The length value to mix in.
 * @param out_root Pointer to the output buffer for the new root.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_mix_in_length(
    const uint8_t *root,
    uint64_t length,
    uint8_t *out_root);

/**
 * Combines a Merkle root with a union selector to produce a new root.
 * The selector is zero-padded to 256 bits internally before hashing, in accordance
 * with the SSZ spec's 'mix_in_selector'.
 * 
 * @param root Pointer to the input Merkle root.
 * @param selector The union selector to mix in.
 * @param out_root Pointer to the output buffer for the new root.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_mix_in_selector(
    const uint8_t *root,
    uint8_t selector,
    uint8_t *out_root);

#endif /* SSZ_MERKLE_H */
