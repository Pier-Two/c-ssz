#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ssz_internal.h"
#include "ssz_merkle.h"

/* Portable 32-byte-aligned allocation for ssz_chunk_t arrays (C99). */
static void *ssz_internal_alloc_aligned32(size_t size)
{
    if (size == 0u)
    {
        size = 1u;
    }
    size_t total = size + 32u + sizeof(void *);
    if (total < size)
    {
        return NULL;
    }
    void *raw = malloc(total);
    if (raw == NULL)
    {
        return NULL;
    }
    uintptr_t addr = ((uintptr_t)raw + sizeof(void *) + 31u) & ~(uintptr_t)31u;
    ((void **)addr)[-1] = raw;
    return (void *)addr;
}

static void ssz_internal_free_aligned32(void *ptr)
{
    if (ptr != NULL)
    {
        free(((void **)ptr)[-1]);
    }
}

typedef ssz_error_t (*ssz_internal_leaf_reader_t)(
    const void *ctx,
    uint64_t index,
    ssz_chunk_t *out_leaf);

typedef struct
{
    const ssz_chunk_t *chunks;
    uint64_t count;
} ssz_internal_chunk_reader_ctx_t;

typedef struct
{
    const uint8_t *bytes;
    size_t byte_len;
    uint64_t chunk_count;
} ssz_internal_bytes_reader_ctx_t;

typedef struct
{
    const ssz_member_codec_t *codec;
    uint64_t count;
} ssz_internal_codec_reader_ctx_t;

#define SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES  UINT64_C(131072)
#define SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES 64u

