#include "ssz_merkle_cache.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ssz_hash.h"
#include "ssz_internal.h"
#include "ssz_merkle.h"

typedef struct
{
    uint64_t *bits;
    size_t *word_idx;
    size_t *word_count;
    size_t word_capacity;
} ssz_merkle_cache_dirty_set_t;

static bool ssz_merkle_cache_internal_struct_size_valid(size_t actual, size_t expected)
{
    return actual >= expected;
}

static bool ssz_merkle_cache_internal_pointer_available(const void *ptr, size_t count)
{
    bool available = true;

    if ((count != 0u) && (ptr == NULL))
    {
        available = false;
    }
    else
    {
        /* intentionally empty */
    }

    return available;
}

static bool ssz_merkle_cache_internal_chunk_buffer_aligned(
    const ssz_chunk_t *ptr,
    size_t chunk_count)
{
    return ssz_internal_validate_chunk_array(ptr, chunk_count) == SSZ_SUCCESS;
}

static bool ssz_merkle_cache_internal_cache_is_bound(const ssz_merkle_cache_t *cache)
{
    bool is_bound = false;

    if ((cache != NULL) &&
        ssz_merkle_cache_internal_struct_size_valid(cache->struct_size, sizeof(*cache)) &&
        (cache->nodes != NULL) && (cache->zero_hashes != NULL) &&
        (ssz_internal_validate_chunk_pointer(cache->nodes) == SSZ_SUCCESS) &&
        (ssz_internal_validate_chunk_pointer(cache->zero_hashes) == SSZ_SUCCESS) &&
        ssz_merkle_cache_internal_chunk_buffer_aligned(
            cache->gather_pairs,
            cache->gather_pair_capacity * 2u) &&
        ((cache->gather_hashes == NULL) ||
         (ssz_internal_validate_chunk_pointer(cache->gather_hashes) == SSZ_SUCCESS)))
    {
        is_bound = true;
    }
    else
    {
        is_bound = false;
    }

    return is_bound;
}

static bool ssz_merkle_cache_internal_storage_has_tokens(const ssz_merkle_cache_storage_t *storage)
{
    bool has_tokens = false;

    if (storage != NULL)
    {
        has_tokens = (storage->token_values != NULL) || (storage->token_values_count != 0u) ||
                     (storage->token_valid_bits != NULL) || (storage->token_valid_words != 0u);
    }
    else
    {
        has_tokens = false;
    }

    return has_tokens;
}

static bool ssz_merkle_cache_internal_is_power_of_two_u64(uint64_t value)
{
    bool is_power = false;

    if ((value != 0u) && ((value & (value - 1u)) == 0u))
    {
        is_power = true;
    }
    else
    {
        is_power = false;
    }

    return is_power;
}

static uint32_t ssz_merkle_cache_internal_log2_u64(uint64_t value)
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

static unsigned ssz_merkle_cache_internal_ctz_u64(uint64_t value)
{
    unsigned count = 0u;
    uint64_t remaining = value;

    while ((remaining & 1u) == 0u)
    {
        remaining >>= 1u;
        count++;
    }
    return count;
}

static void ssz_merkle_cache_internal_sort_size_t_asc(size_t *arr, size_t count)
{
    for (size_t i = 1u; i < count; i++)
    {
        size_t key = arr[i];
        size_t j = i;

        while ((j > 0u) && (arr[j - 1u] > key))
        {
            arr[j] = arr[j - 1u];
            j--;
        }

        arr[j] = key;
    }
}

static ssz_error_t ssz_merkle_cache_internal_leaf_words_for_capacity(
    uint64_t leaf_capacity,
    size_t *out_words)
{
    uint64_t words_u64 = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_words == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_add_overflow_u64(leaf_capacity, 63u, &words_u64))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        words_u64 /= 64u;
        if (!ssz_internal_u64_to_size(words_u64, out_words))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_parent_dirty_words_for_capacity(
    uint64_t mutable_leaf_capacity,
    size_t *out_words)
{
    uint64_t words_u64 = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_words == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (mutable_leaf_capacity <= 1u)
    {
        *out_words = 0u;
    }
    else if (ssz_internal_add_overflow_u64(mutable_leaf_capacity, 127u, &words_u64))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        words_u64 /= 128u;
        if (!ssz_internal_u64_to_size(words_u64, out_words))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_gather_pair_capacity_for_capacity(
    uint64_t mutable_leaf_capacity,
    size_t *out_pairs)
{
    uint64_t pairs_u64 = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_pairs == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (mutable_leaf_capacity <= 1u)
    {
        *out_pairs = 0u;
    }
    else if (ssz_internal_add_overflow_u64(mutable_leaf_capacity, 3u, &pairs_u64))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        pairs_u64 /= 4u;
        if (!ssz_internal_u64_to_size(pairs_u64, out_pairs))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_build_zero_hashes(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t out_zero_hashes[64])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_zero_hashes == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_zero_hashes[0].bytes, 0, SSZ_BYTES_PER_CHUNK);
        for (size_t depth = 1u; (depth < 64u) && (err == SSZ_SUCCESS); depth++)
        {
            err = ssz_hash_2to1(
                hash_fn,
                &out_zero_hashes[depth - 1u],
                &out_zero_hashes[depth - 1u],
                &out_zero_hashes[depth]);
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_compute_level_offsets(
    uint64_t leaf_capacity,
    uint32_t depth,
    size_t out_offsets[64])
{
    size_t running = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_offsets == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        uint64_t width = leaf_capacity;

        for (uint32_t level = 0u; (level <= depth) && (err == SSZ_SUCCESS); level++)
        {
            size_t width_sz = 0u;

            out_offsets[level] = running;
            if (!ssz_internal_u64_to_size(width, &width_sz))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else if (ssz_internal_add_overflow_size(running, width_sz, &running))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                width >>= 1u;
            }
        }
    }

    return err;
}

static void ssz_merkle_cache_internal_fill_zero_tree(ssz_merkle_cache_t *cache)
{
    uint64_t width = cache->leaf_capacity;
    bool stop = false;

    for (uint32_t level = 0u; (level <= cache->depth) && !stop; level++)
    {
        size_t width_sz = 0u;

        if (!ssz_internal_u64_to_size(width, &width_sz))
        {
            stop = true;
        }
        else
        {
            ssz_chunk_t *level_nodes = &cache->nodes[cache->level_offsets[level]];
            for (size_t i = 0u; i < width_sz; i++)
            {
                level_nodes[i] = cache->zero_hashes[level];
            }
            width >>= 1u;
        }
    }
}

static void ssz_merkle_cache_internal_bind_dirty_set(
    ssz_merkle_cache_dirty_set_t *set,
    uint64_t *bits,
    size_t *word_idx,
    size_t *word_count,
    size_t word_capacity)
{
    if (set != NULL)
    {
        set->bits = bits;
        set->word_idx = word_idx;
        set->word_count = word_count;
        set->word_capacity = word_capacity;
    }
    else
    {
        /* intentionally empty */
    }
}

static void ssz_merkle_cache_internal_clear_dirty(
    uint64_t *bits,
    const size_t *word_idx,
    size_t *word_count)
{
    if ((bits != NULL) && (word_idx != NULL) && (word_count != NULL))
    {
        for (size_t i = 0u; i < *word_count; i++)
        {
            bits[word_idx[i]] = 0u;
        }
        *word_count = 0u;
    }
    else
    {
        /* intentionally empty */
    }
}

static void ssz_merkle_cache_internal_dirty_set_clear(ssz_merkle_cache_dirty_set_t *set)
{
    if ((set != NULL) && (set->word_count != NULL))
    {
        ssz_merkle_cache_internal_clear_dirty(set->bits, set->word_idx, set->word_count);
    }
    else
    {
        /* intentionally empty */
    }
}

