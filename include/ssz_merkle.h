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
 * buffer alongside the root, and computes SHA256 to produce a new Merkle root.
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

/**
 * Computes the Merkle tree root for a serialized uint8 value.
 *
 * This function serializes the given uint8 value, packs the result into a fixed-size chunk,
 * and then computes the Merkle tree root using the packed chunk.
 *
 * @param value Pointer to the uint8 value to hash.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint8(const uint8_t *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized uint16 value.
 *
 * This function serializes the given uint16 value, packs the serialized data into a fixed-size chunk,
 * and computes the corresponding Merkle tree root.
 *
 * @param value Pointer to the uint16 value to hash.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint16(const uint16_t *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized uint32 value.
 *
 * This function serializes the given uint32 value into bytes, packs the data into a fixed-size chunk,
 * and computes the Merkle tree root from the chunk.
 *
 * @param value Pointer to the uint32 value to hash.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint32(const uint32_t *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized uint64 value.
 *
 * This function serializes the given uint64 value, packs the serialized data into a fixed-size chunk,
 * and computes the corresponding Merkle tree root.
 *
 * @param value Pointer to the uint64 value to hash.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint64(const uint64_t *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized uint128 value.
 *
 * This function serializes the given 128-bit unsigned integer, packs the serialized data into a fixed-size chunk,
 * and computes the Merkle tree root from the chunk.
 *
 * @param value Pointer to the uint128 value to hash.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint128(const void *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized uint256 value.
 *
 * This function serializes the given 256-bit unsigned integer into bytes,
 * and directly computes the Merkle tree root using the serialized data.
 *
 * @param value Pointer to the uint256 value to hash.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_uint256(const void *value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a serialized boolean value.
 *
 * This function serializes a boolean value, packs the serialized data into a fixed-size chunk,
 * and computes the Merkle tree root from the chunk.
 *
 * @param value Boolean value to be hashed.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_boolean(bool value, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a bitvector.
 *
 * This function packs an array of boolean values into a bitfield, organizes the bitfield into fixed-size chunks,
 * and computes the Merkle tree root using the packed data. The expected number of chunks is determined by the bitvector size.
 *
 * @param bits Pointer to the boolean array representing the bitvector.
 * @param num_bits Number of bits in the bitvector.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_bitvector(const bool *bits, size_t num_bits, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a bitlist.
 *
 * This function packs an array of boolean values into a bitfield, organizes the bitfield into fixed-size chunks,
 * computes the Merkle tree root from the packed data, and then mixes in the length of the bitlist to produce
 * the final root.
 *
 * @param bits Pointer to the boolean array representing the bitlist.
 * @param num_bits Number of bits in the bitlist.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_bitlist(const bool *bits, size_t num_bits, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint8 values.
 *
 * This function packs a contiguous array of uint8 elements into fixed-size chunks,
 * and computes the Merkle tree root from the resulting chunks.
 *
 * @param elements Pointer to the array of uint8 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint8(const uint8_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint16 values.
 *
 * This function packs a contiguous array of uint16 elements into fixed-size chunks,
 * and computes the Merkle tree root from the resulting chunks.
 *
 * @param elements Pointer to the array of uint16 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint16(const uint16_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint32 values.
 *
 * This function packs a contiguous array of uint32 elements into fixed-size chunks,
 * and computes the Merkle tree root from the packed data.
 *
 * @param elements Pointer to the array of uint32 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint32(const uint32_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint64 values.
 *
 * This function packs a contiguous array of uint64 elements into fixed-size chunks,
 * and computes the Merkle tree root from the resulting packed data.
 *
 * @param elements Pointer to the array of uint64 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint64(const uint64_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint128 values.
 *
 * This function packs a contiguous array of 128-bit unsigned integers into fixed-size chunks,
 * and computes the Merkle tree root from the packed data.
 *
 * @param elements Pointer to the array of uint128 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint128(const void *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of uint256 values.
 *
 * This function packs a contiguous array of 256-bit unsigned integers into fixed-size chunks,
 * and computes the Merkle tree root from the packed data.
 *
 * @param elements Pointer to the array of uint256 elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_uint256(const void *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a vector of boolean values.
 *
 * This function packs a contiguous array of boolean values into fixed-size chunks,
 * and computes the Merkle tree root from the packed data.
 *
 * @param elements Pointer to the array of boolean elements.
 * @param element_count Number of elements in the array.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_vector_bool(const bool *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint8 values.
 *
 * This function packs an array of uint8 elements into fixed-size chunks,
 * computes a temporary Merkle root, and then mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint8 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint8(const uint8_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint16 values.
 *
 * This function packs an array of uint16 elements into fixed-size chunks,
 * computes a temporary Merkle root, and mixes in the list length to produce the final root.
 *
 * @param elements Pointer to the array of uint16 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint16(const uint16_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint32 values.
 *
 * This function packs an array of uint32 elements into fixed-size chunks,
 * computes a temporary Merkle root, and mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint32 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint32(const uint32_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint64 values.
 *
 * This function packs an array of uint64 elements into fixed-size chunks,
 * computes a temporary Merkle root, and then mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint64 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint64(const uint64_t *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint128 values.
 *
 * This function packs an array of 128-bit unsigned integers into fixed-size chunks,
 * computes a temporary Merkle root, and mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint128 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint128(const void *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of uint256 values.
 *
 * This function packs an array of 256-bit unsigned integers into fixed-size chunks,
 * computes a temporary Merkle root, and mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint256 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint256(const void *elements, size_t element_count, uint8_t *out_root);

/**
 * Computes the Merkle tree root for a list of boolean values.
 *
 * This function packs an array of boolean elements into fixed-size chunks,
 * computes a temporary Merkle root, and then mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of boolean elements.
 * @param element_count Number of boolean elements in the list.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_bool(const bool *elements, size_t element_count, uint8_t *out_root);

#endif /* SSZ_MERKLE_H */