static ssz_error_t ssz_internal_read_chunk_leaf(
    const void *ctx,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    const ssz_internal_chunk_reader_ctx_t *reader = (const ssz_internal_chunk_reader_ctx_t *)ctx;
    if ((reader == NULL) || (out_leaf == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((index >= reader->count) || ((reader->chunks == NULL) && (reader->count != 0u)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_leaf = reader->chunks[index];
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_read_bytes_leaf(
    const void *ctx,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    const ssz_internal_bytes_reader_ctx_t *reader = (const ssz_internal_bytes_reader_ctx_t *)ctx;
    uint64_t start_u64 = 0u;
    size_t start = 0u;

    if ((reader == NULL) || (out_leaf == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (index >= reader->chunk_count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((reader->byte_len != 0u) && (reader->bytes == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (ssz_internal_mul_overflow_u64(index, SSZ_BYTES_PER_CHUNK, &start_u64) ||
        !ssz_internal_u64_to_size(start_u64, &start))
    {
        return SSZ_ERR_OVERFLOW;
    }

    memset(out_leaf->bytes, 0, SSZ_BYTES_PER_CHUNK);
    if (start < reader->byte_len)
    {
        size_t remaining = reader->byte_len - start;
        size_t copy_len = (remaining < SSZ_BYTES_PER_CHUNK) ? remaining : SSZ_BYTES_PER_CHUNK;
        memcpy(out_leaf->bytes, reader->bytes + start, copy_len);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_read_codec_leaf(
    const void *ctx,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    const ssz_internal_codec_reader_ctx_t *reader = (const ssz_internal_codec_reader_ctx_t *)ctx;

    if ((reader == NULL) || (out_leaf == NULL) || (reader->codec == NULL) ||
        (reader->codec->root == NULL) || (index >= reader->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return reader->codec->root(reader->codec->ctx, index, out_leaf);
}

static uint32_t ssz_internal_log2_u64(uint64_t value)
{
    uint32_t depth = 0u;
    while (value > 1u)
    {
        value >>= 1u;
        depth++;
    }
    return depth;
}

static ssz_error_t ssz_internal_build_zero_hashes(
    uint32_t max_depth,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t zero_hashes[64])
{
    memset(zero_hashes[0].bytes, 0, SSZ_BYTES_PER_CHUNK);

    for (uint32_t depth = 1u; depth <= max_depth; depth++)
    {
        ssz_error_t err = ssz_hash_2to1(hash_fn, &zero_hashes[depth - 1u], &zero_hashes[depth - 1u],
                                        &zero_hashes[depth]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_merkleize_reader_fast(
    ssz_internal_leaf_reader_t reader,
    const void *reader_ctx,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t tree_size,
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t zero_hashes[64],
    ssz_chunk_t *out_root)
{
    size_t tree_size_sz = 0u;
    size_t leaf_count_sz = 0u;
    uint64_t source_end = 0u;
    size_t source_start_sz = 0u;
    size_t width = 0u;
    size_t level_storage_cap = 0u;
    size_t level_storage_bytes = 0u;
    ssz_chunk_t *level_storage = NULL;
    ssz_chunk_t stack_storage[SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES];
    ssz_error_t ret = SSZ_SUCCESS;

    if (!ssz_internal_u64_to_size(tree_size, &tree_size_sz) ||
        !ssz_internal_u64_to_size(leaf_count, &leaf_count_sz))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (tree_size_sz == 0u)
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_add_overflow_u64(source_start, leaf_count, &source_end))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (!ssz_internal_u64_to_size(source_start, &source_start_sz))
    {
        return SSZ_ERR_OVERFLOW;
    }

    if (tree_size_sz <= SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES)
    {
        uint64_t source_index = source_start;
        for (size_t i = 0u; i < leaf_count_sz; i++)
        {
            ssz_error_t err = reader(reader_ctx, source_index, &stack_storage[i]);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
            source_index++;
        }
        for (size_t i = leaf_count_sz; i < tree_size_sz; i++)
        {
            stack_storage[i] = zero_hashes[0];
        }

        width = tree_size_sz;
        while (width > 1u)
        {
            size_t pair_count = width >> 1u;
            ssz_error_t err = ssz_hash_2to1_batch_inplace(hash_fn, stack_storage, pair_count);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
            width = pair_count;
        }

        *out_root = stack_storage[0];
        return SSZ_SUCCESS;
    }

    if ((leaf_count == tree_size) && (tree_size_sz > 1u) && (reader == ssz_internal_read_chunk_leaf))
    {
        const ssz_internal_chunk_reader_ctx_t *chunk_reader =
            (const ssz_internal_chunk_reader_ctx_t *)reader_ctx;
        if ((chunk_reader != NULL) && ((chunk_reader->chunks != NULL) || (leaf_count == 0u)) &&
            (source_start <= chunk_reader->count) && ((chunk_reader->count - source_start) >= leaf_count))
        {
            const ssz_chunk_t *source_chunks = chunk_reader->chunks + source_start_sz;
            level_storage_cap = tree_size_sz >> 1u;
            if (ssz_internal_mul_overflow_size(level_storage_cap, sizeof(*level_storage), &level_storage_bytes))
            {
                return SSZ_ERR_OVERFLOW;
            }
            level_storage = (ssz_chunk_t *)ssz_internal_alloc_aligned32(level_storage_bytes);
            if (level_storage == NULL)
            {
                return SSZ_ERR_OVERFLOW;
            }

            ret = ssz_hash_2to1_batch(hash_fn, source_chunks, level_storage_cap, level_storage);
            if (ret != SSZ_SUCCESS)
            {
                goto cleanup;
            }
            width = level_storage_cap;
            goto reduce_levels;
        }
    }

    if ((leaf_count == tree_size) && (tree_size_sz > 1u) && (reader == ssz_internal_read_bytes_leaf))
    {
        const ssz_internal_bytes_reader_ctx_t *bytes_reader = (const ssz_internal_bytes_reader_ctx_t *)reader_ctx;
        size_t source_offset = 0u;
        size_t copy_len = 0u;

        if ((bytes_reader != NULL) && ((bytes_reader->bytes != NULL) || (bytes_reader->byte_len == 0u)) &&
            !ssz_internal_mul_overflow_size(source_start_sz, SSZ_BYTES_PER_CHUNK, &source_offset) &&
            !ssz_internal_mul_overflow_size(tree_size_sz, SSZ_BYTES_PER_CHUNK, &copy_len) &&
            (source_offset <= bytes_reader->byte_len) &&
            (copy_len <= (bytes_reader->byte_len - source_offset)))
        {
            level_storage_cap = tree_size_sz >> 1u;
            if (ssz_internal_mul_overflow_size(level_storage_cap, sizeof(*level_storage), &level_storage_bytes))
            {
                return SSZ_ERR_OVERFLOW;
            }
            level_storage = (ssz_chunk_t *)ssz_internal_alloc_aligned32(level_storage_bytes);
            if (level_storage == NULL)
            {
                return SSZ_ERR_OVERFLOW;
            }

            ret = ssz_hash_2to1_batch_raw(hash_fn, bytes_reader->bytes + source_offset, level_storage_cap, level_storage);
            if (ret != SSZ_SUCCESS)
            {
                goto cleanup;
            }
            width = level_storage_cap;
            goto reduce_levels;
        }
    }

    level_storage_cap = tree_size_sz;
    if (ssz_internal_mul_overflow_size(level_storage_cap, sizeof(*level_storage), &level_storage_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    level_storage = (ssz_chunk_t *)ssz_internal_alloc_aligned32(level_storage_bytes);
    if (level_storage == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    {
        uint64_t source_index = source_start;
        for (size_t i = 0u; i < leaf_count_sz; i++)
        {
            ret = reader(reader_ctx, source_index, &level_storage[i]);
            if (ret != SSZ_SUCCESS)
            {
                goto cleanup;
            }
            source_index++;
        }
        for (size_t i = leaf_count_sz; i < tree_size_sz; i++)
        {
            level_storage[i] = zero_hashes[0];
        }
    }
    width = tree_size_sz;

reduce_levels:
    while (width > 1u)
    {
        size_t pair_count = width >> 1u;
        ret = ssz_hash_2to1_batch_inplace(hash_fn, level_storage, pair_count);
        if (ret != SSZ_SUCCESS)
        {
            goto cleanup;
        }
        width = pair_count;
    }

    *out_root = level_storage[0];

cleanup:
    ssz_internal_free_aligned32(level_storage);
    return ret;
}

static ssz_error_t ssz_internal_merkleize_subtree(
    ssz_internal_leaf_reader_t reader,
    const void *reader_ctx,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t node_start,
    uint64_t subtree_size,
    uint32_t depth,
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t zero_hashes[64],
    ssz_chunk_t *out_root)
{
    if (node_start >= leaf_count)
    {
        *out_root = zero_hashes[depth];
        return SSZ_SUCCESS;
    }

    if (subtree_size == 1u)
    {
        uint64_t source_index = 0u;
        if (ssz_internal_add_overflow_u64(source_start, node_start, &source_index))
        {
            return SSZ_ERR_OVERFLOW;
        }
        return reader(reader_ctx, source_index, out_root);
    }

    if ((subtree_size <= SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES) &&
        ((leaf_count - node_start) >= subtree_size))
    {
        uint64_t subtree_source_start = 0u;
        if (ssz_internal_add_overflow_u64(source_start, node_start, &subtree_source_start))
        {
            return SSZ_ERR_OVERFLOW;
        }
        return ssz_internal_merkleize_reader_fast(reader,
                                                  reader_ctx,
                                                  subtree_source_start,
                                                  subtree_size,
                                                  subtree_size,
                                                  hash_fn,
                                                  zero_hashes,
                                                  out_root);
    }

    uint64_t half = subtree_size >> 1u;
    uint64_t right_start = 0u;
    if (ssz_internal_add_overflow_u64(node_start, half, &right_start))
    {
        return SSZ_ERR_OVERFLOW;
    }

    ssz_chunk_t left_root;
    ssz_chunk_t right_root;

    ssz_error_t err = ssz_internal_merkleize_subtree(reader,
                                                     reader_ctx,
                                                     source_start,
                                                     leaf_count,
                                                     node_start,
                                                     half,
                                                     depth - 1u,
                                                     hash_fn,
                                                     zero_hashes,
                                                     &left_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_internal_merkleize_subtree(reader,
                                         reader_ctx,
                                         source_start,
                                         leaf_count,
                                         right_start,
                                         half,
                                         depth - 1u,
                                         hash_fn,
                                         zero_hashes,
                                         &right_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_hash_2to1(hash_fn, &left_root, &right_root, out_root);
}

static ssz_error_t ssz_internal_merkleize_reader(
    ssz_internal_leaf_reader_t reader,
    const void *reader_ctx,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t limit,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    uint64_t effective_width = 0u;
    uint64_t tree_size = 0u;
    uint32_t depth = 0u;
    ssz_chunk_t zero_hashes_buf[64];
    const ssz_chunk_t *zero_hashes = NULL;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;

    if ((reader == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    resolved_hash_fn = ssz_internal_resolve_hash_fn(hash_fn);
    if ((resolved_hash_fn == NULL) || (resolved_hash_fn->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (limit == SSZ_NO_LIMIT)
    {
        effective_width = leaf_count;
    }
    else
    {
        if (leaf_count > limit)
        {
            return SSZ_ERR_LIMIT_EXCEEDED;
        }
        effective_width = limit;
    }

    tree_size = ssz_next_pow_of_two(effective_width);
    if (tree_size == 0u)
    {
        return SSZ_ERR_OVERFLOW;
    }

    depth = ssz_internal_log2_u64(tree_size);
    if (resolved_hash_fn == ssz_hash_default())
    {
        zero_hashes = ssz_hash_default_zero_hashes();
    }
    else
    {
        ssz_error_t err = ssz_internal_build_zero_hashes(depth, resolved_hash_fn, zero_hashes_buf);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        zero_hashes = zero_hashes_buf;
    }
    if (zero_hashes == NULL)
    {
        return SSZ_ERR_HASH_FAILURE;
    }

    if (leaf_count == 0u)
    {
        *out_root = zero_hashes[depth];
        return SSZ_SUCCESS;
    }

    if ((leaf_count == tree_size) && (tree_size <= SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES))
    {
        return ssz_internal_merkleize_reader_fast(reader,
                                                  reader_ctx,
                                                  source_start,
                                                  leaf_count,
                                                  tree_size,
                                                  resolved_hash_fn,
                                                  zero_hashes,
                                                  out_root);
    }

    return ssz_internal_merkleize_subtree(reader,
                                          reader_ctx,
                                          source_start,
                                          leaf_count,
                                          0u,
                                          tree_size,
                                          depth,
                                          resolved_hash_fn,
                                          zero_hashes,
                                          out_root);
}

static ssz_error_t ssz_internal_merkleize_progressive_reader(
    ssz_internal_leaf_reader_t reader,
    const void *reader_ctx,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t num_leaves,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    if ((reader == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (leaf_count == 0u)
    {
        memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        return SSZ_SUCCESS;
    }

    uint64_t left_count = (leaf_count < num_leaves) ? leaf_count : num_leaves;
    uint64_t right_count = leaf_count - left_count;

    ssz_chunk_t left_root;
    ssz_chunk_t right_root;

    ssz_error_t err =
        ssz_internal_merkleize_reader(reader, reader_ctx, source_start, left_count, num_leaves,
                                      hash_fn, &left_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    uint64_t next_source_start = 0u;
    if (ssz_internal_add_overflow_u64(source_start, left_count, &next_source_start))
    {
        return SSZ_ERR_OVERFLOW;
    }

    uint64_t next_num_leaves = 0u;
    if (ssz_internal_mul_overflow_u64(num_leaves, 4u, &next_num_leaves))
    {
        return SSZ_ERR_OVERFLOW;
    }

    err = ssz_internal_merkleize_progressive_reader(reader,
                                                    reader_ctx,
                                                    next_source_start,
                                                    right_count,
                                                    next_num_leaves,
                                                    hash_fn,
                                                    &right_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_hash_2to1(hash_fn, &left_root, &right_root, out_root);
}

static ssz_error_t ssz_internal_byte_len_to_chunk_count(size_t byte_len, uint64_t *out_chunk_count)
{
    size_t chunk_count = 0u;

    if (out_chunk_count == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (byte_len == 0u)
    {
        *out_chunk_count = 0u;
        return SSZ_SUCCESS;
    }

    if (ssz_internal_add_overflow_size(byte_len, SSZ_BYTES_PER_CHUNK - 1u, &chunk_count))
    {
        return SSZ_ERR_OVERFLOW;
    }
    chunk_count /= SSZ_BYTES_PER_CHUNK;

    *out_chunk_count = (uint64_t)chunk_count;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_merkleize_packed_bytes(
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t limit_chunks,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    uint64_t chunk_count = 0u;
    ssz_error_t err = ssz_internal_byte_len_to_chunk_count(bytes_len, &chunk_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if ((bytes_len != 0u) && (bytes == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_internal_bytes_reader_ctx_t ctx = {
        .bytes = bytes,
        .byte_len = bytes_len,
        .chunk_count = chunk_count,
    };

    return ssz_internal_merkleize_reader(
        ssz_internal_read_bytes_leaf,
        &ctx,
        0u,
        chunk_count,
        limit_chunks,
        hash_fn,
        out_root);
}

static ssz_error_t ssz_internal_validate_active_fields(
    const uint8_t *active_fields,
    size_t active_fields_len,
    uint32_t field_count)
{
    size_t one_bits = 0u;

    if (field_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((active_fields == NULL) || (active_fields_len == 0u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (active_fields_len > 32u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (active_fields[active_fields_len - 1u] == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    for (size_t i = 0u; i < active_fields_len; i++)
    {
        one_bits += ssz_internal_count_bits_u8(active_fields[i]);
    }

    if (one_bits != field_count)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint8(uint8_t value, ssz_chunk_t *out_root)
{
    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    out_root->bytes[0] = value;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint16(uint16_t value, ssz_chunk_t *out_root)
{
    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    ssz_internal_write_u16_le(out_root->bytes, value);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint32(uint32_t value, ssz_chunk_t *out_root)
{
    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    ssz_internal_write_u32_le(out_root->bytes, value);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint64(uint64_t value, ssz_chunk_t *out_root)
{
    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    ssz_internal_write_u64_le(out_root->bytes, value);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint128(const uint8_t value[16], ssz_chunk_t *out_root)
{
    if ((value == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    memcpy(out_root->bytes, value, 16u);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_uint256(const uint8_t value[32], ssz_chunk_t *out_root)
{
    if ((value == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memcpy(out_root->bytes, value, SSZ_BYTES_PER_CHUNK);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_tree_root_boolean(uint8_t value, ssz_chunk_t *out_root)
{
    if (value > 1u)
    {
        return SSZ_ERR_ENCODING_INVALID;
    }
    return ssz_hash_tree_root_uint8(value, out_root);
}

ssz_error_t ssz_hash_tree_root_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = 0u;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (bit_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (!ssz_internal_bits_to_bytes(bit_count, &bitfield_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((bits_le == NULL) || (bits_le_len < bitfield_bytes))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((bit_count % 8u) != 0u)
    {
        uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
        if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
    }

    if (ssz_internal_add_overflow_u64(bit_count, 255u, &chunk_limit))
    {
        return SSZ_ERR_OVERFLOW;
    }
    chunk_limit /= 256u;

    return ssz_internal_merkleize_packed_bytes(bits_le, bitfield_bytes, chunk_limit, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((bit_limit != SSZ_NO_LIMIT) && (bit_len > bit_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (!ssz_internal_bits_to_bytes(bit_len, &bitfield_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((bitfield_bytes != 0u) && ((bits_le == NULL) || (bits_le_len < bitfield_bytes)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((bit_len % 8u) != 0u)
    {
        uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
        if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
    }

    if (bit_limit != SSZ_NO_LIMIT)
    {
        if (ssz_internal_add_overflow_u64(bit_limit, 255u, &chunk_limit))
        {
            return SSZ_ERR_OVERFLOW;
        }
        chunk_limit /= 256u;
    }

    ssz_error_t err =
        ssz_internal_merkleize_packed_bytes(bits_le, bitfield_bytes, chunk_limit, hash_fn, &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, bit_len, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (element_size == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((elements == NULL) && (total_bytes != 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_internal_merkleize_packed_bytes(elements, total_bytes, SSZ_NO_LIMIT, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_vector_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((codec == NULL) || (codec->root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_internal_codec_reader_ctx_t ctx = {
        .codec = codec,
        .count = element_count,
    };

    return ssz_internal_merkleize_reader(
        ssz_internal_read_codec_leaf,
        &ctx,
        0u,
        element_count,
        SSZ_NO_LIMIT,
        hash_fn,
        out_root);
}

ssz_error_t ssz_hash_tree_root_vector_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    if (count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (count > (uint64_t)SIZE_MAX)
    {
        return SSZ_ERR_OVERFLOW;
    }

    return ssz_merkleize(roots, (size_t)count, SSZ_NO_LIMIT, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_size == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((elements == NULL) && (total_bytes != 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (element_limit != SSZ_NO_LIMIT)
    {
        uint64_t limit_bytes = 0u;
        if (ssz_internal_mul_overflow_u64(element_limit, (uint64_t)element_size, &limit_bytes) ||
            ssz_internal_add_overflow_u64(limit_bytes, SSZ_BYTES_PER_CHUNK - 1u, &chunk_limit))
        {
            return SSZ_ERR_OVERFLOW;
        }
        chunk_limit /= SSZ_BYTES_PER_CHUNK;
    }

    ssz_error_t err =
        ssz_internal_merkleize_packed_bytes(elements, total_bytes, chunk_limit, hash_fn, &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, element_count, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_list_composite(
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if ((codec == NULL) || (codec->root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_internal_codec_reader_ctx_t ctx = {
        .codec = codec,
        .count = element_count,
    };

    ssz_error_t err = ssz_internal_merkleize_reader(
        ssz_internal_read_codec_leaf,
        &ctx,
        0u,
        element_count,
        element_limit,
        hash_fn,
        &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, element_count, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    uint64_t limit,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((limit != SSZ_NO_LIMIT) && (count > limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (count > (uint64_t)SIZE_MAX)
    {
        return SSZ_ERR_OVERFLOW;
    }

    ssz_error_t err = ssz_merkleize(roots, (size_t)count, limit, hash_fn, &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, count, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_union(
    uint8_t selector,
    bool has_none,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t value_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (has_none && (selector == 0u))
    {
        memset(value_root.bytes, 0, SSZ_BYTES_PER_CHUNK);
    }
    else
    {
        if ((codec == NULL) || (codec->root == NULL))
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        ssz_error_t err = codec->root(codec->ctx, selector, &value_root);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return ssz_mix_in_selector(&value_root, selector, hash_fn, out_root);
}

ssz_error_t ssz_merkleize(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    uint64_t limit,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_internal_chunk_reader_ctx_t ctx = {
        .chunks = chunks,
        .count = (uint64_t)chunk_count,
    };

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((chunk_count != 0u) && (chunks == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_internal_merkleize_reader(
        ssz_internal_read_chunk_leaf,
        &ctx,
        0u,
        (uint64_t)chunk_count,
        limit,
        hash_fn,
        out_root);
}

ssz_error_t ssz_mix_in_length(
    const ssz_chunk_t *root,
    uint64_t length,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t length_chunk;

    if ((root == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(length_chunk.bytes, 0, SSZ_BYTES_PER_CHUNK);
    ssz_internal_write_u64_le(length_chunk.bytes, length);

    return ssz_hash_2to1(hash_fn, root, &length_chunk, out_root);
}

ssz_error_t ssz_mix_in_selector(
    const ssz_chunk_t *root,
    uint8_t selector,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t selector_chunk;

    if ((root == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(selector_chunk.bytes, 0, SSZ_BYTES_PER_CHUNK);
    selector_chunk.bytes[0] = selector;

    return ssz_hash_2to1(hash_fn, root, &selector_chunk, out_root);
}

ssz_error_t ssz_merkleize_progressive(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_internal_chunk_reader_ctx_t ctx = {
        .chunks = chunks,
        .count = (uint64_t)chunk_count,
    };

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((chunk_count != 0u) && (chunks == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_internal_merkleize_progressive_reader(
        ssz_internal_read_chunk_leaf,
        &ctx,
        0u,
        (uint64_t)chunk_count,
        1u,
        hash_fn,
        out_root);
}

ssz_error_t ssz_mix_in_active_fields(
    const ssz_chunk_t *root,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t active_chunk;

    if ((root == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (active_fields_len > 32u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((active_fields_len != 0u) && (active_fields == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(active_chunk.bytes, 0, SSZ_BYTES_PER_CHUNK);
    if (active_fields_len != 0u)
    {
        memcpy(active_chunk.bytes, active_fields, active_fields_len);
    }

    return ssz_hash_2to1(hash_fn, root, &active_chunk, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_container(
    uint32_t field_count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((codec == NULL) || (codec->root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_internal_validate_active_fields(active_fields, active_fields_len, field_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    ssz_internal_codec_reader_ctx_t ctx = {
        .codec = codec,
        .count = field_count,
    };

    err = ssz_internal_merkleize_progressive_reader(
        ssz_internal_read_codec_leaf,
        &ctx,
        0u,
        field_count,
        1u,
        hash_fn,
        &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_active_fields(&data_root, active_fields, active_fields_len, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;
    uint64_t chunk_count = 0u;
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_size == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((elements == NULL) && (total_bytes != 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_internal_byte_len_to_chunk_count(total_bytes, &chunk_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    ssz_internal_bytes_reader_ctx_t ctx = {
        .bytes = elements,
        .byte_len = total_bytes,
        .chunk_count = chunk_count,
    };

    err = ssz_internal_merkleize_progressive_reader(
        ssz_internal_read_bytes_leaf,
        &ctx,
        0u,
        chunk_count,
        1u,
        hash_fn,
        &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, element_count, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_list_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((codec == NULL) || (codec->root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_internal_codec_reader_ctx_t ctx = {
        .codec = codec,
        .count = element_count,
    };

    ssz_error_t err = ssz_internal_merkleize_progressive_reader(
        ssz_internal_read_codec_leaf,
        &ctx,
        0u,
        element_count,
        1u,
        hash_fn,
        &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, element_count, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_count = 0u;
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!ssz_internal_bits_to_bytes(bit_len, &bitfield_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((bitfield_bytes != 0u) && ((bits_le == NULL) || (bits_le_len < bitfield_bytes)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((bit_len % 8u) != 0u)
    {
        uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
        if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
    }

    ssz_error_t err = ssz_internal_byte_len_to_chunk_count(bitfield_bytes, &chunk_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    ssz_internal_bytes_reader_ctx_t ctx = {
        .bytes = bits_le,
        .byte_len = bitfield_bytes,
        .chunk_count = chunk_count,
    };

    err = ssz_internal_merkleize_progressive_reader(
        ssz_internal_read_bytes_leaf,
        &ctx,
        0u,
        chunk_count,
        1u,
        hash_fn,
        &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, bit_len, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_container_roots(
    const ssz_chunk_t *roots,
    uint32_t count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_internal_validate_active_fields(active_fields, active_fields_len, count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkleize_progressive(roots, count, hash_fn, &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_active_fields(&data_root, active_fields, active_fields_len, hash_fn, out_root);
}

ssz_error_t ssz_hash_tree_root_progressive_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;

    if (out_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (count > (uint64_t)SIZE_MAX)
    {
        return SSZ_ERR_OVERFLOW;
    }

    ssz_error_t err = ssz_merkleize_progressive(roots, (size_t)count, hash_fn, &data_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_length(&data_root, count, hash_fn, out_root);
}
