#include <stdlib.h>
#include <string.h>
#include "crypto/mincrypt/sha256.h"
#include "ssz_merkle.h"
#include "ssz_utils.h"
#include "ssz_constants.h"

/**
 * Computes the Merkle root from an array of chunks.
 *
 * This function constructs a Merkle tree by first copying the provided chunks
 * into leaf nodes, padding with zeros if necessary, and then iteratively hashing
 * pairs of nodes until a single root is obtained.
 *
 * @param chunks Pointer to the array of chunks (each chunk is SSZ_BYTES_PER_CHUNK bytes).
 * @param chunk_count Number of chunks provided.
 * @param limit Maximum number of chunks allowed; if non-zero, chunk_count must not exceed this limit.
 * @param out_root Output buffer to write the resulting Merkle root (at least SSZ_BYTES_PER_CHUNK bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_merkleize(const uint8_t *restrict chunks, size_t chunk_count, size_t limit, uint8_t *restrict out_root) 
{
    size_t effective = chunk_count;
    if (limit != 0) 
    {
        if (chunk_count > limit) 
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        effective = limit;
    }
    if (effective == 0) 
    {
        memset(out_root, 0, SSZ_BYTES_PER_CHUNK);
        return SSZ_SUCCESS;
    }
    size_t padded = next_pow_of_two(effective);
    if (padded == 1) 
    {
        if (chunk_count >= 1) 
        {
            memcpy(out_root, chunks, SSZ_BYTES_PER_CHUNK);
        } else 
        {
            memset(out_root, 0, SSZ_BYTES_PER_CHUNK);
        }
        return SSZ_SUCCESS;
    }
    uint8_t *restrict nodes = malloc(padded * SSZ_BYTES_PER_CHUNK);
    if (!nodes) 
    {
        return SSZ_ERROR_MERKLEIZATION;
    }
    if (chunk_count) 
    {
        memcpy(nodes, chunks, chunk_count * SSZ_BYTES_PER_CHUNK);
    }
    if (chunk_count < padded) 
    {
        memset(nodes + chunk_count * SSZ_BYTES_PER_CHUNK, 0, (padded - chunk_count) * SSZ_BYTES_PER_CHUNK);
    }
    size_t num = padded;
    while (num > 1) 
    {
        size_t parent = num >> 1;
        for (size_t i = 0; i < parent; i++) 
        {
            SHA256_hash(nodes + (2 * i) * SSZ_BYTES_PER_CHUNK, 2 * SSZ_BYTES_PER_CHUNK, nodes + i * SSZ_BYTES_PER_CHUNK);
        }
        num = parent;
    }
    memcpy(out_root, nodes, SSZ_BYTES_PER_CHUNK);
    free(nodes);
    return SSZ_SUCCESS;
}

/**
 * Packs a contiguous byte array into fixed-size chunks.
 *
 * This function divides the input byte array into chunks of size SSZ_BYTES_PER_CHUNK.
 * If the total number of bytes is not a multiple of SSZ_BYTES_PER_CHUNK, the last chunk is zero-padded.
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
    if (value_count != 0 && total_bytes / value_count != value_size) {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (total_bytes == 0)
    {
        *out_chunk_count = 0;
        return SSZ_SUCCESS;
    }
    size_t chunk_count = (total_bytes + SSZ_BYTES_PER_CHUNK - 1) / SSZ_BYTES_PER_CHUNK;
    size_t padded_size = chunk_count * SSZ_BYTES_PER_CHUNK;
    memcpy(out_chunks, values, total_bytes);
    if (padded_size > total_bytes)
    {
        memset(out_chunks + total_bytes, 0, padded_size - total_bytes);
    }
    *out_chunk_count = chunk_count;
    return SSZ_SUCCESS;
}

/**
 * Packs an array of boolean values into fixed-size chunks.
 *
 * This function converts a bitfield represented as an array of booleans into a compact byte array,
 * and then divides that byte array into fixed-size chunks (each of size SSZ_BYTES_PER_CHUNK) for Merkleization.
 * If bit_count is zero, a single default chunk is generated.
 *
 * @param bits Pointer to the array of boolean values.
 * @param bit_count Number of boolean values in the array.
 * @param out_chunks Output buffer to write the packed bitfield chunks.
 * @param out_chunk_count Pointer to store the number of chunks written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_pack_bits(const bool *bits, size_t bit_count, uint8_t *out_chunks, size_t *out_chunk_count) {
    size_t bitfield_len = bit_count ? (bit_count + 7) >> 3 : 1;
    uint8_t small_buf[SSZ_SMALL_BUFFER_SIZE];
    uint8_t *bitfield_bytes = bitfield_len <= SSZ_SMALL_BUFFER_SIZE ? small_buf : malloc(bitfield_len);
    if (!bitfield_bytes) 
    {
        return SSZ_ERROR_MERKLEIZATION;
    }
    if (!bit_count) 
    {
        bitfield_bytes[0] = 0x01;
    }
    else 
    {
        memset(bitfield_bytes, 0, bitfield_len);
        size_t full_bytes = bit_count >> 3, rem = bit_count & 7;
        for (size_t i = 0; i < full_bytes; i++) 
        {
            size_t base = i << 3;
            bitfield_bytes[i] = (uint8_t)bits[base] | (uint8_t)bits[base+1] << 1 |
                (uint8_t)bits[base+2] << 2 | (uint8_t)bits[base+3] << 3 |
                (uint8_t)bits[base+4] << 4 | (uint8_t)bits[base+5] << 5 |
                (uint8_t)bits[base+6] << 6 | (uint8_t)bits[base+7] << 7;
        }
        if (rem) 
        {
            uint8_t byte = 0;
            for (size_t j = 0, base = full_bytes << 3; j < rem; j++) byte |= (uint8_t)bits[base + j] << j;
            bitfield_bytes[full_bytes] = byte;
        }
    }
    ssz_error_t err = ssz_pack(bitfield_bytes, 1, bitfield_len, out_chunks, out_chunk_count);
    if (bitfield_bytes != small_buf)
    {
        free(bitfield_bytes);
    }
    return err;
}

/**
 * Mixes a length value into a Merkle root to produce an updated root.
 *
 * This function takes an existing Merkle root and a length value, then mixes the length
 * into the root by placing it in a buffer alongside the original root and computing SHA256_hash.
 *
 * @param root Pointer to the original Merkle root (32 bytes).
 * @param length 64-bit unsigned integer representing the length to mix in.
 * @param out_root Output buffer to write the new Merkle root (32 bytes).
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_mix_in_length(const uint8_t *root, uint64_t length, uint8_t *out_root) {
    uint8_t buf[64];
    memcpy(buf, root, 32);
    buf[32] = (uint8_t)length;
    buf[33] = (uint8_t)(length >> 8);
    buf[34] = (uint8_t)(length >> 16);
    buf[35] = (uint8_t)(length >> 24);
    buf[36] = (uint8_t)(length >> 32);
    buf[37] = (uint8_t)(length >> 40);
    buf[38] = (uint8_t)(length >> 48);
    buf[39] = (uint8_t)(length >> 56);
    memset(buf + 40, 0, 24);
    SHA256_hash(buf, 64, out_root);
    return SSZ_SUCCESS;
}

/**
 * Mixes a selector byte into a Merkle root to produce an updated root.
 *
 * This function takes an existing Merkle root and a selector byte, places the selector in a
 * buffer along with the root, and computes SHA256_hash to produce a new Merkle root.
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
    uint8_t buf[64] = {0};
    memcpy(buf, root, 32);
    buf[32] = selector;
    SHA256_hash(buf, 64, out_root);
    return SSZ_SUCCESS;
}