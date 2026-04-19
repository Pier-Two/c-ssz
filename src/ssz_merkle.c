#include "ssz_merkle.h"

#include <stdint.h>
#include <string.h>

#include "ssz_internal.h"

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

typedef struct ssz_internal_leaf_source_s ssz_internal_leaf_source_t;

typedef struct
{
    const ssz_internal_leaf_source_t *source;
    const uint8_t *active_fields;
    size_t active_fields_len;
} ssz_internal_progressive_container_reader_ctx_t;

typedef ssz_error_t (
    *ssz_internal_custom_leaf_reader_t)(const void *ctx, uint64_t index, ssz_chunk_t *out_leaf);

typedef enum
{
    SSZ_INTERNAL_LEAF_SOURCE_CUSTOM = 0,
    SSZ_INTERNAL_LEAF_SOURCE_CHUNKS,
    SSZ_INTERNAL_LEAF_SOURCE_BYTES,
    SSZ_INTERNAL_LEAF_SOURCE_CODEC
} ssz_internal_leaf_source_kind_t;

struct ssz_internal_leaf_source_s
{
    ssz_internal_leaf_source_kind_t kind;
    ssz_internal_chunk_reader_ctx_t chunk_reader;
    ssz_internal_bytes_reader_ctx_t bytes_reader;
    ssz_internal_codec_reader_ctx_t codec_reader;
    ssz_internal_custom_leaf_reader_t custom_reader;
    const void *custom_ctx;
};

#define SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES  SSZ_MERKLE_SCRATCH_MAX_CHUNKS
#define SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES 64u