static ssz_error_t ssz_merkle_cache_internal_dirty_mark_bit(
    uint64_t *bits,
    size_t *word_idx,
    size_t *word_count,
    size_t word_capacity,
    uint64_t bit_index)
{
    size_t word = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((bits == NULL) || (word_idx == NULL) || (word_count == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_u64_to_size(bit_index >> 6u, &word))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (word >= word_capacity)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        uint64_t mask = UINT64_C(1) << (bit_index & 63u);
        uint64_t prior = bits[word];

        if ((prior & mask) != 0u)
        {
            /* already marked */
        }
        else
        {
            if (prior == 0u)
            {
                if (*word_count >= word_capacity)
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    word_idx[*word_count] = word;
                    (*word_count)++;
                }
            }

            if (err == SSZ_SUCCESS)
            {
                bits[word] = prior | mask;
            }
            else
            {
                /* intentionally empty */
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_dirty_set_mark(
    ssz_merkle_cache_dirty_set_t *set,
    uint64_t bit_index)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((set == NULL) || (set->word_count == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_dirty_mark_bit(
            set->bits,
            set->word_idx,
            set->word_count,
            set->word_capacity,
            bit_index);
    }

    return err;
}

static void ssz_merkle_cache_internal_invalidate_data_root(ssz_merkle_cache_t *cache)
{
    cache->data_root_valid = false;
    cache->final_root_valid = false;
}

static void ssz_merkle_cache_internal_invalidate_final_root(ssz_merkle_cache_t *cache)
{
    cache->final_root_valid = false;
}

static bool ssz_merkle_cache_internal_token_valid_get(
    const ssz_merkle_cache_t *cache,
    uint64_t index)
{
    size_t word = 0u;
    bool is_valid = false;

    if ((cache == NULL) || !cache->token_storage_ready || (cache->token_valid_bits == NULL))
    {
        is_valid = false;
    }
    else if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        is_valid = false;
    }
    else if (word >= cache->token_word_capacity)
    {
        is_valid = false;
    }
    else
    {
        uint64_t mask = UINT64_C(1) << (index & 63u);

        is_valid = (cache->token_valid_bits[word] & mask) != 0u;
    }

    return is_valid;
}

static void ssz_merkle_cache_internal_token_valid_set(
    ssz_merkle_cache_t *cache,
    uint64_t index,
    bool value)
{
    size_t word = 0u;

    if ((cache == NULL) || !cache->token_storage_ready || (cache->token_valid_bits == NULL))
    {
        /* intentionally empty */
    }
    else if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        /* intentionally empty */
    }
    else if (word >= cache->token_word_capacity)
    {
        /* intentionally empty */
    }
    else
    {
        uint64_t mask = UINT64_C(1) << (index & 63u);

        if (value)
        {
            cache->token_valid_bits[word] |= mask;
        }
        else
        {
            cache->token_valid_bits[word] &= ~mask;
        }
    }
}

static void ssz_merkle_cache_internal_token_valid_clear_range(
    ssz_merkle_cache_t *cache,
    uint64_t start,
    uint64_t count)
{
    uint64_t end = 0u;

    if ((cache == NULL) || !cache->token_storage_ready)
    {
        /* intentionally empty */
    }
    else if (count == 0u)
    {
        /* intentionally empty */
    }
    else if (ssz_internal_add_overflow_u64(start, count, &end))
    {
        /* intentionally empty */
    }
    else
    {
        for (uint64_t i = start; i < end; i++)
        {
            ssz_merkle_cache_internal_token_valid_set(cache, i, false);
        }
    }
}

static ssz_error_t ssz_merkle_cache_internal_mark_leaf_dirty(
    ssz_merkle_cache_t *cache,
    uint64_t leaf_index)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_dirty_mark_bit(
            cache->leaf_dirty_bits,
            cache->leaf_dirty_word_idx,
            &cache->leaf_dirty_word_count,
            cache->leaf_dirty_word_capacity,
            leaf_index);
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_set_leaf(
    ssz_merkle_cache_t *cache,
    uint64_t leaf_index,
    const ssz_chunk_t *leaf,
    bool force_dirty)
{
    size_t leaf_index_sz = 0u;
    ssz_chunk_t *leaf_slot = NULL;
    bool changed = false;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (leaf == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_validate_chunk_pointer(leaf) != SSZ_SUCCESS)
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else if (leaf_index >= cache->mutable_leaf_capacity)
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_u64_to_size(leaf_index, &leaf_index_sz))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        leaf_slot = &cache->nodes[cache->level_offsets[0] + leaf_index_sz];
        changed = (memcmp(leaf_slot->bytes, leaf->bytes, SSZ_BYTES_PER_CHUNK) != 0);
        if (changed)
        {
            *leaf_slot = *leaf;
        }
        else
        {
            /* intentionally empty */
        }
    }

    if ((err == SSZ_SUCCESS) && (changed || force_dirty))
    {
        err = ssz_merkle_cache_internal_mark_leaf_dirty(cache, leaf_index);
        if (err == SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
        else
        {
            /* propagate error */
        }
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_byte_len_to_chunk_count(
    size_t byte_len,
    uint64_t *out_chunk_count)
{
    size_t rounded = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_chunk_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (byte_len == 0u)
    {
        *out_chunk_count = 0u;
    }
    else if (ssz_internal_add_overflow_size(byte_len, SSZ_BYTES_PER_CHUNK - 1u, &rounded))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        rounded /= SSZ_BYTES_PER_CHUNK;
        *out_chunk_count = (uint64_t)rounded;
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_effective_tree_depth(
    const ssz_merkle_cache_t *cache,
    uint32_t *out_depth)
{
    uint64_t width = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (out_depth == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (cache->leaf_limit == SSZ_NO_LIMIT)
    {
        width = cache->leaf_count;
    }
    else if (cache->leaf_count > cache->leaf_limit)
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        width = cache->leaf_limit;
    }

    if (err == SSZ_SUCCESS)
    {
        uint64_t tree_size = ssz_next_pow_of_two(width);

        if (tree_size == 0u)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            *out_depth = ssz_merkle_cache_internal_log2_u64(tree_size);
        }
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_refresh_cached_data_root(ssz_merkle_cache_t *cache)
{
    uint32_t data_depth = 0u;
    ssz_error_t err = ssz_merkle_cache_internal_effective_tree_depth(cache, &data_depth);

    if ((err == SSZ_SUCCESS) && (data_depth > cache->depth))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (err == SSZ_SUCCESS)
    {
        cache->cached_data_root = cache->nodes[cache->level_offsets[data_depth]];
        cache->data_root_valid = true;
        cache->final_root_valid = false;
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_flush_contiguous_run(
    ssz_merkle_cache_t *cache,
    const ssz_chunk_t *child_level_nodes,
    ssz_chunk_t *parent_level_nodes,
    uint64_t run_start,
    uint64_t run_len,
    size_t *io_gather_count)
{
    size_t run_start_sz = 0u;
    size_t run_len_sz = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (child_level_nodes == NULL) || (parent_level_nodes == NULL) ||
        (io_gather_count == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (run_len == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if (
        !ssz_internal_u64_to_size(run_start, &run_start_sz) ||
        !ssz_internal_u64_to_size(run_len, &run_len_sz))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (run_len >= 2u)
    {
        err = ssz_hash_2to1_batch(
            cache->hash_fn,
            &child_level_nodes[run_start_sz * 2u],
            run_len_sz,
            &parent_level_nodes[run_start_sz]);
    }
    else if (*io_gather_count >= cache->gather_pair_capacity)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        cache->gather_parent_indices[*io_gather_count] = run_start_sz;
        cache->gather_pairs[*io_gather_count * 2u] = child_level_nodes[run_start_sz * 2u];
        cache->gather_pairs[((*io_gather_count) * 2u) + 1u] =
            child_level_nodes[(run_start_sz * 2u) + 1u];
        (*io_gather_count)++;
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_hash_dirty_parents_exact(
    ssz_merkle_cache_t *cache,
    uint32_t level,
    ssz_merkle_cache_dirty_set_t *dirty_parents,
    uint64_t parent_width)
{
    size_t gather_count = 0u;
    ssz_chunk_t *parent_level_nodes = NULL;
    const ssz_chunk_t *child_level_nodes = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (dirty_parents == NULL) || (dirty_parents->word_count == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (*dirty_parents->word_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else
    {
        ssz_merkle_cache_internal_sort_size_t_asc(
            dirty_parents->word_idx,
            *dirty_parents->word_count);

        bool run_active = false;
        uint64_t run_start = 0u;
        uint64_t run_prev = 0u;
        uint64_t run_len = 0u;

        child_level_nodes = &cache->nodes[cache->level_offsets[level]];
        parent_level_nodes = &cache->nodes[cache->level_offsets[level + 1u]];

        for (size_t wi = 0u; (wi < *dirty_parents->word_count) && (err == SSZ_SUCCESS); wi++)
        {
            size_t word_index = dirty_parents->word_idx[wi];

            if (word_index >= dirty_parents->word_capacity)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                uint64_t word_bits = dirty_parents->bits[word_index];
                uint64_t base = ((uint64_t)word_index) << 6u;

                while ((word_bits != 0u) && (err == SSZ_SUCCESS))
                {
                    unsigned bit = ssz_merkle_cache_internal_ctz_u64(word_bits);
                    uint64_t parent_index = base + (uint64_t)bit;

                    if (parent_index < parent_width)
                    {
                        if (!run_active)
                        {
                            run_active = true;
                            run_start = parent_index;
                            run_prev = parent_index;
                            run_len = 1u;
                        }
                        else if (parent_index == (run_prev + 1u))
                        {
                            run_prev = parent_index;
                            run_len++;
                        }
                        else
                        {
                            err = ssz_merkle_cache_internal_flush_contiguous_run(
                                cache,
                                child_level_nodes,
                                parent_level_nodes,
                                run_start,
                                run_len,
                                &gather_count);
                            if (err == SSZ_SUCCESS)
                            {
                                run_start = parent_index;
                                run_prev = parent_index;
                                run_len = 1u;
                            }
                            else
                            {
                                /* intentionally empty */
                            }
                        }
                    }
                    else
                    {
                        /* intentionally empty */
                    }

                    word_bits &= (word_bits - 1u);
                }
            }
        }

        if (run_active && (err == SSZ_SUCCESS))
        {
            err = ssz_merkle_cache_internal_flush_contiguous_run(
                cache,
                child_level_nodes,
                parent_level_nodes,
                run_start,
                run_len,
                &gather_count);
        }
        else
        {
            /* intentionally empty */
        }

        if ((err == SSZ_SUCCESS) && (gather_count != 0u))
        {
            if (cache->use_gather_inplace)
            {
                err =
                    ssz_hash_2to1_batch_inplace(cache->hash_fn, cache->gather_pairs, gather_count);
                if (err == SSZ_SUCCESS)
                {
                    for (size_t i = 0u; i < gather_count; i++)
                    {
                        parent_level_nodes[cache->gather_parent_indices[i]] =
                            cache->gather_pairs[i];
                    }
                }
                else
                {
                    /* intentionally empty */
                }
            }
            else
            {
                err = ssz_hash_2to1_batch(
                    cache->hash_fn,
                    cache->gather_pairs,
                    gather_count,
                    cache->gather_hashes);
                if (err == SSZ_SUCCESS)
                {
                    for (size_t i = 0u; i < gather_count; i++)
                    {
                        parent_level_nodes[cache->gather_parent_indices[i]] =
                            cache->gather_hashes[i];
                    }
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_build_parent_dirty_set(
    size_t *child_word_idx,
    size_t child_word_count,
    const uint64_t *child_bits,
    size_t child_word_capacity,
    uint64_t child_width,
    ssz_merkle_cache_dirty_set_t *out_parent_set)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((child_word_idx == NULL) || (child_bits == NULL) || (out_parent_set == NULL) ||
        (out_parent_set->word_count == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (child_word_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else
    {
        ssz_merkle_cache_internal_sort_size_t_asc(child_word_idx, child_word_count);

        for (size_t wi = 0u; (wi < child_word_count) && (err == SSZ_SUCCESS); wi++)
        {
            size_t word_index = child_word_idx[wi];

            if (word_index >= child_word_capacity)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                uint64_t word_bits = child_bits[word_index];
                uint64_t base = ((uint64_t)word_index) << 6u;

                while ((word_bits != 0u) && (err == SSZ_SUCCESS))
                {
                    unsigned bit = ssz_merkle_cache_internal_ctz_u64(word_bits);
                    uint64_t child_index = base + (uint64_t)bit;

                    if (child_index < child_width)
                    {
                        err = ssz_merkle_cache_internal_dirty_set_mark(
                            out_parent_set,
                            child_index >> 1u);
                    }
                    else
                    {
                        /* intentionally empty */
                    }

                    word_bits &= (word_bits - 1u);
                }
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_recompute_data_root(ssz_merkle_cache_t *cache)
{
    ssz_merkle_cache_dirty_set_t leaf_set;
    ssz_merkle_cache_dirty_set_t parent_sets[2];
    ssz_merkle_cache_dirty_set_t *current_scratch = NULL;
    ssz_merkle_cache_dirty_set_t *current_set = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&leaf_set, 0, sizeof(leaf_set));
    (void)memset(parent_sets, 0, sizeof(parent_sets));

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (cache->leaf_dirty_word_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if (cache->depth == 0u)
    {
        ssz_merkle_cache_internal_clear_dirty(
            cache->leaf_dirty_bits,
            cache->leaf_dirty_word_idx,
            &cache->leaf_dirty_word_count);
    }
    else
    {
        bool current_is_leaf = true;
        uint64_t current_width = cache->leaf_capacity;

        ssz_merkle_cache_internal_bind_dirty_set(
            &leaf_set,
            cache->leaf_dirty_bits,
            cache->leaf_dirty_word_idx,
            &cache->leaf_dirty_word_count,
            cache->leaf_dirty_word_capacity);
        ssz_merkle_cache_internal_bind_dirty_set(
            &parent_sets[0],
            cache->parent_dirty_bits[0],
            cache->parent_dirty_word_idx[0],
            &cache->parent_dirty_word_count[0],
            cache->parent_dirty_word_capacity);
        ssz_merkle_cache_internal_bind_dirty_set(
            &parent_sets[1],
            cache->parent_dirty_bits[1],
            cache->parent_dirty_word_idx[1],
            &cache->parent_dirty_word_count[1],
            cache->parent_dirty_word_capacity);

        current_set = &leaf_set;

        for (uint32_t level = 0u; (level < cache->depth) && (err == SSZ_SUCCESS); level++)
        {
            uint64_t parent_width = current_width >> 1u;
            ssz_merkle_cache_dirty_set_t *next_set = &parent_sets[level & 1u];

            ssz_merkle_cache_internal_dirty_set_clear(next_set);

            if (*current_set->word_count != 0u)
            {
                err = ssz_merkle_cache_internal_build_parent_dirty_set(
                    current_set->word_idx,
                    *current_set->word_count,
                    current_set->bits,
                    current_set->word_capacity,
                    current_width,
                    next_set);
            }
            else
            {
                /* intentionally empty */
            }

            if ((err == SSZ_SUCCESS) && (*next_set->word_count != 0u))
            {
                err = ssz_merkle_cache_internal_hash_dirty_parents_exact(
                    cache,
                    level,
                    next_set,
                    parent_width);
            }
            else
            {
                /* intentionally empty */
            }

            if (err == SSZ_SUCCESS)
            {
                if (current_is_leaf)
                {
                    ssz_merkle_cache_internal_dirty_set_clear(&leaf_set);
                }
                else if (current_scratch != NULL)
                {
                    ssz_merkle_cache_internal_dirty_set_clear(current_scratch);
                }
                else
                {
                    /* intentionally empty */
                }

                current_is_leaf = false;
                current_scratch = next_set;
                current_set = next_set;
                current_width = parent_width;
            }
            else
            {
                /* intentionally empty */
            }
        }

        if (current_scratch != NULL)
        {
            ssz_merkle_cache_internal_dirty_set_clear(current_scratch);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_validate_config(
    const ssz_merkle_cache_config_t *config,
    const ssz_hash_fn_t **out_resolved_hash_fn)
{
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((config == NULL) || (out_resolved_hash_fn == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_merkle_cache_internal_struct_size_valid(config->struct_size, sizeof(*config)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (
        (config->leaf_limit != SSZ_NO_LIMIT) && (config->initial_leaf_count > config->leaf_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        resolved_hash_fn = ssz_internal_resolve_hash_fn(config->hash_fn);
        if ((resolved_hash_fn == NULL) || (resolved_hash_fn->hash == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (config->leaf_limit == SSZ_NO_LIMIT)
        {
            if (!ssz_merkle_cache_internal_is_power_of_two_u64(config->reserved_leaf_capacity))
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else if (config->initial_leaf_count > config->reserved_leaf_capacity)
            {
                err = SSZ_ERR_LIMIT_EXCEEDED;
            }
            else
            {
                *out_resolved_hash_fn = resolved_hash_fn;
            }
        }
        else
        {
            *out_resolved_hash_fn = resolved_hash_fn;
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_compute_requirements(
    const ssz_merkle_cache_config_t *config,
    ssz_merkle_cache_requirements_t *out_requirements,
    const ssz_hash_fn_t **out_resolved_hash_fn)
{
    ssz_merkle_cache_requirements_t requirements;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    uint64_t mutable_capacity = 0u;
    uint64_t physical_capacity = 0u;
    uint64_t nodes_u64 = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&requirements, 0, sizeof(requirements));
    requirements.struct_size = sizeof(requirements);

    if (out_requirements == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_validate_config(config, &resolved_hash_fn);
    }

    if (err == SSZ_SUCCESS)
    {
        if (config->leaf_limit == SSZ_NO_LIMIT)
        {
            mutable_capacity = config->reserved_leaf_capacity;
            physical_capacity = config->reserved_leaf_capacity;
        }
        else
        {
            mutable_capacity = config->leaf_limit;
            physical_capacity = ssz_next_pow_of_two(config->leaf_limit);
            if (physical_capacity == 0u)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                /* intentionally empty */
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        requirements.physical_leaf_capacity = physical_capacity;
        requirements.mutable_leaf_capacity = mutable_capacity;
        requirements.depth = ssz_merkle_cache_internal_log2_u64(physical_capacity);

        if (ssz_internal_mul_overflow_u64(physical_capacity, 2u, &nodes_u64) || (nodes_u64 == 0u))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            nodes_u64 -= 1u;
            if (!ssz_internal_u64_to_size(nodes_u64, &requirements.nodes_count))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                err = ssz_merkle_cache_internal_leaf_words_for_capacity(
                    mutable_capacity,
                    &requirements.leaf_dirty_words);
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_parent_dirty_words_for_capacity(
            mutable_capacity,
            &requirements.parent_dirty_words);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_gather_pair_capacity_for_capacity(
            mutable_capacity,
            &requirements.gather_pair_capacity);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        if (ssz_internal_mul_overflow_size(
                requirements.gather_pair_capacity,
                2u,
                &requirements.gather_pairs_count))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            requirements.gather_parent_indices_count = requirements.gather_pair_capacity;
            requirements.gather_hashes_count =
                (resolved_hash_fn == ssz_hash_default()) ? 0u : requirements.gather_pair_capacity;

            if (!ssz_internal_u64_to_size(mutable_capacity, &requirements.token_values_count) ||
                !ssz_internal_u64_to_size(mutable_capacity, &requirements.root_batch_roots_count))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                requirements.token_valid_words = requirements.leaf_dirty_words;
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        *out_requirements = requirements;
        if (out_resolved_hash_fn != NULL)
        {
            *out_resolved_hash_fn = resolved_hash_fn;
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

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_validate_storage(
    const ssz_merkle_cache_requirements_t *requirements,
    const ssz_merkle_cache_storage_t *storage,
    bool *out_token_storage_ready)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((requirements == NULL) || (storage == NULL) || (out_token_storage_ready == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_merkle_cache_internal_struct_size_valid(storage->struct_size, sizeof(*storage)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((storage->nodes == NULL) || (storage->nodes_count < requirements->nodes_count))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (!ssz_merkle_cache_internal_chunk_buffer_aligned(
                 storage->nodes,
                 requirements->nodes_count))
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else if (
        !ssz_merkle_cache_internal_pointer_available(
            storage->leaf_dirty_bits,
            requirements->leaf_dirty_words) ||
        (storage->leaf_dirty_words < requirements->leaf_dirty_words) ||
        !ssz_merkle_cache_internal_pointer_available(
            storage->leaf_dirty_word_idx,
            requirements->leaf_dirty_words) ||
        (storage->leaf_dirty_word_idx_count < requirements->leaf_dirty_words))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (
        (requirements->parent_dirty_words != 0u) &&
        (!ssz_merkle_cache_internal_pointer_available(
             storage->parent_dirty_bits[0],
             requirements->parent_dirty_words) ||
         !ssz_merkle_cache_internal_pointer_available(
             storage->parent_dirty_bits[1],
             requirements->parent_dirty_words) ||
         (storage->parent_dirty_words < requirements->parent_dirty_words) ||
         !ssz_merkle_cache_internal_pointer_available(
             storage->parent_dirty_word_idx[0],
             requirements->parent_dirty_words) ||
         !ssz_merkle_cache_internal_pointer_available(
             storage->parent_dirty_word_idx[1],
             requirements->parent_dirty_words) ||
         (storage->parent_dirty_word_idx_count < requirements->parent_dirty_words)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (
        (requirements->gather_pairs_count != 0u) &&
        (!ssz_merkle_cache_internal_pointer_available(
             storage->gather_pairs,
             requirements->gather_pairs_count) ||
         (storage->gather_pairs_count < requirements->gather_pairs_count) ||
         !ssz_merkle_cache_internal_pointer_available(
             storage->gather_parent_indices,
             requirements->gather_parent_indices_count) ||
         (storage->gather_parent_indices_count < requirements->gather_parent_indices_count)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (
        (requirements->gather_pairs_count != 0u) &&
        !ssz_merkle_cache_internal_chunk_buffer_aligned(
            storage->gather_pairs,
            requirements->gather_pairs_count))
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else if (
        (requirements->gather_hashes_count != 0u) &&
        (!ssz_merkle_cache_internal_pointer_available(
             storage->gather_hashes,
             requirements->gather_hashes_count) ||
         (storage->gather_hashes_count < requirements->gather_hashes_count)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else if (
        (requirements->gather_hashes_count != 0u) &&
        !ssz_merkle_cache_internal_chunk_buffer_aligned(
            storage->gather_hashes,
            requirements->gather_hashes_count))
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else
    {
        bool token_storage_ready = ssz_merkle_cache_internal_storage_has_tokens(storage);

        if (token_storage_ready &&
            ((storage->token_values_count < requirements->token_values_count) ||
             (storage->token_valid_words < requirements->token_valid_words) ||
             !ssz_merkle_cache_internal_pointer_available(
                 storage->token_values,
                 requirements->token_values_count) ||
             !ssz_merkle_cache_internal_pointer_available(
                 storage->token_valid_bits,
                 requirements->token_valid_words)))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else
        {
            *out_token_storage_ready = token_storage_ready;
        }
    }

    return err;
}

static void ssz_merkle_cache_internal_clear_u64_array(uint64_t *values, size_t count)
{
    if ((values != NULL) && (count != 0u))
    {
        (void)memset(values, 0, count * sizeof(*values));
    }
    else
    {
        /* intentionally empty */
    }
}

static void ssz_merkle_cache_internal_clear_bound_working_state(ssz_merkle_cache_t *cache)
{
    if (cache != NULL)
    {
        ssz_merkle_cache_internal_clear_u64_array(
            cache->leaf_dirty_bits,
            cache->leaf_dirty_word_capacity);
        cache->leaf_dirty_word_count = 0u;

        ssz_merkle_cache_internal_clear_u64_array(
            cache->parent_dirty_bits[0],
            cache->parent_dirty_word_capacity);
        ssz_merkle_cache_internal_clear_u64_array(
            cache->parent_dirty_bits[1],
            cache->parent_dirty_word_capacity);
        cache->parent_dirty_word_count[0] = 0u;
        cache->parent_dirty_word_count[1] = 0u;

        if (cache->token_storage_ready)
        {
            ssz_merkle_cache_internal_clear_u64_array(cache->token_values, cache->token_capacity);
            ssz_merkle_cache_internal_clear_u64_array(
                cache->token_valid_bits,
                cache->token_word_capacity);
        }
        else
        {
            /* intentionally empty */
        }

        (void)memset(&cache->cached_data_root, 0, sizeof(cache->cached_data_root));
        (void)memset(&cache->cached_root, 0, sizeof(cache->cached_root));
        cache->data_root_valid = false;
        cache->final_root_valid = false;
        cache->needs_resync = false;
    }
    else
    {
        /* intentionally empty */
    }
}

static ssz_error_t ssz_merkle_cache_internal_prepare_bound_cache(
    const ssz_merkle_cache_config_t *config,
    const ssz_merkle_cache_requirements_t *requirements,
    const ssz_merkle_cache_storage_t *storage,
    const ssz_hash_fn_t *resolved_hash_fn,
    bool token_storage_ready,
    ssz_merkle_cache_t *out_cache)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((config == NULL) || (requirements == NULL) || (storage == NULL) ||
        (resolved_hash_fn == NULL) || (out_cache == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_cache, 0, sizeof(*out_cache));

        out_cache->struct_size = sizeof(*out_cache);
        out_cache->hash_fn = resolved_hash_fn;
        out_cache->leaf_limit = config->leaf_limit;
        out_cache->leaf_capacity = requirements->physical_leaf_capacity;
        out_cache->mutable_leaf_capacity = requirements->mutable_leaf_capacity;
        out_cache->depth = requirements->depth;
        out_cache->leaf_count = config->initial_leaf_count;
        out_cache->logical_length = config->logical_length;
        out_cache->mix_in_length = config->mix_in_length;

        out_cache->nodes = storage->nodes;
        out_cache->leaf_dirty_bits = storage->leaf_dirty_bits;
        out_cache->leaf_dirty_word_idx = storage->leaf_dirty_word_idx;
        out_cache->leaf_dirty_word_capacity = requirements->leaf_dirty_words;

        out_cache->parent_dirty_bits[0] = storage->parent_dirty_bits[0];
        out_cache->parent_dirty_bits[1] = storage->parent_dirty_bits[1];
        out_cache->parent_dirty_word_idx[0] = storage->parent_dirty_word_idx[0];
        out_cache->parent_dirty_word_idx[1] = storage->parent_dirty_word_idx[1];
        out_cache->parent_dirty_word_capacity = requirements->parent_dirty_words;

        out_cache->gather_pairs = storage->gather_pairs;
        out_cache->gather_hashes = storage->gather_hashes;
        out_cache->gather_parent_indices = storage->gather_parent_indices;
        out_cache->gather_pair_capacity = requirements->gather_pair_capacity;
        out_cache->use_gather_inplace = (resolved_hash_fn == ssz_hash_default());

        if (token_storage_ready)
        {
            out_cache->token_values = storage->token_values;
            out_cache->token_valid_bits = storage->token_valid_bits;
            out_cache->token_capacity = requirements->token_values_count;
            out_cache->token_word_capacity = requirements->token_valid_words;
            out_cache->token_storage_ready = true;
        }
        else
        {
            out_cache->token_values = NULL;
            out_cache->token_valid_bits = NULL;
            out_cache->token_capacity = 0u;
            out_cache->token_word_capacity = 0u;
            out_cache->token_storage_ready = false;
        }

        err = ssz_merkle_cache_internal_compute_level_offsets(
            out_cache->leaf_capacity,
            out_cache->depth,
            out_cache->level_offsets);
        if (err == SSZ_SUCCESS)
        {
            if (resolved_hash_fn == ssz_hash_default())
            {
                out_cache->zero_hashes = ssz_hash_default_zero_hashes();
            }
            else
            {
                err = ssz_merkle_cache_internal_build_zero_hashes(
                    resolved_hash_fn,
                    out_cache->zero_hashes_buf);
                if (err == SSZ_SUCCESS)
                {
                    out_cache->zero_hashes = out_cache->zero_hashes_buf;
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }
        else
        {
            /* intentionally empty */
        }

        if (err == SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_clear_bound_working_state(out_cache);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_initialize_bound_cache(ssz_merkle_cache_t *cache)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_merkle_cache_internal_fill_zero_tree(cache);
        err = ssz_merkle_cache_internal_refresh_cached_data_root(cache);
        if (err == SSZ_SUCCESS)
        {
            if (cache->mix_in_length)
            {
                err = ssz_mix_in_length_u64(
                    &cache->cached_data_root,
                    cache->logical_length,
                    cache->hash_fn,
                    &cache->cached_root);
                if (err == SSZ_SUCCESS)
                {
                    cache->final_root_valid = true;
                }
                else
                {
                    /* intentionally empty */
                }
            }
            else
            {
                cache->cached_root = cache->cached_data_root;
                cache->final_root_valid = true;
            }
        }
        else
        {
            /* intentionally empty */
        }

        if (err == SSZ_SUCCESS)
        {
            cache->data_root_valid = true;
            cache->needs_resync = false;
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_migrate_nodes(
    const ssz_merkle_cache_t *src,
    ssz_merkle_cache_t *dst)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((src == NULL) || (dst == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint32_t level = 0u; (level <= dst->depth) && (err == SSZ_SUCCESS); level++)
        {
            uint64_t dst_width = dst->leaf_capacity >> level;
            uint64_t src_width = 0u;
            size_t dst_width_sz = 0u;
            size_t src_width_sz = 0u;
            ssz_chunk_t *dst_level_nodes = NULL;

            if (level <= src->depth)
            {
                src_width = src->leaf_capacity >> level;
            }
            else
            {
                src_width = 0u;
            }

            if (!ssz_internal_u64_to_size(dst_width, &dst_width_sz) ||
                !ssz_internal_u64_to_size(src_width, &src_width_sz))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                dst_level_nodes = &dst->nodes[dst->level_offsets[level]];

                if (level > src->depth)
                {
                    const ssz_chunk_t *child_level_nodes =
                        &dst->nodes[dst->level_offsets[level - 1u]];

                    err = ssz_hash_2to1_batch(
                        dst->hash_fn,
                        child_level_nodes,
                        dst_width_sz,
                        dst_level_nodes);
                }
                else if (src_width_sz != 0u)
                {
                    (void)memcpy(
                        dst_level_nodes,
                        &src->nodes[src->level_offsets[level]],
                        src_width_sz * sizeof(*dst_level_nodes));
                }
                else
                {
                    /* intentionally empty */
                }

                if (err != SSZ_SUCCESS)
                {
                    /* propagate error */
                }
                else if (level <= src->depth)
                {
                    for (size_t i = src_width_sz; i < dst_width_sz; i++)
                    {
                        dst_level_nodes[i] = dst->zero_hashes[level];
                    }
                }
                else
                {
                    /* rebuilt from the destination child level */
                }
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_capacity_for_count(
    const ssz_merkle_cache_t *cache,
    uint64_t required_count)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (required_count > cache->mutable_leaf_capacity)
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        err = SSZ_SUCCESS;
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_compute_data_root_if_needed(ssz_merkle_cache_t *cache)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!cache->data_root_valid)
    {
        err = ssz_merkle_cache_internal_recompute_data_root(cache);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_internal_refresh_cached_data_root(cache);
        }
        else
        {
            /* intentionally empty */
        }
    }
    else
    {
        err = SSZ_SUCCESS;
    }

    return err;
}

static bool ssz_merkle_cache_internal_limit_matches(
    const ssz_merkle_cache_t *cache,
    uint64_t expected_limit)
{
    bool matches = false;

    if (cache == NULL)
    {
        matches = false;
    }
    else if (cache->leaf_limit == expected_limit)
    {
        matches = true;
    }
    else
    {
        matches = false;
    }

    return matches;
}

static ssz_error_t ssz_merkle_cache_internal_validate_sync_opts(
    const ssz_merkle_cache_t *cache,
    uint64_t element_count,
    const ssz_merkle_cache_sync_composite_opts_t *opts)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (opts == NULL)
    {
        err = SSZ_SUCCESS;
    }
    else if (!ssz_merkle_cache_internal_struct_size_valid(opts->struct_size, sizeof(*opts)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((opts->token != NULL) && !cache->token_storage_ready)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (opts->root_batch != NULL)
    {
        if (opts->workspace == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (!ssz_merkle_cache_internal_struct_size_valid(
                     opts->workspace->struct_size,
                     sizeof(*opts->workspace)))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (
            (opts->workspace->root_batch_roots_count < (size_t)element_count) ||
            !ssz_merkle_cache_internal_pointer_available(
                opts->workspace->root_batch_roots,
                (size_t)element_count))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else if (!ssz_merkle_cache_internal_chunk_buffer_aligned(
                     opts->workspace->root_batch_roots,
                     (size_t)element_count))
        {
            err = SSZ_ERR_ALIGNMENT_INVALID;
        }
        else
        {
            err = SSZ_SUCCESS;
        }
    }
    else
    {
        err = SSZ_SUCCESS;
    }

    return err;
}

ssz_error_t ssz_merkle_cache_requirements(
    const ssz_merkle_cache_config_t *config,
    ssz_merkle_cache_requirements_t *out_requirements)
{
    return ssz_merkle_cache_internal_compute_requirements(config, out_requirements, NULL);
}

ssz_error_t ssz_merkle_cache_bind(
    const ssz_merkle_cache_config_t *config,
    const ssz_merkle_cache_storage_t *storage,
    ssz_merkle_cache_t *out_cache)
{
    ssz_merkle_cache_requirements_t requirements;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    bool token_storage_ready = false;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&requirements, 0, sizeof(requirements));

    if (out_cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_compute_requirements(
            config,
            &requirements,
            &resolved_hash_fn);
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_validate_storage(
            &requirements,
            storage,
            &token_storage_ready);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_prepare_bound_cache(
            config,
            &requirements,
            storage,
            resolved_hash_fn,
            token_storage_ready,
            out_cache);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_initialize_bound_cache(out_cache);
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

ssz_error_t ssz_merkle_cache_migrate_into(
    const ssz_merkle_cache_t *src,
    const ssz_merkle_cache_config_t *dst_config,
    const ssz_merkle_cache_storage_t *dst_storage,
    ssz_merkle_cache_t *out_cache)
{
    ssz_merkle_cache_config_t bind_config;
    ssz_merkle_cache_requirements_t requirements;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    bool token_storage_ready = false;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&bind_config, 0, sizeof(bind_config));
    (void)memset(&requirements, 0, sizeof(requirements));

    if (!ssz_merkle_cache_internal_cache_is_bound(src) || (dst_config == NULL) ||
        (dst_storage == NULL) || (out_cache == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_resolve_hash_fn(dst_config->hash_fn) != src->hash_fn)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (dst_config->mix_in_length != src->mix_in_length)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (dst_config->leaf_limit != src->leaf_limit)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (
        (src->leaf_limit == SSZ_NO_LIMIT) &&
        (dst_config->reserved_leaf_capacity < src->leaf_capacity))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        bind_config = *dst_config;
        bind_config.initial_leaf_count = src->leaf_count;
        bind_config.logical_length = src->logical_length;

        err = ssz_merkle_cache_internal_compute_requirements(
            &bind_config,
            &requirements,
            &resolved_hash_fn);
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_validate_storage(
            &requirements,
            dst_storage,
            &token_storage_ready);
        if ((err == SSZ_SUCCESS) && src->token_storage_ready && !token_storage_ready)
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
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

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_prepare_bound_cache(
            &bind_config,
            &requirements,
            dst_storage,
            resolved_hash_fn,
            token_storage_ready,
            out_cache);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_migrate_nodes(src, out_cache);
    }
    else
    {
        /* intentionally empty */
    }

    if ((err == SSZ_SUCCESS) && (src->leaf_dirty_word_capacity != 0u))
    {
        (void)memcpy(
            out_cache->leaf_dirty_bits,
            src->leaf_dirty_bits,
            src->leaf_dirty_word_capacity * sizeof(*out_cache->leaf_dirty_bits));
        if (src->leaf_dirty_word_count != 0u)
        {
            (void)memcpy(
                out_cache->leaf_dirty_word_idx,
                src->leaf_dirty_word_idx,
                src->leaf_dirty_word_count * sizeof(*out_cache->leaf_dirty_word_idx));
        }
        else
        {
            /* intentionally empty */
        }
        out_cache->leaf_dirty_word_count = src->leaf_dirty_word_count;
    }
    else
    {
        /* intentionally empty */
    }

    if ((err == SSZ_SUCCESS) && src->token_storage_ready && out_cache->token_storage_ready)
    {
        if (src->token_capacity != 0u)
        {
            (void)memcpy(
                out_cache->token_values,
                src->token_values,
                src->token_capacity * sizeof(*out_cache->token_values));
        }
        else
        {
            /* intentionally empty */
        }

        if (src->token_word_capacity != 0u)
        {
            (void)memcpy(
                out_cache->token_valid_bits,
                src->token_valid_bits,
                src->token_word_capacity * sizeof(*out_cache->token_valid_bits));
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

    if (err == SSZ_SUCCESS)
    {
        if (src->depth == out_cache->depth)
        {
            out_cache->cached_data_root = src->cached_data_root;
            out_cache->cached_root = src->cached_root;
            out_cache->data_root_valid = src->data_root_valid;
            out_cache->final_root_valid = src->final_root_valid;
        }
        else
        {
            out_cache->data_root_valid = false;
            out_cache->final_root_valid = false;
        }
        out_cache->needs_resync = src->needs_resync;
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

ssz_error_t ssz_merkle_cache_reset(ssz_merkle_cache_t *cache)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_merkle_cache_internal_fill_zero_tree(cache);
        ssz_merkle_cache_internal_clear_bound_working_state(cache);

        cache->leaf_count = 0u;
        cache->logical_length = 0u;

        err = ssz_merkle_cache_internal_refresh_cached_data_root(cache);
    }

    return err;
}

ssz_error_t ssz_merkle_cache_data_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_validate_chunk_pointer(out_root) != SSZ_SUCCESS)
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else
    {
        err = ssz_merkle_cache_internal_compute_data_root_if_needed(cache);
        if (err == SSZ_SUCCESS)
        {
            *out_root = cache->cached_data_root;
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_validate_chunk_pointer(out_root) != SSZ_SUCCESS)
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else
    {
        err = ssz_merkle_cache_internal_compute_data_root_if_needed(cache);
        if (err == SSZ_SUCCESS)
        {
            if (!cache->mix_in_length)
            {
                cache->cached_root = cache->cached_data_root;
                cache->final_root_valid = true;
            }
            else if (!cache->final_root_valid)
            {
                err = ssz_mix_in_length_u64(
                    &cache->cached_data_root,
                    cache->logical_length,
                    cache->hash_fn,
                    &cache->cached_root);
                if (err == SSZ_SUCCESS)
                {
                    cache->final_root_valid = true;
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

            if (err == SSZ_SUCCESS)
            {
                *out_root = cache->cached_root;
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

    return err;
}

ssz_error_t ssz_merkle_cache_update_root_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    const ssz_chunk_t *roots,
    uint64_t root_count)
{
    uint64_t end_index = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (root_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if (roots == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_merkle_cache_internal_chunk_buffer_aligned(roots, (size_t)root_count))
    {
        err = SSZ_ERR_ALIGNMENT_INVALID;
    }
    else if (ssz_internal_add_overflow_u64(start_index, root_count, &end_index))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, end_index);
    }

    if ((err == SSZ_SUCCESS) && (root_count != 0u))
    {
        for (uint64_t i = 0u; (i < root_count) && (err == SSZ_SUCCESS); i++)
        {
            uint64_t index = start_index + i;

            err = ssz_merkle_cache_internal_set_leaf(cache, index, &roots[i], false);
            if (err == SSZ_SUCCESS)
            {
                ssz_merkle_cache_internal_token_valid_set(cache, index, false);
            }
            else
            {
                /* intentionally empty */
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if ((err == SSZ_SUCCESS) && (root_count != 0u))
    {
        if (end_index > cache->leaf_count)
        {
            cache->leaf_count = end_index;
            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
        else
        {
            /* intentionally empty */
        }

        cache->needs_resync = false;
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

ssz_error_t ssz_merkle_cache_zero_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    uint64_t zero_count)
{
    uint64_t end_index = 0u;
    uint64_t old_leaf_count = 0u;
    ssz_chunk_t zero_leaf;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&zero_leaf, 0, sizeof(zero_leaf));

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (zero_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if (ssz_internal_add_overflow_u64(start_index, zero_count, &end_index))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, end_index);
    }

    if (err == SSZ_SUCCESS)
    {
        old_leaf_count = cache->leaf_count;
        for (uint64_t i = start_index; (i < end_index) && (err == SSZ_SUCCESS); i++)
        {
            err = ssz_merkle_cache_internal_set_leaf(cache, i, &zero_leaf, false);
        }
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        ssz_merkle_cache_internal_token_valid_clear_range(
            cache,
            start_index,
            end_index - start_index);

        if ((start_index < old_leaf_count) && (end_index >= old_leaf_count))
        {
            cache->leaf_count = start_index;
            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
        else
        {
            /* intentionally empty */
        }

        cache->needs_resync = false;
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

ssz_error_t ssz_merkle_cache_set_logical_length(ssz_merkle_cache_t *cache, uint64_t logical_length)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (cache->logical_length != logical_length)
        {
            cache->logical_length = logical_length;
            ssz_merkle_cache_internal_invalidate_final_root(cache);
        }
        else
        {
            /* intentionally empty */
        }
        cache->needs_resync = false;
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_packed_bytes(
    ssz_merkle_cache_t *cache,
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t logical_length)
{
    uint64_t chunk_count = 0u;
    uint64_t old_leaf_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((bytes_len != 0u) && (bytes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_byte_len_to_chunk_count(bytes_len, &chunk_count);
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, chunk_count);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        old_leaf_count = cache->leaf_count;
        for (uint64_t chunk_index = 0u; (chunk_index < chunk_count) && (err == SSZ_SUCCESS);
             chunk_index++)
        {
            ssz_chunk_t leaf;
            size_t chunk_offset = 0u;

            (void)memset(leaf.bytes, 0, sizeof(leaf.bytes));
            if (!ssz_internal_u64_to_size(chunk_index, &chunk_offset) ||
                ssz_internal_mul_overflow_size(chunk_offset, SSZ_BYTES_PER_CHUNK, &chunk_offset))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                if (chunk_offset < bytes_len)
                {
                    size_t copy_len = SSZ_BYTES_PER_CHUNK;
                    size_t remaining = bytes_len - chunk_offset;

                    if (remaining < copy_len)
                    {
                        copy_len = remaining;
                    }
                    else
                    {
                        /* intentionally empty */
                    }
                    (void)memcpy(leaf.bytes, &bytes[chunk_offset], copy_len);
                }
                else
                {
                    /* intentionally empty */
                }

                err = ssz_merkle_cache_internal_set_leaf(cache, chunk_index, &leaf, false);
                if (err == SSZ_SUCCESS)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, chunk_index, false);
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if ((err == SSZ_SUCCESS) && (chunk_count < old_leaf_count))
    {
        err = ssz_merkle_cache_zero_range(cache, chunk_count, old_leaf_count - chunk_count);
    }
    else if ((err == SSZ_SUCCESS) && (chunk_count > old_leaf_count))
    {
        cache->leaf_count = chunk_count;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_set_logical_length(cache, logical_length);
        if (err == SSZ_SUCCESS)
        {
            cache->needs_resync = false;
        }
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_packed_vector_fixed(
    ssz_merkle_cache_t *cache,
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size)
{
    size_t total_bytes = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
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
    else if (!ssz_merkle_cache_internal_limit_matches(cache, SSZ_NO_LIMIT))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (cache->mix_in_length)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err =
            ssz_merkle_cache_sync_packed_bytes(cache, elements, total_bytes, cache->logical_length);
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_packed_list_fixed(
    ssz_merkle_cache_t *cache,
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size)
{
    size_t total_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
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
        else
        {
            /* intentionally empty */
        }

        if ((err == SSZ_SUCCESS) && !ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if ((err == SSZ_SUCCESS) && !cache->mix_in_length)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_sync_packed_bytes(cache, elements, total_bytes, element_count);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_bitvector(
    ssz_merkle_cache_t *cache,
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
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
            else
            {
                /* intentionally empty */
            }
        }
        else
        {
            /* intentionally empty */
        }

        if ((err == SSZ_SUCCESS) && ssz_internal_add_overflow_u64(bit_count, 255u, &chunk_limit))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else if (err == SSZ_SUCCESS)
        {
            chunk_limit /= 256u;
            if (!ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else if (cache->mix_in_length)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = ssz_merkle_cache_sync_packed_bytes(
                    cache,
                    bits_le,
                    bitfield_bytes,
                    cache->logical_length);
            }
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_bitlist(
    ssz_merkle_cache_t *cache,
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = SSZ_NO_LIMIT;
    ssz_error_t err = SSZ_SUCCESS;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache))
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
            else
            {
                /* intentionally empty */
            }
        }
        else
        {
            /* intentionally empty */
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
        else
        {
            /* intentionally empty */
        }

        if ((err == SSZ_SUCCESS) && !ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if ((err == SSZ_SUCCESS) && !cache->mix_in_length)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_sync_packed_bytes(cache, bits_le, bitfield_bytes, bit_len);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_sync_composite_run(
    ssz_merkle_cache_t *cache,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts,
    uint64_t run_start,
    uint64_t run_len,
    bool commit_tokens)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (codec == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (run_len == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if (commit_tokens && !cache->token_storage_ready)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((opts != NULL) && (opts->root_batch != NULL))
    {
        if ((opts->workspace == NULL) || !ssz_merkle_cache_internal_struct_size_valid(
                                             opts->workspace->struct_size,
                                             sizeof(*opts->workspace)))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (
            (opts->workspace->root_batch_roots_count < (size_t)run_len) ||
            !ssz_merkle_cache_internal_pointer_available(
                opts->workspace->root_batch_roots,
                (size_t)run_len))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else if (!ssz_merkle_cache_internal_chunk_buffer_aligned(
                     opts->workspace->root_batch_roots,
                     (size_t)run_len))
        {
            err = SSZ_ERR_ALIGNMENT_INVALID;
        }
        else
        {
            ssz_chunk_t *roots = opts->workspace->root_batch_roots;

            err = opts->root_batch(opts->ctx, run_start, run_len, roots);
            for (uint64_t i = 0u; (i < run_len) && (err == SSZ_SUCCESS); i++)
            {
                uint64_t index = run_start + i;

                err = ssz_merkle_cache_internal_set_leaf(cache, index, &roots[i], false);
                if ((err == SSZ_SUCCESS) && commit_tokens)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, index, true);
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }
    }
    else
    {
        for (uint64_t i = 0u; (i < run_len) && (err == SSZ_SUCCESS); i++)
        {
            uint64_t index = run_start + i;
            ssz_chunk_t root;

            err = codec->root(codec->ctx, index, &root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_merkle_cache_internal_set_leaf(cache, index, &root, false);
            }
            else
            {
                /* intentionally empty */
            }

            if ((err == SSZ_SUCCESS) && commit_tokens)
            {
                ssz_merkle_cache_internal_token_valid_set(cache, index, true);
            }
            else
            {
                /* intentionally empty */
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_sync_composite_fallback(
    ssz_merkle_cache_t *cache,
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (codec == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        uint64_t old_leaf_count = cache->leaf_count;

        if ((opts != NULL) && (opts->root_batch != NULL) && (element_count != 0u))
        {
            err = ssz_merkle_cache_internal_sync_composite_run(
                cache,
                codec,
                opts,
                0u,
                element_count,
                false);
            if (err == SSZ_SUCCESS)
            {
                ssz_merkle_cache_internal_token_valid_clear_range(cache, 0u, element_count);
            }
            else
            {
                /* intentionally empty */
            }
        }
        else
        {
            for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
            {
                ssz_chunk_t root;

                err = codec->root(codec->ctx, i, &root);
                if (err == SSZ_SUCCESS)
                {
                    err = ssz_merkle_cache_internal_set_leaf(cache, i, &root, false);
                }
                else
                {
                    /* intentionally empty */
                }

                if (err == SSZ_SUCCESS)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, i, false);
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }

        if ((err == SSZ_SUCCESS) && (element_count < old_leaf_count))
        {
            err = ssz_merkle_cache_zero_range(cache, element_count, old_leaf_count - element_count);
        }
        else if ((err == SSZ_SUCCESS) && (element_count > old_leaf_count))
        {
            cache->leaf_count = element_count;
            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_sync_composite(
    ssz_merkle_cache_t *cache,
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts)
{
    ssz_error_t err = SSZ_SUCCESS;
    bool mark_needs_resync = false;

    if (!ssz_merkle_cache_internal_cache_is_bound(cache) || (codec == NULL) ||
        (codec->root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_merkle_cache_internal_limit_matches(cache, element_limit))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_validate_sync_opts(cache, element_count, opts);
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, element_count);
    }
    else
    {
        /* intentionally empty */
    }

    if (err == SSZ_SUCCESS)
    {
        if ((opts == NULL) || (opts->token == NULL))
        {
            err = ssz_merkle_cache_internal_sync_composite_fallback(
                cache,
                element_count,
                codec,
                opts);
            if (err != SSZ_SUCCESS)
            {
                mark_needs_resync = true;
            }
            else
            {
                err = ssz_merkle_cache_set_logical_length(cache, element_count);
                if (err == SSZ_SUCCESS)
                {
                    cache->needs_resync = false;
                }
            }
        }
        else
        {
            uint64_t old_leaf_count = cache->leaf_count;
            uint64_t run_start = 0u;
            uint64_t run_len = 0u;

            for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
            {
                uint64_t token = 0u;

                err = opts->token(opts->ctx, i, &token);
                if (err != SSZ_SUCCESS)
                {
                    mark_needs_resync = true;
                }
                else
                {
                    bool unchanged = false;

                    if (i < old_leaf_count)
                    {
                        size_t token_index = 0u;

                        if (!ssz_internal_u64_to_size(i, &token_index) ||
                            (token_index >= cache->token_capacity))
                        {
                            err = SSZ_ERR_OVERFLOW;
                            mark_needs_resync = true;
                        }
                        else
                        {
                            unchanged = ssz_merkle_cache_internal_token_valid_get(cache, i) &&
                                        (cache->token_values[token_index] == token);
                        }
                    }
                    else
                    {
                        unchanged = false;
                    }

                    if ((err == SSZ_SUCCESS) && unchanged)
                    {
                        if (run_len != 0u)
                        {
                            err = ssz_merkle_cache_internal_sync_composite_run(
                                cache,
                                codec,
                                opts,
                                run_start,
                                run_len,
                                true);
                            if (err != SSZ_SUCCESS)
                            {
                                mark_needs_resync = true;
                            }
                            else
                            {
                                run_len = 0u;
                            }
                        }
                        else
                        {
                            /* intentionally empty */
                        }
                    }
                    else if (err == SSZ_SUCCESS)
                    {
                        size_t token_index = 0u;

                        if (!ssz_internal_u64_to_size(i, &token_index) ||
                            (token_index >= cache->token_capacity))
                        {
                            err = SSZ_ERR_OVERFLOW;
                            mark_needs_resync = true;
                        }
                        else
                        {
                            cache->token_values[token_index] = token;
                            ssz_merkle_cache_internal_token_valid_set(cache, i, false);

                            if (run_len == 0u)
                            {
                                run_start = i;
                                run_len = 1u;
                            }
                            else
                            {
                                run_len++;
                            }
                        }
                    }
                    else
                    {
                        /* intentionally empty */
                    }
                }
            }

            if ((err == SSZ_SUCCESS) && (run_len != 0u))
            {
                err = ssz_merkle_cache_internal_sync_composite_run(
                    cache,
                    codec,
                    opts,
                    run_start,
                    run_len,
                    true);
                if (err != SSZ_SUCCESS)
                {
                    mark_needs_resync = true;
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

            if ((err == SSZ_SUCCESS) && (element_count < old_leaf_count))
            {
                err = ssz_merkle_cache_zero_range(
                    cache,
                    element_count,
                    old_leaf_count - element_count);
                if (err != SSZ_SUCCESS)
                {
                    mark_needs_resync = true;
                }
                else
                {
                    /* intentionally empty */
                }
            }
            else if ((err == SSZ_SUCCESS) && (element_count > old_leaf_count))
            {
                cache->leaf_count = element_count;
                ssz_merkle_cache_internal_invalidate_data_root(cache);
            }
            else
            {
                /* intentionally empty */
            }

            if (err == SSZ_SUCCESS)
            {
                err = ssz_merkle_cache_set_logical_length(cache, element_count);
                if (err == SSZ_SUCCESS)
                {
                    cache->needs_resync = false;
                }
            }
            else
            {
                /* intentionally empty */
            }
        }
    }
    else
    {
        /* intentionally empty */
    }

    if ((err != SSZ_SUCCESS) && mark_needs_resync)
    {
        cache->needs_resync = true;
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

bool ssz_merkle_cache_needs_resync(const ssz_merkle_cache_t *cache)
{
    bool needs_resync = false;

    if (cache != NULL)
    {
        needs_resync = cache->needs_resync;
    }
    else
    {
        needs_resync = false;
    }

    return needs_resync;
}
