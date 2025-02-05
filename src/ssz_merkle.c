#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "ssz_merkle.h"
#include "ssz_utils.h"
#include "ssz_constants.h"
#include "ssz_serialize.h"
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
    uint8_t *out_root)
{
    size_t effective_count = chunk_count;
    if (limit != 0) {
        if (chunk_count > limit) {
            return SSZ_ERROR_SERIALIZATION;
        }
        effective_count = limit;
    }

    size_t padded_leaves = next_pow_of_two(effective_count);
    size_t num_leaves = padded_leaves;
    uint8_t *nodes = malloc(num_leaves * BYTES_PER_CHUNK);
    if (!nodes) {
        return SSZ_ERROR_SERIALIZATION;
    }

    if (chunk_count > 0) {
        memcpy(nodes, chunks, chunk_count * BYTES_PER_CHUNK);
    }

    for (size_t i = chunk_count; i < num_leaves; i++) {
        memset(nodes + i * BYTES_PER_CHUNK, 0, BYTES_PER_CHUNK);
    }

    while (num_leaves > 1) {
        size_t parent_count = num_leaves / 2;
        for (size_t i = 0; i < parent_count; i++) {
            uint8_t concat[64];
            memcpy(concat, nodes + 2 * i * BYTES_PER_CHUNK, BYTES_PER_CHUNK);
            memcpy(concat + BYTES_PER_CHUNK, nodes + (2 * i + 1) * BYTES_PER_CHUNK, BYTES_PER_CHUNK);
            SHA256(concat, 64, nodes + i * BYTES_PER_CHUNK);
        }
        num_leaves = parent_count;
    }

    memcpy(out_root, nodes, BYTES_PER_CHUNK);
    free(nodes);
    return SSZ_SUCCESS;
}

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
    size_t *out_chunk_count)
{
    size_t total_bytes = value_size * value_count;
    if (total_bytes == 0) {
        *out_chunk_count = 0;
        return SSZ_SUCCESS;
    }
    size_t chunk_count = (total_bytes + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK;
    size_t padded_size = chunk_count * BYTES_PER_CHUNK;
    memcpy(out_chunks, values, total_bytes);
    if (padded_size > total_bytes) {
        memset(out_chunks + total_bytes, 0, padded_size - total_bytes);
    }
    *out_chunk_count = chunk_count;
    return SSZ_SUCCESS;
}

/**
 * Packs an array of boolean values into fixed-size chunks as a bitfield.
 *
 * This function converts the boolean array into a bitfield representation,
 * then packs the bitfield into chunks of size BYTES_PER_CHUNK, zero-padding
 * if necessary. Note that this version does not set a terminating bit, making
 * it usable for either bitvectors or bitlists (caller may handle the extra bit
 * if needed).
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
    size_t *out_chunk_count)
{
    size_t bitfield_len = (bit_count + 7) / 8;
    size_t chunk_count = (bitfield_len + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK;
    size_t padded_size = chunk_count * BYTES_PER_CHUNK;
    memset(out_chunks, 0, padded_size);
    for (size_t i = 0; i < bit_count; i++) {
        if (bits[i]) {
            out_chunks[i / 8] |= (1 << (i % 8));
        }
    }
    *out_chunk_count = chunk_count;
    return SSZ_SUCCESS;
}

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
    uint8_t *out_root)
{
    uint8_t buf[64];
    memcpy(buf, root, 32);
    memset(buf + 32, 0, 32);
    for (size_t i = 0; i < 8; i++) {
        buf[32 + i] = (uint8_t)((length >> (8 * i)) & 0xFF);
    }
    SHA256(buf, 64, out_root);
    return SSZ_SUCCESS;
}

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
    uint8_t *out_root)
{
    uint8_t buf[64];
    memcpy(buf, root, 32);
    memset(buf + 32, 0, 32);
    buf[32] = selector;
    SHA256(buf, 64, out_root);
    return SSZ_SUCCESS;
}

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
ssz_error_t ssz_hash_tree_root_uint8(const uint8_t *value, uint8_t *out_root) {
    uint8_t serialized[1];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint8(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 1, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_uint16(const uint16_t *value, uint8_t *out_root) {
    uint8_t serialized[2];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint16(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 2, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_uint32(const uint32_t *value, uint8_t *out_root) {
    uint8_t serialized[4];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint32(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 4, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_uint64(const uint64_t *value, uint8_t *out_root) {
    uint8_t serialized[8];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint64(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 8, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_uint128(const void *value, uint8_t *out_root) {
    uint8_t serialized[16];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint128(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 16, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_uint256(const void *value, uint8_t *out_root) {
    uint8_t serialized[32];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_uint256(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(serialized, 1, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_boolean(bool value, uint8_t *out_root) {
    uint8_t serialized[1];
    size_t ser_size = 0;
    ssz_error_t err = ssz_serialize_boolean(value, serialized, &ser_size);
    if (err != SSZ_SUCCESS) return err;

    uint8_t chunk[BYTES_PER_CHUNK] = {0};
    size_t chunk_count = 0;
    err = ssz_pack(serialized, 1, 1, chunk, &chunk_count);
    if (err != SSZ_SUCCESS) return err;

    return ssz_merkleize(chunk, chunk_count, 0, out_root);
}

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
ssz_error_t ssz_hash_tree_root_bitvector(const bool *bits, size_t num_bits, uint8_t *out_root) {
    size_t expected_chunks = chunk_count_bitvector(num_bits);
    uint8_t *packed = malloc(expected_chunks * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack_bits(bits, num_bits, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, expected_chunks, out_root);
    free(packed);
    return merkle_err;
}

/**
 * Computes the Merkle tree root for a bitlist.
 *
 * This function packs an array of boolean values into a bitfield, organizes the bitfield into fixed-size chunks,
 * and computes the Merkle tree root from the packed data. It then mixes in the length of the bitlist to produce
 * the final root, ensuring that the length is taken into account.
 *
 * @param bits Pointer to the boolean array representing the bitlist.
 * @param num_bits Number of bits in the bitlist.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_bitlist(const bool *bits, size_t num_bits, uint8_t *out_root) {
    size_t expected_chunks = chunk_count_bitlist(num_bits);
    uint8_t *packed = malloc(expected_chunks * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack_bits(bits, num_bits, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, expected_chunks, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;

    return ssz_mix_in_length(temp_root, num_bits, out_root);
}

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
ssz_error_t ssz_hash_tree_root_vector_uint8(const uint8_t *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT8);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack(elements, BYTE_SIZE_OF_UINT8, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_uint16(const uint16_t *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT16);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_UINT16, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_uint32(const uint32_t *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT32);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_UINT32, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_uint64(const uint64_t *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT64);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_UINT64, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_uint128(const void *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT128);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_UINT128, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_uint256(const void *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_UINT256);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_UINT256, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

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
ssz_error_t ssz_hash_tree_root_vector_bool(const bool *elements, size_t element_count, uint8_t *out_root) {
    size_t needed = chunk_count_vector_basic(element_count, BYTE_SIZE_OF_BOOL);
    uint8_t *packed = malloc(needed * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, BYTE_SIZE_OF_BOOL, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return merkle_err;
}

/**
 * Computes the Merkle tree root for a list of uint8 values.
 *
 * This function packs an array of uint8 elements into fixed-size chunks using a basic size,
 * computes a temporary Merkle root, and then mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint8 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint8(const uint8_t *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT8;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;

    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack(elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }

    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;

    return ssz_mix_in_length(temp_root, element_count, out_root);
}

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
ssz_error_t ssz_hash_tree_root_list_uint16(const uint16_t *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT16;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}

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
ssz_error_t ssz_hash_tree_root_list_uint32(const uint32_t *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT32;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}

/**
 * Computes the Merkle tree root for a list of uint64 values.
 *
 * This function packs an array of uint64 elements into fixed-size chunks,
 * computes a temporary Merkle root, and then mixes in the list length to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint64 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint64(const uint64_t *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT64;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}

/**
 * Computes the Merkle tree root for a list of uint128 values.
 *
 * This function packs an array of 128-bit unsigned integers into fixed-size chunks,
 * computes a temporary Merkle tree root, and mixes in the length of the list to produce
 * the final root.
 *
 * @param elements Pointer to the array of uint128 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint128(const void *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT128;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}

/**
 * Computes the Merkle tree root for a list of uint256 values.
 *
 * This function packs an array of 256-bit unsigned integers into fixed-size chunks,
 * computes a temporary Merkle tree root, and mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of uint256 elements.
 * @param element_count Number of elements in the list.
 * @param out_root Output buffer to store the computed Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_uint256(const void *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_UINT256;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}

/**
 * Computes the Merkle tree root for a list of boolean values.
 *
 * This function packs an array of boolean elements into fixed-size chunks,
 * computes a temporary Merkle tree root, and then mixes in the length of the list to produce
 * the final Merkle tree root.
 *
 * @param elements Pointer to the array of boolean elements.
 * @param element_count Number of boolean elements in the list.
 * @param out_root Output buffer to store the resulting Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_hash_tree_root_list_bool(const bool *elements, size_t element_count, uint8_t *out_root) {
    size_t basic_size = BYTE_SIZE_OF_BOOL;
    size_t limit = chunk_count_list_basic(element_count, basic_size);
    uint8_t *packed = malloc(limit * BYTES_PER_CHUNK);
    if (!packed) return SSZ_ERROR_MERKLEIZATION;
    
    size_t chunk_count = 0;
    ssz_error_t err = ssz_pack((const uint8_t *)elements, basic_size, element_count, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    
    uint8_t temp_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed, chunk_count, limit, temp_root);
    free(packed);
    if (merkle_err != SSZ_SUCCESS) return merkle_err;
    
    return ssz_mix_in_length(temp_root, element_count, out_root);
}