static ssz_error_t ssz_internal_read_chunk_leaf(
    const ssz_internal_chunk_reader_ctx_t *reader,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((reader == NULL) || (out_leaf == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((index >= reader->count) || ((reader->chunks == NULL) && (reader->count != 0u)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_leaf = reader->chunks[index];
    }

    return err;
}

static ssz_error_t ssz_internal_read_bytes_leaf(
    const ssz_internal_bytes_reader_ctx_t *reader,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    uint64_t start_u64 = 0u;
    size_t start = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((reader == NULL) || (out_leaf == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (index >= reader->chunk_count)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((reader->byte_len != 0u) && (reader->bytes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (
        ssz_internal_mul_overflow_u64(index, SSZ_BYTES_PER_CHUNK, &start_u64) ||
        !ssz_internal_u64_to_size(start_u64, &start))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        (void)memset(out_leaf->bytes, 0, SSZ_BYTES_PER_CHUNK);
        if (start < reader->byte_len)
        {
            size_t remaining = reader->byte_len - start;
            size_t copy_len = (remaining < SSZ_BYTES_PER_CHUNK) ? remaining : SSZ_BYTES_PER_CHUNK;
            (void)memcpy(out_leaf->bytes, &reader->bytes[start], copy_len);
        }
    }

    return err;
}

static ssz_error_t ssz_internal_read_codec_leaf(
    const ssz_internal_codec_reader_ctx_t *reader,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((reader == NULL) || (out_leaf == NULL) || (reader->codec == NULL) ||
        (reader->codec->root == NULL) || (index >= reader->count))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = reader->codec->root(reader->codec->ctx, index, out_leaf);
    }

    return err;
}

static bool ssz_internal_scratch_is_invalid(const ssz_merkle_scratch_t *scratch)
{
    return (scratch != NULL) && (scratch->chunk_count != 0u) && (scratch->chunks == NULL);
}

static ssz_error_t ssz_internal_get_scratch_chunks(
    const ssz_merkle_scratch_t *scratch,
    size_t required_chunks,
    ssz_chunk_t **out_chunks)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_chunks == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_chunks = NULL;
        if (required_chunks == 0u)
        {
            /* intentionally empty */
        }
        else if (
            (scratch == NULL) || (scratch->chunks == NULL) ||
            (scratch->chunk_count < required_chunks))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else
        {
            *out_chunks = scratch->chunks;
        }
    }

    return err;
}

static ssz_error_t ssz_internal_read_leaf(
    const ssz_internal_leaf_source_t *source,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((source == NULL) || (out_leaf == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        switch (source->kind)
        {
        case SSZ_INTERNAL_LEAF_SOURCE_CHUNKS:
            err = ssz_internal_read_chunk_leaf(&source->chunk_reader, index, out_leaf);
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_BYTES:
            err = ssz_internal_read_bytes_leaf(&source->bytes_reader, index, out_leaf);
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_CODEC:
            err = ssz_internal_read_codec_leaf(&source->codec_reader, index, out_leaf);
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_CUSTOM:
            if (source->custom_reader == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = source->custom_reader(source->custom_ctx, index, out_leaf);
            }
            break;
        default:
            err = SSZ_ERR_INVALID_ARGUMENT;
            break;
        }
    }

    return err;
}

static bool ssz_internal_active_field_bit_is_set(
    const uint8_t *active_fields,
    size_t active_fields_len,
    uint64_t index)
{
    uint64_t byte_index = index / 8u;
    uint8_t bit_mask = (uint8_t)(1u << (index % 8u));

    if ((active_fields == NULL) || (byte_index >= active_fields_len))
    {
        return false;
    }

    return (active_fields[byte_index] & bit_mask) != 0u;
}

static ssz_error_t ssz_internal_progressive_container_slot_count(
    const uint8_t *active_fields,
    size_t active_fields_len,
    uint64_t *out_slot_count)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_slot_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((active_fields == NULL) || (active_fields_len == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        size_t byte_index = active_fields_len - 1u;
        uint8_t byte = active_fields[byte_index];
        uint64_t slot_count = 0u;
        uint8_t bit_index = 0u;

        while ((byte & 0x80u) == 0u)
        {
            byte <<= 1u;
            bit_index++;
        }

        if (ssz_internal_mul_overflow_u64((uint64_t)byte_index, 8u, &slot_count) ||
            ssz_internal_add_overflow_u64(slot_count, (uint64_t)(8u - bit_index), &slot_count))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            *out_slot_count = slot_count;
        }
    }

    return err;
}

static ssz_error_t ssz_internal_progressive_container_read_leaf(
    const void *ctx,
    uint64_t index,
    ssz_chunk_t *out_leaf)
{
    const ssz_internal_progressive_container_reader_ctx_t *reader =
        (const ssz_internal_progressive_container_reader_ctx_t *)ctx;
    uint64_t source_index = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((reader == NULL) || (reader->source == NULL) || (out_leaf == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_active_field_bit_is_set(reader->active_fields, reader->active_fields_len, index))
    {
        (void)memset(out_leaf->bytes, 0, SSZ_BYTES_PER_CHUNK);
    }
    else
    {
        for (uint64_t i = 0u; i < index; i++)
        {
            if (ssz_internal_active_field_bit_is_set(reader->active_fields, reader->active_fields_len, i))
            {
                if (ssz_internal_add_overflow_u64(source_index, 1u, &source_index))
                {
                    err = SSZ_ERR_OVERFLOW;
                    break;
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_read_leaf(reader->source, source_index, out_leaf);
        }
    }

    return err;
}

static void ssz_internal_init_progressive_container_source(
    ssz_internal_leaf_source_t *source,
    const ssz_internal_progressive_container_reader_ctx_t *reader)
{
    (void)memset(source, 0, sizeof(*source));
    source->kind = SSZ_INTERNAL_LEAF_SOURCE_CUSTOM;
    source->custom_reader = ssz_internal_progressive_container_read_leaf;
    source->custom_ctx = reader;
}

static ssz_error_t ssz_internal_validate_leaf_range(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_end)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (source == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        switch (source->kind)
        {
        case SSZ_INTERNAL_LEAF_SOURCE_CHUNKS:
            if (source_end > source->chunk_reader.count)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_BYTES:
            if (source_end > source->bytes_reader.chunk_count)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_CODEC:
            if (source_end > source->codec_reader.count)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            break;
        case SSZ_INTERNAL_LEAF_SOURCE_CUSTOM:
            break;
        default:
            err = SSZ_ERR_INVALID_ARGUMENT;
            break;
        }
    }

    return err;
}

static void ssz_internal_init_chunk_source(
    ssz_internal_leaf_source_t *source,
    const ssz_chunk_t *chunks,
    uint64_t count)
{
    (void)memset(source, 0, sizeof(*source));
    source->kind = SSZ_INTERNAL_LEAF_SOURCE_CHUNKS;
    source->chunk_reader.chunks = chunks;
    source->chunk_reader.count = count;
}

static void ssz_internal_init_bytes_source(
    ssz_internal_leaf_source_t *source,
    const uint8_t *bytes,
    size_t byte_len,
    uint64_t chunk_count)
{
    (void)memset(source, 0, sizeof(*source));
    source->kind = SSZ_INTERNAL_LEAF_SOURCE_BYTES;
    source->bytes_reader.bytes = bytes;
    source->bytes_reader.byte_len = byte_len;
    source->bytes_reader.chunk_count = chunk_count;
}

static void ssz_internal_init_codec_source(
    ssz_internal_leaf_source_t *source,
    const ssz_member_codec_t *codec,
    uint64_t count)
{
    (void)memset(source, 0, sizeof(*source));
    source->kind = SSZ_INTERNAL_LEAF_SOURCE_CODEC;
    source->codec_reader.codec = codec;
    source->codec_reader.count = count;
}

static uint32_t ssz_internal_log2_u64(uint64_t value)
{
    uint32_t depth = 0u;
    uint64_t current_value = value;

    while (current_value > 1u)
    {
        current_value >>= 1u;
        depth++;
    }
    return depth;
}

static bool ssz_internal_count_fits_size(uint64_t count)
{
    return ssz_internal_u64_to_size(count, NULL);
}

static ssz_error_t ssz_internal_build_zero_hashes(
    uint32_t max_depth,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t zero_hashes[64])
{
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(zero_hashes[0].bytes, 0, SSZ_BYTES_PER_CHUNK);

    for (uint32_t depth = 1u; depth <= max_depth; depth++)
    {
        err = ssz_hash_2to1(
            hash_fn,
            &zero_hashes[depth - 1u],
            &zero_hashes[depth - 1u],
            &zero_hashes[depth]);
        if (err != SSZ_SUCCESS)
        {
            break;
        }
    }

    return err;
}

static ssz_error_t ssz_internal_merkleize_reader_fast(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t tree_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t zero_hashes[64],
    ssz_chunk_t *out_root)
{
    size_t tree_size_sz = 0u;
    size_t leaf_count_sz = 0u;
    uint64_t source_end = 0u;
    size_t source_start_sz = 0u;
    ssz_chunk_t *level_storage = NULL;
    ssz_chunk_t stack_storage[SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES];
    ssz_error_t ret = SSZ_SUCCESS;

    if ((source == NULL) || (out_root == NULL))
    {
        ret = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (
        !ssz_internal_u64_to_size(tree_size, &tree_size_sz) ||
        !ssz_internal_u64_to_size(leaf_count, &leaf_count_sz))
    {
        ret = SSZ_ERR_OVERFLOW;
    }
    else if (tree_size_sz == 0u)
    {
        ret = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_add_overflow_u64(source_start, leaf_count, &source_end))
    {
        ret = SSZ_ERR_OVERFLOW;
    }
    else if (!ssz_internal_u64_to_size(source_start, &source_start_sz))
    {
        ret = SSZ_ERR_OVERFLOW;
    }
    else
    {
        ret = ssz_internal_validate_leaf_range(source, source_end);
    }

    if (ret == SSZ_SUCCESS)
    {
        if (tree_size_sz <= SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES)
        {
            uint64_t source_index = source_start;

            for (size_t i = 0u; i < leaf_count_sz; i++)
            {
                ret = ssz_internal_read_leaf(source, source_index, &stack_storage[i]);
                if (ret != SSZ_SUCCESS)
                {
                    break;
                }
                source_index++;
            }
            if (ret == SSZ_SUCCESS)
            {
                size_t width = tree_size_sz;

                for (size_t i = leaf_count_sz; i < tree_size_sz; i++)
                {
                    stack_storage[i] = zero_hashes[0];
                }

                while ((width > 1u) && (ret == SSZ_SUCCESS))
                {
                    size_t pair_count = width >> 1u;

                    ret = ssz_hash_2to1_batch_inplace(hash_fn, stack_storage, pair_count);
                    width = pair_count;
                }

                if (ret == SSZ_SUCCESS)
                {
                    *out_root = stack_storage[0];
                }
            }
        }
        else
        {
            bool initialized_levels = false;
            size_t width;

            if ((leaf_count == tree_size) && (source->kind == SSZ_INTERNAL_LEAF_SOURCE_CHUNKS))
            {
                const ssz_internal_chunk_reader_ctx_t *chunk_reader = &source->chunk_reader;

                if ((chunk_reader->chunks != NULL) && (source_end <= chunk_reader->count))
                {
                    const ssz_chunk_t *source_chunks = &chunk_reader->chunks[source_start_sz];
                    size_t level_storage_cap = tree_size_sz >> 1u;

                    ret =
                        ssz_internal_get_scratch_chunks(scratch, level_storage_cap, &level_storage);
                    if (ret == SSZ_SUCCESS)
                    {
                        ret = ssz_hash_2to1_batch(
                            hash_fn,
                            source_chunks,
                            level_storage_cap,
                            level_storage);
                        if (ret == SSZ_SUCCESS)
                        {
                            width = level_storage_cap;
                            initialized_levels = true;
                        }
                    }
                }
            }

            if ((ret == SSZ_SUCCESS) && !initialized_levels && (leaf_count == tree_size) &&
                (source->kind == SSZ_INTERNAL_LEAF_SOURCE_BYTES))
            {
                const ssz_internal_bytes_reader_ctx_t *bytes_reader = &source->bytes_reader;
                size_t source_offset = 0u;
                size_t copy_len = 0u;

                if ((bytes_reader->bytes != NULL) &&
                    !ssz_internal_mul_overflow_size(
                        source_start_sz,
                        SSZ_BYTES_PER_CHUNK,
                        &source_offset) &&
                    !ssz_internal_mul_overflow_size(tree_size_sz, SSZ_BYTES_PER_CHUNK, &copy_len) &&
                    (source_offset <= bytes_reader->byte_len) &&
                    (copy_len <= (bytes_reader->byte_len - source_offset)))
                {
                    size_t level_storage_cap = tree_size_sz >> 1u;

                    ret =
                        ssz_internal_get_scratch_chunks(scratch, level_storage_cap, &level_storage);
                    if (ret == SSZ_SUCCESS)
                    {
                        ret = ssz_hash_2to1_batch_raw(
                            hash_fn,
                            &bytes_reader->bytes[source_offset],
                            level_storage_cap,
                            level_storage);
                        if (ret == SSZ_SUCCESS)
                        {
                            width = level_storage_cap;
                            initialized_levels = true;
                        }
                    }
                }
            }

            if ((ret == SSZ_SUCCESS) && !initialized_levels)
            {
                size_t level_storage_cap = tree_size_sz;

                ret = ssz_internal_get_scratch_chunks(scratch, level_storage_cap, &level_storage);
                if (ret == SSZ_SUCCESS)
                {
                    uint64_t source_index = source_start;

                    for (size_t i = 0u; i < leaf_count_sz; i++)
                    {
                        ret = ssz_internal_read_leaf(source, source_index, &level_storage[i]);
                        if (ret != SSZ_SUCCESS)
                        {
                            break;
                        }
                        source_index++;
                    }
                    if (ret == SSZ_SUCCESS)
                    {
                        for (size_t i = leaf_count_sz; i < tree_size_sz; i++)
                        {
                            level_storage[i] = zero_hashes[0];
                        }
                        width = tree_size_sz;
                        initialized_levels = true;
                    }
                }
            }

            if ((ret == SSZ_SUCCESS) && initialized_levels)
            {
                while ((width > 1u) && (ret == SSZ_SUCCESS))
                {
                    size_t pair_count = width >> 1u;

                    ret = ssz_hash_2to1_batch_inplace(hash_fn, level_storage, pair_count);
                    width = pair_count;
                }

                if (ret == SSZ_SUCCESS)
                {
                    *out_root = level_storage[0];
                }
            }
        }
    }

    return ret;
}

static ssz_error_t ssz_internal_merkleize_full_range_iter(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_start,
    uint64_t full_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t zero_hashes[64],
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint64_t source_end = 0u;
    uint64_t tile_size = 0u;
    uint64_t tile_count = 0u;
    uint64_t tile_index = 0u;
    uint64_t tile_source_start = 0u;
    uint32_t tile_height = 0u;
    uint32_t full_depth = 0u;
    bool frontier_valid[64];
    ssz_chunk_t frontier[64];
    ssz_chunk_t current;

    (void)memset(frontier_valid, 0, sizeof(frontier_valid));

    if ((source == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((full_size == 0u) || ((full_size & (full_size - 1u)) != 0u))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_add_overflow_u64(source_start, full_size, &source_end))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_internal_validate_leaf_range(source, source_end);
        if (err != SSZ_SUCCESS)
        {
            /* propagate */
        }
        else
        {
            tile_size = full_size;
            if (tile_size > SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES)
            {
                tile_size = SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES;
            }
            else
            {
                /* intentionally empty */
            }

            while (err == SSZ_SUCCESS)
            {
                err = ssz_internal_merkleize_reader_fast(
                    source,
                    source_start,
                    tile_size,
                    tile_size,
                    scratch,
                    hash_fn,
                    zero_hashes,
                    &current);

                if (err == SSZ_SUCCESS)
                {
                    break;
                }

                if ((err == SSZ_ERR_BUFFER_TOO_SMALL) &&
                    (tile_size > SSZ_INTERNAL_STACK_MERKLE_MAX_LEAVES))
                {
                    tile_size >>= 1u;
                    err = SSZ_SUCCESS;
                }
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        tile_count = full_size / tile_size;
        tile_height = ssz_internal_log2_u64(tile_size);
        full_depth = ssz_internal_log2_u64(full_size);
        tile_source_start = source_start;
    }
    else
    {
        /* intentionally empty */
    }

    while ((err == SSZ_SUCCESS) && (tile_index < tile_count))
    {
        if (tile_index != 0u)
        {
            err = ssz_internal_merkleize_reader_fast(
                source,
                tile_source_start,
                tile_size,
                tile_size,
                scratch,
                hash_fn,
                zero_hashes,
                &current);
        }
        else
        {
            /* current already holds the first tile root */
        }

        if (err == SSZ_SUCCESS)
        {
            uint32_t level = tile_height;
            bool placed = false;

            while ((err == SSZ_SUCCESS) && !placed)
            {
                if (!frontier_valid[level])
                {
                    frontier[level] = current;
                    frontier_valid[level] = true;
                    placed = true;
                }
                else
                {
                    err = ssz_hash_2to1(hash_fn, &frontier[level], &current, &current);
                    if (err == SSZ_SUCCESS)
                    {
                        frontier_valid[level] = false;
                        level++;
                    }
                    else
                    {
                        /* propagate */
                    }
                }
            }
        }
        else
        {
            /* propagate */
        }

        if (err == SSZ_SUCCESS)
        {
            tile_index++;
            if (tile_index < tile_count)
            {
                if (ssz_internal_add_overflow_u64(tile_source_start, tile_size, &tile_source_start))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    /* intentionally empty */
                }
            }
            else
            {
                /* intentionally empty */
            }
        }
        else
        {
            /* propagate */
        }
    }

    if (err == SSZ_SUCCESS)
    {
        if (!frontier_valid[full_depth])
        {
            err = SSZ_ERR_HASH_FAILURE;
        }
        else
        {
            *out_root = frontier[full_depth];
        }
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

static ssz_error_t ssz_internal_merkleize_subtree_iter(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t node_start,
    uint64_t subtree_size,
    uint32_t depth,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t zero_hashes[64],
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint64_t subtree_mid = 0u;
    uint64_t subtree_source_start = 0u;

    if ((source == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (node_start >= leaf_count)
    {
        *out_root = zero_hashes[depth];
    }
    else if (
        (subtree_size > 1u) &&
        ssz_internal_add_overflow_u64(node_start, (subtree_size >> 1u), &subtree_mid))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_add_overflow_u64(source_start, node_start, &subtree_source_start))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        uint64_t occupied = leaf_count - node_start;

        if (occupied > subtree_size)
        {
            occupied = subtree_size;
        }
        else
        {
            /* intentionally empty */
        }

        if (subtree_size == 1u)
        {
            err = ssz_internal_read_leaf(source, subtree_source_start, out_root);
        }
        else if (occupied == subtree_size)
        {
            err = ssz_internal_merkleize_full_range_iter(
                source,
                subtree_source_start,
                subtree_size,
                scratch,
                hash_fn,
                zero_hashes,
                out_root);
        }
        else
        {
            uint32_t acc_height = 0u;
            bool acc_valid = false;
            ssz_chunk_t acc;
            ssz_chunk_t block_root;
            uint64_t remaining = occupied;

            while ((err == SSZ_SUCCESS) && (remaining != 0u))
            {
                uint64_t reduced = remaining & (remaining - 1u);
                uint64_t block_size = remaining ^ reduced;
                uint64_t block_start = remaining - block_size;
                uint64_t block_source_start;
                uint32_t block_height = ssz_internal_log2_u64(block_size);

                if (ssz_internal_add_overflow_u64(
                        subtree_source_start,
                        block_start,
                        &block_source_start))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    err = ssz_internal_merkleize_full_range_iter(
                        source,
                        block_source_start,
                        block_size,
                        scratch,
                        hash_fn,
                        zero_hashes,
                        &block_root);
                }

                if (err == SSZ_SUCCESS)
                {
                    if (!acc_valid)
                    {
                        acc = block_root;
                        acc_height = block_height;
                        acc_valid = true;
                    }
                    else
                    {
                        while ((err == SSZ_SUCCESS) && (acc_height < block_height))
                        {
                            err = ssz_hash_2to1(hash_fn, &acc, &zero_hashes[acc_height], &acc);

                            if (err == SSZ_SUCCESS)
                            {
                                acc_height++;
                            }
                            else
                            {
                                /* propagate */
                            }
                        }

                        if (err == SSZ_SUCCESS)
                        {
                            err = ssz_hash_2to1(hash_fn, &block_root, &acc, &acc);
                            if (err == SSZ_SUCCESS)
                            {
                                acc_height++;
                            }
                            else
                            {
                                /* propagate */
                            }
                        }
                        else
                        {
                            /* propagate */
                        }
                    }
                }
                else
                {
                    /* propagate */
                }

                if (err == SSZ_SUCCESS)
                {
                    remaining = block_start;
                }
                else
                {
                    /* propagate */
                }
            }

            while ((err == SSZ_SUCCESS) && (acc_height < depth))
            {
                err = ssz_hash_2to1(hash_fn, &acc, &zero_hashes[acc_height], &acc);

                if (err == SSZ_SUCCESS)
                {
                    acc_height++;
                }
                else
                {
                    /* propagate */
                }
            }

            if (err == SSZ_SUCCESS)
            {
                *out_root = acc;
            }
            else
            {
                /* propagate */
            }
        }
    }

    return err;
}

static ssz_error_t ssz_internal_merkleize_reader(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t zero_hashes_buf[64];
    const ssz_chunk_t *zero_hashes = NULL;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((source == NULL) || (out_root == NULL) || ssz_internal_scratch_is_invalid(scratch))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        uint64_t effective_width = 0u;

        resolved_hash_fn = ssz_internal_resolve_hash_fn(hash_fn);
        if ((resolved_hash_fn == NULL) || (resolved_hash_fn->hash == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (limit == SSZ_NO_LIMIT)
        {
            effective_width = leaf_count;
        }
        else if (leaf_count > limit)
        {
            err = SSZ_ERR_LIMIT_EXCEEDED;
        }
        else
        {
            effective_width = limit;
        }

        if (err == SSZ_SUCCESS)
        {
            uint64_t tree_size = ssz_next_pow_of_two(effective_width);

            if (tree_size == 0u)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                uint32_t depth = ssz_internal_log2_u64(tree_size);

                if (resolved_hash_fn == ssz_hash_default())
                {
                    zero_hashes = ssz_hash_default_zero_hashes();
                }
                else
                {
                    err = ssz_internal_build_zero_hashes(depth, resolved_hash_fn, zero_hashes_buf);
                    if (err == SSZ_SUCCESS)
                    {
                        zero_hashes = zero_hashes_buf;
                    }
                }
                if ((err == SSZ_SUCCESS) && (zero_hashes == NULL))
                {
                    err = SSZ_ERR_HASH_FAILURE;
                }

                if (err == SSZ_SUCCESS)
                {
                    if (leaf_count == 0u)
                    {
                        *out_root = zero_hashes[depth];
                    }
                    else if (
                        (leaf_count == tree_size) &&
                        (tree_size <= SSZ_INTERNAL_FAST_MERKLE_MAX_LEAVES))
                    {
                        err = ssz_internal_merkleize_reader_fast(
                            source,
                            source_start,
                            leaf_count,
                            tree_size,
                            scratch,
                            resolved_hash_fn,
                            zero_hashes,
                            out_root);
                        if (err == SSZ_ERR_BUFFER_TOO_SMALL)
                        {
                            err = ssz_internal_merkleize_subtree_iter(
                                source,
                                source_start,
                                leaf_count,
                                0u,
                                tree_size,
                                depth,
                                scratch,
                                resolved_hash_fn,
                                zero_hashes,
                                out_root);
                        }
                    }
                    else
                    {
                        err = ssz_internal_merkleize_subtree_iter(
                            source,
                            source_start,
                            leaf_count,
                            0u,
                            tree_size,
                            depth,
                            scratch,
                            resolved_hash_fn,
                            zero_hashes,
                            out_root);
                    }
                }
            }
        }
    }

    return err;
}

static ssz_error_t ssz_internal_merkleize_progressive_reader_iter(
    const ssz_internal_leaf_source_t *source,
    uint64_t source_start,
    uint64_t leaf_count,
    uint64_t num_leaves,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint64_t absolute_start = 0u;
    uint64_t next_absolute_start = 0u;
    uint64_t next_segment_width = 0u;
    ssz_chunk_t acc;
    ssz_chunk_t segment_root;

    if ((source == NULL) || (out_root == NULL) || (num_leaves == 0u))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (leaf_count == 0u)
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
    }
    else
    {
        uint64_t segment_start = 0u;
        uint64_t segment_count = leaf_count;
        uint64_t segment_width = num_leaves;

        while ((err == SSZ_SUCCESS) && (segment_count > segment_width))
        {
            segment_count -= segment_width;

            if (ssz_internal_add_overflow_u64(segment_start, segment_width, &segment_start))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else if (ssz_internal_mul_overflow_u64(segment_width, 4u, &segment_width))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                /* intentionally empty */
            }
        }

        if (err == SSZ_SUCCESS)
        {
            (void)memset(acc.bytes, 0, SSZ_BYTES_PER_CHUNK);

            while (err == SSZ_SUCCESS)
            {
                if (ssz_internal_add_overflow_u64(source_start, segment_start, &absolute_start))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    err = ssz_internal_merkleize_reader(
                        source,
                        absolute_start,
                        segment_count,
                        segment_width,
                        scratch,
                        hash_fn,
                        &segment_root);
                }

                if (err == SSZ_SUCCESS)
                {
                    if (ssz_internal_add_overflow_u64(
                            absolute_start,
                            segment_count,
                            &next_absolute_start))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                    else if (ssz_internal_mul_overflow_u64(segment_width, 4u, &next_segment_width))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                    else
                    {
                        /* intentionally empty */
                    }
                }
                else
                {
                    /* propagate */
                }

                if (err == SSZ_SUCCESS)
                {
                    err = ssz_hash_2to1(hash_fn, &segment_root, &acc, &acc);
                }
                else
                {
                    /* propagate */
                }

                if (err == SSZ_SUCCESS)
                {
                    if (segment_start == 0u)
                    {
                        break;
                    }

                    {
                        uint64_t previous_width = segment_width / 4u;

                        if (segment_start < previous_width)
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                        else
                        {
                            segment_start -= previous_width;
                            segment_count = previous_width;
                            segment_width = previous_width;
                        }
                    }
                }
                else
                {
                    /* propagate */
                }
            }

            if (err == SSZ_SUCCESS)
            {
                *out_root = acc;
            }
            else
            {
                /* propagate */
            }
        }
        else
        {
            /* propagate */
        }
    }

    return err;
}

static ssz_error_t ssz_internal_byte_len_to_chunk_count(size_t byte_len, uint64_t *out_chunk_count)
{
    size_t chunk_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_chunk_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (byte_len == 0u)
    {
        *out_chunk_count = 0u;
    }
    else if (ssz_internal_add_overflow_size(byte_len, SSZ_BYTES_PER_CHUNK - 1u, &chunk_count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        chunk_count /= SSZ_BYTES_PER_CHUNK;
        *out_chunk_count = (uint64_t)chunk_count;
    }

    return err;
}

static ssz_error_t ssz_internal_merkleize_packed_bytes(
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t limit_chunks,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    uint64_t chunk_count = 0u;
    ssz_error_t err = ssz_internal_byte_len_to_chunk_count(bytes_len, &chunk_count);

    if (err != SSZ_SUCCESS)
    {
        /* intentionally empty */
    }
    else if ((bytes_len != 0u) && (bytes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_leaf_source_t source;

        ssz_internal_init_bytes_source(&source, bytes, bytes_len, chunk_count);

        err = ssz_internal_merkleize_reader(
            &source,
            0u,
            chunk_count,
            limit_chunks,
            scratch,
            hash_fn,
            out_root);
    }

    return err;
}

static ssz_error_t ssz_internal_validate_active_fields(
    const uint8_t *active_fields,
    size_t active_fields_len,
    uint32_t field_count)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (field_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((active_fields == NULL) || (active_fields_len == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (active_fields_len > 32u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (active_fields[active_fields_len - 1u] == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        size_t one_bits = 0u;

        for (size_t i = 0u; i < active_fields_len; i++)
        {
            one_bits += ssz_internal_count_bits_u8(active_fields[i]);
        }
        if (one_bits != field_count)
        {
            err = SSZ_ERR_SCHEMA_INVALID;
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint8(uint8_t value, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        out_root->bytes[0] = value;
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint16(uint16_t value, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        ssz_internal_write_u16_le(out_root->bytes, value);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint32(uint32_t value, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        ssz_internal_write_u32_le(out_root->bytes, value);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint64(uint64_t value, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        ssz_internal_write_u64_le(out_root->bytes, value);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint128(
    const uint8_t *value,
    size_t value_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (value_len != 16u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        (void)memset(out_root->bytes, 0, SSZ_BYTES_PER_CHUNK);
        (void)memcpy(out_root->bytes, value, 16u);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_uint256(
    const uint8_t *value,
    size_t value_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (value_len != SSZ_BYTES_PER_CHUNK)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        (void)memcpy(out_root->bytes, value, SSZ_BYTES_PER_CHUNK);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_boolean(uint8_t value, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (value > 1u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        err = ssz_hash_tree_root_uint8(value, out_root);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (bit_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_bits_to_bytes(bit_count, &bitfield_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((bits_le == NULL) || (bits_le_len < bitfield_bytes))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ((bit_count % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
            if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if ((err == SSZ_SUCCESS) && ssz_internal_add_overflow_u64(bit_count, 255u, &chunk_limit))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        if (err == SSZ_SUCCESS)
        {
            chunk_limit /= 256u;
            err = ssz_internal_merkleize_packed_bytes(
                bits_le,
                bitfield_bytes,
                chunk_limit,
                scratch,
                hash_fn,
                out_root);
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_chunk_t data_root;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((bit_limit != SSZ_NO_LIMIT) && (bit_len > bit_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_bits_to_bytes(bit_len, &bitfield_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((bitfield_bytes != 0u) && ((bits_le == NULL) || (bits_le_len < bitfield_bytes)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ((bit_len % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
            if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if ((err == SSZ_SUCCESS) && (bit_limit != SSZ_NO_LIMIT))
        {
            if (ssz_internal_add_overflow_u64(bit_limit, 255u, &chunk_limit))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                chunk_limit /= 256u;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_merkleize_packed_bytes(
                bits_le,
                bitfield_bytes,
                chunk_limit,
                scratch,
                hash_fn,
                &data_root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_length_u64(&data_root, bit_len, hash_fn, out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((elements == NULL) && (total_bytes != 0u))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_merkleize_packed_bytes(
            elements,
            total_bytes,
            SSZ_NO_LIMIT,
            scratch,
            hash_fn,
            out_root);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_vector_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_init_codec_source(&source, codec, element_count);

        err = ssz_internal_merkleize_reader(
            &source,
            0u,
            element_count,
            SSZ_NO_LIMIT,
            scratch,
            hash_fn,
            out_root);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_vector_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_count_fits_size(count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_merkleize(roots, (size_t)count, SSZ_NO_LIMIT, scratch, hash_fn, out_root);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_chunk_t data_root;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((elements == NULL) && (total_bytes != 0u))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (element_limit != SSZ_NO_LIMIT)
        {
            uint64_t limit_bytes = 0u;

            if (ssz_internal_mul_overflow_u64(
                    element_limit,
                    (uint64_t)element_size,
                    &limit_bytes) ||
                ssz_internal_add_overflow_u64(limit_bytes, SSZ_BYTES_PER_CHUNK - 1u, &chunk_limit))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                chunk_limit /= SSZ_BYTES_PER_CHUNK;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_merkleize_packed_bytes(
                elements,
                total_bytes,
                chunk_limit,
                scratch,
                hash_fn,
                &data_root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_length_u64(&data_root, element_count, hash_fn, out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_list_composite(
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if ((codec == NULL) || (codec->root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_init_codec_source(&source, codec, element_count);

        err = ssz_internal_merkleize_reader(
            &source,
            0u,
            element_count,
            element_limit,
            scratch,
            hash_fn,
            &data_root);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_mix_in_length_u64(&data_root, element_count, hash_fn, out_root);
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    uint64_t limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((limit != SSZ_NO_LIMIT) && (count > limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_count_fits_size(count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_merkleize(roots, (size_t)count, limit, scratch, hash_fn, &data_root);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_mix_in_length_u64(&data_root, count, hash_fn, out_root);
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_union(
    uint8_t selector,
    bool has_none,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t value_root;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (has_none && (selector == 0u))
    {
        (void)memset(value_root.bytes, 0, SSZ_BYTES_PER_CHUNK);
    }
    else
    {
        if ((codec == NULL) || (codec->root == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = codec->root(codec->ctx, selector, &value_root);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_mix_in_selector(&value_root, selector, hash_fn, out_root);
    }

    return err;
}

ssz_error_t ssz_merkleize(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    uint64_t limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((chunk_count != 0u) && (chunks == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_init_chunk_source(&source, chunks, (uint64_t)chunk_count);
        err = ssz_internal_merkleize_reader(
            &source,
            0u,
            (uint64_t)chunk_count,
            limit,
            scratch,
            hash_fn,
            out_root);
    }

    return err;
}

ssz_error_t ssz_mix_in_length(
    const ssz_chunk_t *root,
    const uint8_t *length,
    size_t length_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t length_chunk;
    ssz_error_t err = SSZ_SUCCESS;

    if ((root == NULL) || (length == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (length_len != SSZ_BYTES_PER_CHUNK)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        (void)memcpy(length_chunk.bytes, length, SSZ_BYTES_PER_CHUNK);
        err = ssz_hash_2to1(hash_fn, root, &length_chunk, out_root);
    }

    return err;
}

ssz_error_t ssz_mix_in_length_u64(
    const ssz_chunk_t *root,
    uint64_t length,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    uint8_t length_bytes[32];
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(length_bytes, 0, sizeof(length_bytes));
    ssz_internal_write_u64_le(length_bytes, length);
    err = ssz_mix_in_length(root, length_bytes, sizeof(length_bytes), hash_fn, out_root);

    return err;
}

ssz_error_t ssz_mix_in_selector(
    const ssz_chunk_t *root,
    uint8_t selector,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t selector_chunk;
    ssz_error_t err = SSZ_SUCCESS;

    if ((root == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(selector_chunk.bytes, 0, SSZ_BYTES_PER_CHUNK);
        selector_chunk.bytes[0] = selector;
        err = ssz_hash_2to1(hash_fn, root, &selector_chunk, out_root);
    }

    return err;
}

ssz_error_t ssz_merkleize_progressive(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((chunk_count != 0u) && (chunks == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_init_chunk_source(&source, chunks, (uint64_t)chunk_count);
        err = ssz_internal_merkleize_progressive_reader_iter(
            &source,
            0u,
            (uint64_t)chunk_count,
            1u,
            scratch,
            hash_fn,
            out_root);
    }

    return err;
}

ssz_error_t ssz_mix_in_active_fields(
    const ssz_chunk_t *root,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t active_chunk;
    ssz_error_t err = SSZ_SUCCESS;

    if ((root == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (active_fields_len > 32u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((active_fields_len != 0u) && (active_fields == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(active_chunk.bytes, 0, SSZ_BYTES_PER_CHUNK);
        if (active_fields_len != 0u)
        {
            (void)memcpy(active_chunk.bytes, active_fields, active_fields_len);
        }
        err = ssz_hash_2to1(hash_fn, root, &active_chunk, out_root);
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_container(
    uint32_t field_count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_internal_leaf_source_t sparse_source;
    ssz_internal_progressive_container_reader_ctx_t reader;
    uint64_t slot_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((codec == NULL) || (codec->root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_validate_active_fields(active_fields, active_fields_len, field_count);
        if (err == SSZ_SUCCESS)
        {
            ssz_internal_init_codec_source(&source, codec, field_count);
            reader.source = &source;
            reader.active_fields = active_fields;
            reader.active_fields_len = active_fields_len;
            ssz_internal_init_progressive_container_source(&sparse_source, &reader);

            err = ssz_internal_progressive_container_slot_count(
                active_fields,
                active_fields_len,
                &slot_count);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_internal_merkleize_progressive_reader_iter(
                    &sparse_source,
                    0u,
                    slot_count,
                    1u,
                    scratch,
                    hash_fn,
                    &data_root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_active_fields(
                    &data_root,
                    active_fields,
                    active_fields_len,
                    hash_fn,
                    out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t total_bytes = 0u;
    uint64_t chunk_count = 0u;
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &total_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((elements == NULL) && (total_bytes != 0u))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_byte_len_to_chunk_count(total_bytes, &chunk_count);
        if (err == SSZ_SUCCESS)
        {
            ssz_internal_init_bytes_source(&source, elements, total_bytes, chunk_count);

            err = ssz_internal_merkleize_progressive_reader_iter(
                &source,
                0u,
                chunk_count,
                1u,
                scratch,
                hash_fn,
                &data_root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_length_u64(&data_root, element_count, hash_fn, out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_list_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((codec == NULL) || (codec->root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_init_codec_source(&source, codec, element_count);

        err = ssz_internal_merkleize_progressive_reader_iter(
            &source,
            0u,
            element_count,
            1u,
            scratch,
            hash_fn,
            &data_root);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_mix_in_length_u64(&data_root, element_count, hash_fn, out_root);
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_count = 0u;
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_bits_to_bytes(bit_len, &bitfield_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((bitfield_bytes != 0u) && ((bits_le == NULL) || (bits_le_len < bitfield_bytes)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ((bit_len % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
            if ((bits_le[bitfield_bytes - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_byte_len_to_chunk_count(bitfield_bytes, &chunk_count);
        }
        if (err == SSZ_SUCCESS)
        {
            ssz_internal_init_bytes_source(&source, bits_le, bitfield_bytes, chunk_count);

            err = ssz_internal_merkleize_progressive_reader_iter(
                &source,
                0u,
                chunk_count,
                1u,
                scratch,
                hash_fn,
                &data_root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_length_u64(&data_root, bit_len, hash_fn, out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_container_roots(
    const ssz_chunk_t *roots,
    uint32_t count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_internal_leaf_source_t source;
    ssz_internal_leaf_source_t sparse_source;
    ssz_internal_progressive_container_reader_ctx_t reader;
    uint64_t slot_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_validate_active_fields(active_fields, active_fields_len, count);
        if (err == SSZ_SUCCESS)
        {
            ssz_internal_init_chunk_source(&source, roots, count);
            reader.source = &source;
            reader.active_fields = active_fields;
            reader.active_fields_len = active_fields_len;
            ssz_internal_init_progressive_container_source(&sparse_source, &reader);

            err = ssz_internal_progressive_container_slot_count(
                active_fields,
                active_fields_len,
                &slot_count);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_internal_merkleize_progressive_reader_iter(
                    &sparse_source,
                    0u,
                    slot_count,
                    1u,
                    scratch,
                    hash_fn,
                    &data_root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_mix_in_active_fields(
                    &data_root,
                    active_fields,
                    active_fields_len,
                    hash_fn,
                    out_root);
            }
        }
    }

    return err;
}

ssz_error_t ssz_hash_tree_root_progressive_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    ssz_chunk_t data_root;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_root == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_count_fits_size(count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_merkleize_progressive(roots, (size_t)count, scratch, hash_fn, &data_root);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_mix_in_length_u64(&data_root, count, hash_fn, out_root);
        }
    }

    return err;
}
