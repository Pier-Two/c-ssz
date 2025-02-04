#ifndef SSZ_MERKLE_H
#define SSZ_MERKLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Computes the Merkle root from an array of chunks.
 *
 * This function constructs a Merkle tree by first copying the provided chunks
 * into leaf nodes, padding with zeros if necessary, and then iteratively hashing
 * pairs of nodes until a single root is obtained.
 *
 * @param chunks Pointer to the array of chunks (each chunk is BYTES_PER_CHUNK bytes).
 * @param chunk_count Number of chunks provided.
 * @param limit Maximum number of chunks allowed; if non-zero, chunk_count must not exceed this limit.
 * @param out_root Output buffer to write the resulting Merkle root (at least BYTES_PER_CHUNK bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_merkleize(
    const uint8_t *chunks,
    size_t chunk_count,
    size_t limit,
    uint8_t *out_root);

/**
 * Packs a contiguous byte array into fixed-size chunks.
 *
 * This function divides the input byte array into chunks of size BYTES_PER_CHUNK.
 * If the total number of bytes is not a multiple of BYTES_PER_CHUNK, the last chunk is zero-padded.
 *
 * @param values Pointer to the input byte array.
 * @param value_size Size of each value element in bytes.
 * @param value_count Number of values in the array.
 * @param out_chunks Output buffer to write the chunked data.
 * @param out_chunk_count Pointer to store the number of chunks written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_pack(
    const uint8_t *values,
    size_t value_size,
    size_t value_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Packs an array of boolean values into fixed-size chunks as a bitfield.
 *
 * This function converts the boolean array into a bitfield representation,
 * appending a terminating bit after the provided bits, then packs the bitfield
 * into chunks of size BYTES_PER_CHUNK, padding with zeros if necessary.
 *
 * @param bits Pointer to the array of boolean values.
 * @param bit_count Number of boolean values in the array.
 * @param out_chunks Output buffer to write the packed bitfield chunks.
 * @param out_chunk_count Pointer to store the number of chunks written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_pack_bits(
    const bool *bits,
    size_t bit_count,
    uint8_t *out_chunks,
    size_t *out_chunk_count);

/**
 * Mixes a length value into a Merkle root to produce an updated root.
 *
 * This function takes an existing Merkle root and a length value, then mixes the length
 * into the root by placing it in a buffer alongside the original root and computing SHA256.
 *
 * @param root Pointer to the original Merkle root (32 bytes).
 * @param length 64-bit unsigned integer representing the length to mix in.
 * @param out_root Output buffer to write the new Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_mix_in_length(
    const uint8_t *root,
    uint64_t length,
    uint8_t *out_root);

/**
 * Mixes a selector byte into a Merkle root to produce an updated root.
 *
 * This function takes an existing Merkle root and a selector byte, places the selector in a
 * buffer along with the root, and computes SHA256 to produce a new Merkle root.
 *
 * @param root Pointer to the original Merkle root (32 bytes).
 * @param selector The selector byte to mix into the root.
 * @param out_root Output buffer to write the new Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_mix_in_selector(
    const uint8_t *root,
    uint8_t selector,
    uint8_t *out_root);

#endif /* SSZ_MERKLE_H */
