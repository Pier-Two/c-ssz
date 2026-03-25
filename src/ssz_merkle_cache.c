#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ssz_hash.h"
#include "ssz_internal.h"
#include "ssz_merkle.h"
#include "ssz_merkle_cache.h"

typedef struct
{
    uint64_t *bits;
    size_t *word_idx;
    size_t word_count;
    size_t word_capacity;
} ssz_merkle_cache_dirty_set_t;

struct ssz_merkle_cache
{
    const ssz_hash_fn_t *hash_fn;
    const ssz_chunk_t *zero_hashes;
    ssz_chunk_t zero_hashes_buf[64];

    ssz_chunk_t *nodes;
    size_t level_offsets[64];
    uint64_t leaf_capacity;
    uint32_t depth;

    uint64_t leaf_limit;
    uint64_t leaf_count;
    uint64_t logical_length;
    bool mix_in_length;

    uint64_t *leaf_dirty_bits;
    size_t *leaf_dirty_word_idx;
    size_t leaf_dirty_word_count;
    size_t leaf_dirty_word_capacity;

    ssz_merkle_cache_dirty_set_t scratch_dirty[2];

    ssz_chunk_t *scratch_pairs;
    ssz_chunk_t *scratch_hashes;
    uint64_t *scratch_parent_indices;
    size_t scratch_pair_capacity;

    uint64_t *leaf_tokens;
    uint64_t *leaf_token_valid_bits;
    size_t leaf_token_capacity;
    size_t leaf_token_word_capacity;
    bool token_storage_ready;

    bool data_root_valid;
    bool final_root_valid;
    bool needs_resync;
    ssz_chunk_t cached_data_root;
    ssz_chunk_t cached_root;
};

/* Portable 32-byte-aligned allocation for ssz_chunk_t arrays (C99). */
static void *ssz_merkle_cache_internal_alloc_aligned32(size_t size)
{
    size_t alloc_size = size;
    size_t total = 0u;
    void *aligned_ptr = NULL;
    void *raw = NULL;

    if (alloc_size == 0u)
    {
        alloc_size = 1u;
    }
    if (!ssz_internal_add_overflow_size(alloc_size, 32u + sizeof(void *), &total))
    {
        raw = malloc(total);
        if (raw != NULL)
        {
            uintptr_t aligned =
                ((uintptr_t)raw + sizeof(void *) + 31u) & ~(uintptr_t)31u; /* NOLINT(misra-c2012-11.6) */

            ((void **)aligned)[-1] = raw; /* NOLINT(misra-c2012-11.6) */
            aligned_ptr = (void *)aligned; /* NOLINT(misra-c2012-11.6) */
        }
    }

    return aligned_ptr;
}

static void ssz_merkle_cache_internal_free_aligned32(void *ptr)
{
    if (ptr != NULL)
    {
        free(((void **)ptr)[-1]); /* NOLINT(misra-c2012-11.6) */
    }
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

static size_t ssz_merkle_cache_internal_popcount_u64(uint64_t value)
{
    size_t count = 0u;
    uint64_t remaining = value;

    while (remaining != 0u)
    {
        remaining &= (remaining - 1u);
        count++;
    }
    return count;
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

static ssz_error_t ssz_merkle_cache_internal_grow_capacity(
    size_t current_capacity,
    size_t required,
    size_t *out_capacity)
{
    size_t cap = (current_capacity == 0u) ? 8u : current_capacity;
    ssz_error_t err = SSZ_SUCCESS;

    while ((cap < required) && (err == SSZ_SUCCESS))
    {
        if (cap > (SIZE_MAX / 2u))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            cap <<= 1u;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        *out_capacity = cap;
    }

    return err;
}

static int ssz_merkle_cache_internal_compare_size_t(const void *a, const void *b)
{
    const size_t va = *(const size_t *)a;
    const size_t vb = *(const size_t *)b;
    int comparison = 0;

    if (va < vb)
    {
        comparison = -1;
    }
    else if (va > vb)
    {
        comparison = 1;
    }
    else
    {
        /* intentionally empty */
    }

    return comparison;
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
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_node_count(
    uint64_t leaf_capacity,
    size_t *out_node_count)
{
    uint64_t nodes_u64 = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_node_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_mul_overflow_u64(leaf_capacity, 2u, &nodes_u64) || (nodes_u64 == 0u))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        nodes_u64 -= 1u;
        if (!ssz_internal_u64_to_size(nodes_u64, out_node_count))
        {
            err = SSZ_ERR_OVERFLOW;
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
            err = ssz_hash_2to1(hash_fn,
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
    uint64_t width = leaf_capacity;
    size_t running = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_offsets == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
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

static ssz_error_t ssz_merkle_cache_internal_allocate_nodes(
    uint64_t leaf_capacity,
    uint32_t depth,
    const size_t level_offsets[64],
    ssz_chunk_t **out_nodes)
{
    size_t node_count = 0u;
    size_t bytes = 0u;
    ssz_chunk_t *nodes = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((level_offsets == NULL) || (out_nodes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_node_count(leaf_capacity, &node_count);
        if (err == SSZ_SUCCESS)
        {
            if (ssz_internal_mul_overflow_size(node_count, sizeof(*nodes), &bytes))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                nodes = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(bytes);
                if (nodes == NULL)
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    *out_nodes = nodes;
                }
            }
        }
    }

    (void)depth;

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

static ssz_error_t ssz_merkle_cache_internal_dirty_set_init(
    ssz_merkle_cache_dirty_set_t *set,
    size_t word_capacity)
{
    uint64_t *bits = NULL;
    size_t *word_idx = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if (set == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        bits = (uint64_t *)calloc(word_capacity, sizeof(*bits));
        if (bits == NULL)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            word_idx = (size_t *)malloc(word_capacity * sizeof(*word_idx));
            if (word_idx == NULL)
            {
                free(bits);
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                set->bits = bits;
                set->word_idx = word_idx;
                set->word_count = 0u;
                set->word_capacity = word_capacity;
            }
        }
    }

    return err;
}

static void ssz_merkle_cache_internal_dirty_set_free(ssz_merkle_cache_dirty_set_t *set)
{
    if (set != NULL)
    {
        free(set->bits);
        free(set->word_idx);
        set->bits = NULL;
        set->word_idx = NULL;
        set->word_count = 0u;
        set->word_capacity = 0u;
    }
}

static void ssz_merkle_cache_internal_clear_dirty(
    uint64_t *bits,
    size_t *word_idx,
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
}

static void ssz_merkle_cache_internal_dirty_set_clear(ssz_merkle_cache_dirty_set_t *set)
{
    if (set != NULL)
    {
        ssz_merkle_cache_internal_clear_dirty(set->bits, set->word_idx, &set->word_count);
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
    uint64_t mask = 0u;
    uint64_t prior = 0u;
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
        mask = UINT64_C(1) << (bit_index & 63u);
        prior = bits[word];
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
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_dirty_set_mark(
    ssz_merkle_cache_dirty_set_t *set,
    uint64_t bit_index)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (set == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_dirty_mark_bit(
            set->bits, set->word_idx, &set->word_count, set->word_capacity, bit_index);
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

static bool ssz_merkle_cache_internal_token_valid_get(const ssz_merkle_cache_t *cache, uint64_t index)
{
    size_t word = 0u;
    uint64_t mask = 0u;
    bool is_valid = false;

    if ((cache == NULL) || !cache->token_storage_ready || (cache->leaf_token_valid_bits == NULL))
    {
        is_valid = false;
    }
    else if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        is_valid = false;
    }
    else if (word >= cache->leaf_token_word_capacity)
    {
        is_valid = false;
    }
    else
    {
        mask = UINT64_C(1) << (index & 63u);
        is_valid = (cache->leaf_token_valid_bits[word] & mask) != 0u;
    }

    return is_valid;
}

static void ssz_merkle_cache_internal_token_valid_set(
    ssz_merkle_cache_t *cache,
    uint64_t index,
    bool value)
{
    size_t word = 0u;
    uint64_t mask = 0u;

    if ((cache == NULL) || !cache->token_storage_ready || (cache->leaf_token_valid_bits == NULL))
    {
        /* intentionally empty */
    }
    else if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        /* intentionally empty */
    }
    else if (word >= cache->leaf_token_word_capacity)
    {
        /* intentionally empty */
    }
    else
    {
        mask = UINT64_C(1) << (index & 63u);
        if (value)
        {
            cache->leaf_token_valid_bits[word] |= mask;
        }
        else
        {
            cache->leaf_token_valid_bits[word] &= ~mask;
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
        err = ssz_merkle_cache_internal_dirty_mark_bit(cache->leaf_dirty_bits,
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
    else if (leaf_index >= cache->leaf_capacity)
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
    }
    if ((err == SSZ_SUCCESS) && (changed || force_dirty))
    {
        err = ssz_merkle_cache_internal_mark_leaf_dirty(cache, leaf_index);
        if (err != SSZ_SUCCESS)
        {
            /* propagate error */
        }
        else
        {
            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
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
    uint64_t tree_size = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (out_depth == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (cache->leaf_limit == SSZ_NO_LIMIT)
    {
        width = cache->leaf_count;
    }
    else
    {
        if (cache->leaf_count > cache->leaf_limit)
        {
            err = SSZ_ERR_LIMIT_EXCEEDED;
        }
        else
        {
            width = cache->leaf_limit;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        tree_size = ssz_next_pow_of_two(width);
        if (tree_size == 0u)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            *out_depth = ssz_merkle_cache_internal_log2_u64(tree_size);
        }
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
    if (err == SSZ_SUCCESS)
    {
        cache->cached_data_root = cache->nodes[cache->level_offsets[data_depth]];
        cache->data_root_valid = true;
        cache->final_root_valid = false;
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_gather_capacity(
    ssz_merkle_cache_t *cache,
    size_t pair_capacity)
{
    ssz_chunk_t *new_pairs = NULL;
    ssz_chunk_t *new_hashes = NULL;
    uint64_t *new_parent_indices = NULL;
    size_t pairs_bytes = 0u;
    size_t hashes_bytes = 0u;
    size_t indices_bytes = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (pair_capacity <= cache->scratch_pair_capacity)
    {
        err = SSZ_SUCCESS;
    }
    else if (ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_parent_indices), &indices_bytes) ||
             ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_hashes), &hashes_bytes) ||
             ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_pairs) * 2u, &pairs_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        new_pairs = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(pairs_bytes);
        if (new_pairs == NULL)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            new_hashes = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(hashes_bytes);
            if (new_hashes == NULL)
            {
                ssz_merkle_cache_internal_free_aligned32(new_pairs);
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                new_parent_indices = (uint64_t *)malloc(indices_bytes);
                if (new_parent_indices == NULL)
                {
                    ssz_merkle_cache_internal_free_aligned32(new_pairs);
                    ssz_merkle_cache_internal_free_aligned32(new_hashes);
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    ssz_merkle_cache_internal_free_aligned32(cache->scratch_pairs);
                    ssz_merkle_cache_internal_free_aligned32(cache->scratch_hashes);
                    free(cache->scratch_parent_indices);

                    cache->scratch_pairs = new_pairs;
                    cache->scratch_hashes = new_hashes;
                    cache->scratch_parent_indices = new_parent_indices;
                    cache->scratch_pair_capacity = pair_capacity;
                }
            }
        }
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
    else if (!ssz_internal_u64_to_size(run_start, &run_start_sz) ||
             !ssz_internal_u64_to_size(run_len, &run_len_sz))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (run_len >= 2u)
    {
        err = ssz_hash_2to1_batch(cache->hash_fn,
                                  &child_level_nodes[run_start_sz * 2u],
                                  run_len_sz,
                                  &parent_level_nodes[run_start_sz]);
    }
    else if (*io_gather_count >= cache->scratch_pair_capacity)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        cache->scratch_parent_indices[*io_gather_count] = run_start;
        cache->scratch_pairs[*io_gather_count * 2u] = child_level_nodes[run_start_sz * 2u];
        cache->scratch_pairs[((*io_gather_count) * 2u) + 1u] =
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
    size_t dirty_count = 0u;
    size_t gather_count = 0u;
    bool run_active = false;
    uint64_t run_start = 0u;
    uint64_t run_prev = 0u;
    uint64_t run_len = 0u;
    ssz_chunk_t *parent_level_nodes = NULL;
    const ssz_chunk_t *child_level_nodes = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (dirty_parents == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (dirty_parents->word_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else
    {
        qsort(dirty_parents->word_idx,
              dirty_parents->word_count,
              sizeof(dirty_parents->word_idx[0]),
              ssz_merkle_cache_internal_compare_size_t);

        for (size_t wi = 0u; (wi < dirty_parents->word_count) && (err == SSZ_SUCCESS); wi++)
        {
            size_t word = dirty_parents->word_idx[wi];
            if (word >= dirty_parents->word_capacity)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                dirty_count += ssz_merkle_cache_internal_popcount_u64(dirty_parents->bits[word]);
            }
        }

        if ((err == SSZ_SUCCESS) && (dirty_count != 0u))
        {
            err = ssz_merkle_cache_internal_ensure_gather_capacity(cache, dirty_count);
        }
        if ((err == SSZ_SUCCESS) && (dirty_count != 0u))
        {
            child_level_nodes = &cache->nodes[cache->level_offsets[level]];
            parent_level_nodes = &cache->nodes[cache->level_offsets[level + 1u]];

            for (size_t wi = 0u; (wi < dirty_parents->word_count) && (err == SSZ_SUCCESS); wi++)
            {
                size_t word_index = dirty_parents->word_idx[wi];
                uint64_t word_bits = dirty_parents->bits[word_index];
                uint64_t base = 0u;

                if (!ssz_internal_u64_to_size((uint64_t)word_index << 6u, NULL))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    base = (uint64_t)word_index << 6u;

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
                            }
                        }
                        word_bits &= (word_bits - 1u);
                    }
                }
            }

            if (run_active && (err == SSZ_SUCCESS))
            {
                err = ssz_merkle_cache_internal_flush_contiguous_run(
                    cache, child_level_nodes, parent_level_nodes, run_start, run_len, &gather_count);
            }

            if ((err == SSZ_SUCCESS) && (gather_count != 0u))
            {
                err = ssz_hash_2to1_batch(
                    cache->hash_fn, cache->scratch_pairs, gather_count, cache->scratch_hashes);
                if (err == SSZ_SUCCESS)
                {
                    for (size_t i = 0u; (i < gather_count) && (err == SSZ_SUCCESS); i++)
                    {
                        size_t parent_index = 0u;
                        if (!ssz_internal_u64_to_size(cache->scratch_parent_indices[i], &parent_index))
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                        else
                        {
                            parent_level_nodes[parent_index] = cache->scratch_hashes[i];
                        }
                    }
                }
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_build_parent_dirty_set(
    size_t *child_word_idx,
    size_t child_word_count,
    uint64_t *child_bits,
    size_t child_word_capacity,
    uint64_t child_width,
    ssz_merkle_cache_dirty_set_t *out_parent_set)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((child_word_idx == NULL) || (child_bits == NULL) || (out_parent_set == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (child_word_count == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else
    {
        qsort(child_word_idx,
              child_word_count,
              sizeof(child_word_idx[0]),
              ssz_merkle_cache_internal_compare_size_t);

        for (size_t wi = 0u; (wi < child_word_count) && (err == SSZ_SUCCESS); wi++)
        {
            size_t word_index = child_word_idx[wi];
            uint64_t word_bits = 0u;
            uint64_t base = 0u;

            if (word_index >= child_word_capacity)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                word_bits = child_bits[word_index];
                base = (uint64_t)word_index << 6u;

                while ((word_bits != 0u) && (err == SSZ_SUCCESS))
                {
                    unsigned bit = ssz_merkle_cache_internal_ctz_u64(word_bits);
                    uint64_t child_index = base + (uint64_t)bit;
                    if (child_index < child_width)
                    {
                        err = ssz_merkle_cache_internal_dirty_set_mark(out_parent_set, child_index >> 1u);
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
    uint64_t current_width = 0u;
    bool current_is_leaf = true;
    uint64_t *current_bits = NULL;
    size_t *current_word_idx = NULL;
    size_t current_word_count = 0u;
    size_t current_word_capacity = 0u;
    ssz_merkle_cache_dirty_set_t *current_scratch = NULL;
    ssz_error_t err = SSZ_SUCCESS;

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
            cache->leaf_dirty_bits, cache->leaf_dirty_word_idx, &cache->leaf_dirty_word_count);
    }
    else
    {
        current_bits = cache->leaf_dirty_bits;
        current_word_idx = cache->leaf_dirty_word_idx;
        current_word_count = cache->leaf_dirty_word_count;
        current_word_capacity = cache->leaf_dirty_word_capacity;
        current_width = cache->leaf_capacity;

        for (uint32_t level = 0u; (level < cache->depth) && (err == SSZ_SUCCESS); level++)
        {
            uint64_t parent_width = current_width >> 1u;
            ssz_merkle_cache_dirty_set_t *next_set = &cache->scratch_dirty[level & 1u];

            ssz_merkle_cache_internal_dirty_set_clear(next_set);

            if (current_word_count != 0u)
            {
                err = ssz_merkle_cache_internal_build_parent_dirty_set(current_word_idx,
                                                                       current_word_count,
                                                                       current_bits,
                                                                       current_word_capacity,
                                                                       current_width,
                                                                       next_set);
            }

            if ((err == SSZ_SUCCESS) && (next_set->word_count != 0u))
            {
                err = ssz_merkle_cache_internal_hash_dirty_parents_exact(
                    cache, level, next_set, parent_width);
            }

            if (err == SSZ_SUCCESS)
            {
                if (current_is_leaf)
                {
                    ssz_merkle_cache_internal_clear_dirty(
                        cache->leaf_dirty_bits, cache->leaf_dirty_word_idx, &cache->leaf_dirty_word_count);
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
                current_bits = next_set->bits;
                current_word_idx = next_set->word_idx;
                current_word_count = next_set->word_count;
                current_word_capacity = next_set->word_capacity;
                current_width = parent_width;
            }
        }

        if (current_scratch != NULL)
        {
            ssz_merkle_cache_internal_dirty_set_clear(current_scratch);
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_token_storage(ssz_merkle_cache_t *cache)
{
    uint64_t *new_tokens = NULL;
    uint64_t *new_valid_bits = NULL;
    size_t requested_token_capacity = 0u;
    size_t requested_word_capacity = 0u;
    size_t copy_tokens = 0u;
    size_t copy_words = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_u64_to_size(cache->leaf_capacity, &requested_token_capacity))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        requested_word_capacity = cache->leaf_dirty_word_capacity;
        if (cache->token_storage_ready &&
            (cache->leaf_token_capacity == requested_token_capacity) &&
            (cache->leaf_token_word_capacity == requested_word_capacity))
        {
            err = SSZ_SUCCESS;
        }
        else
        {
            new_tokens = (uint64_t *)calloc(requested_token_capacity, sizeof(*new_tokens));
            if (new_tokens == NULL)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                new_valid_bits = (uint64_t *)calloc(requested_word_capacity, sizeof(*new_valid_bits));
                if (new_valid_bits == NULL)
                {
                    free(new_tokens);
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    if (cache->token_storage_ready)
                    {
                        copy_tokens = cache->leaf_token_capacity;
                        copy_words = cache->leaf_token_word_capacity;
                        if (copy_tokens > requested_token_capacity)
                        {
                            copy_tokens = requested_token_capacity;
                        }
                        if (copy_words > requested_word_capacity)
                        {
                            copy_words = requested_word_capacity;
                        }
                        if ((copy_tokens != 0u) && (cache->leaf_tokens != NULL))
                        {
                            (void)memcpy(new_tokens, cache->leaf_tokens, copy_tokens * sizeof(*new_tokens));
                        }
                        if ((copy_words != 0u) && (cache->leaf_token_valid_bits != NULL))
                        {
                            (void)memcpy(new_valid_bits,
                                         cache->leaf_token_valid_bits,
                                         copy_words * sizeof(*new_valid_bits));
                        }
                    }

                    free(cache->leaf_tokens);
                    free(cache->leaf_token_valid_bits);
                    cache->leaf_tokens = new_tokens;
                    cache->leaf_token_valid_bits = new_valid_bits;
                    cache->leaf_token_capacity = requested_token_capacity;
                    cache->leaf_token_word_capacity = requested_word_capacity;
                    cache->token_storage_ready = true;
                }
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_resize_token_storage(
    const ssz_merkle_cache_t *old_cache,
    uint64_t new_capacity,
    size_t new_word_capacity,
    uint64_t **out_tokens,
    uint64_t **out_valid_bits,
    size_t *out_token_capacity,
    size_t *out_token_word_capacity)
{
    size_t new_token_capacity = 0u;
    size_t token_bytes = 0u;
    size_t valid_bytes = 0u;
    uint64_t *new_tokens = NULL;
    uint64_t *new_valid_bits = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((old_cache == NULL) || (out_tokens == NULL) || (out_valid_bits == NULL) ||
        (out_token_capacity == NULL) || (out_token_word_capacity == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!old_cache->token_storage_ready)
    {
        *out_tokens = NULL;
        *out_valid_bits = NULL;
        *out_token_capacity = 0u;
        *out_token_word_capacity = 0u;
    }
    else if (!ssz_internal_u64_to_size(new_capacity, &new_token_capacity))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size(new_token_capacity, sizeof(*new_tokens), &token_bytes) ||
             ssz_internal_mul_overflow_size(new_word_capacity, sizeof(*new_valid_bits), &valid_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        new_tokens = (uint64_t *)calloc(new_token_capacity, sizeof(*new_tokens));
        if (new_tokens == NULL)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            new_valid_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_valid_bits));
            if (new_valid_bits == NULL)
            {
                free(new_tokens);
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                size_t copy_tokens = old_cache->leaf_token_capacity;
                size_t copy_words = old_cache->leaf_token_word_capacity;

                if (copy_tokens > new_token_capacity)
                {
                    copy_tokens = new_token_capacity;
                }
                if (copy_words > new_word_capacity)
                {
                    copy_words = new_word_capacity;
                }
                if ((copy_tokens != 0u) && (old_cache->leaf_tokens != NULL))
                {
                    (void)memcpy(new_tokens, old_cache->leaf_tokens, copy_tokens * sizeof(*new_tokens));
                }
                if ((copy_words != 0u) && (old_cache->leaf_token_valid_bits != NULL))
                {
                    (void)memcpy(new_valid_bits,
                                 old_cache->leaf_token_valid_bits,
                                 copy_words * sizeof(*new_valid_bits));
                }

                *out_tokens = new_tokens;
                *out_valid_bits = new_valid_bits;
                *out_token_capacity = new_token_capacity;
                *out_token_word_capacity = new_word_capacity;
            }
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_grow(ssz_merkle_cache_t *cache, uint64_t new_capacity)
{
    ssz_chunk_t *new_nodes = NULL;
    size_t new_offsets[64];
    uint32_t new_depth = 0u;
    size_t new_word_capacity = 0u;
    uint64_t *new_leaf_dirty_bits = NULL;
    size_t *new_leaf_dirty_word_idx = NULL;
    uint64_t *new_scratch0_bits = NULL;
    size_t *new_scratch0_idx = NULL;
    uint64_t *new_scratch1_bits = NULL;
    size_t *new_scratch1_idx = NULL;
    uint64_t *new_tokens = NULL;
    uint64_t *new_token_valid_bits = NULL;
    size_t new_token_capacity = 0u;
    size_t new_token_word_capacity = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (new_capacity <= cache->leaf_capacity)
    {
        err = SSZ_SUCCESS;
    }
    else
    {
        (void)memset(new_offsets, 0, sizeof(new_offsets));
        new_depth = ssz_merkle_cache_internal_log2_u64(new_capacity);
        assert(new_depth < 64u);

        err = ssz_merkle_cache_internal_compute_level_offsets(new_capacity, new_depth, new_offsets);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_internal_allocate_nodes(new_capacity, new_depth, new_offsets, &new_nodes);
        }

        if (err == SSZ_SUCCESS)
        {
            uint64_t width = new_capacity;

            for (uint32_t level = 0u; (level <= new_depth) && (err == SSZ_SUCCESS); level++)
            {
                size_t width_sz = 0u;

                if (!ssz_internal_u64_to_size(width, &width_sz))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    ssz_chunk_t *new_level_nodes = &new_nodes[new_offsets[level]];
                    for (size_t i = 0u; i < width_sz; i++)
                    {
                        new_level_nodes[i] = cache->zero_hashes[level];
                    }

                    if (level <= cache->depth)
                    {
                        uint64_t old_width = cache->leaf_capacity >> level;
                        size_t old_width_sz = 0u;

                        if (!ssz_internal_u64_to_size(old_width, &old_width_sz))
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                        else
                        {
                            (void)memcpy(new_level_nodes,
                                         &cache->nodes[cache->level_offsets[level]],
                                         old_width_sz * sizeof(*new_level_nodes));
                        }
                    }

                    width >>= 1u;
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_internal_leaf_words_for_capacity(new_capacity, &new_word_capacity);
        }
        if (err == SSZ_SUCCESS)
        {
            new_leaf_dirty_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_leaf_dirty_bits));
            if (new_leaf_dirty_bits == NULL)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                new_leaf_dirty_word_idx = (size_t *)malloc(new_word_capacity * sizeof(*new_leaf_dirty_word_idx));
                if (new_leaf_dirty_word_idx == NULL)
                {
                    err = SSZ_ERR_OVERFLOW;
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            (void)memcpy(new_leaf_dirty_bits,
                         cache->leaf_dirty_bits,
                         cache->leaf_dirty_word_capacity * sizeof(*new_leaf_dirty_bits));
            (void)memcpy(new_leaf_dirty_word_idx,
                         cache->leaf_dirty_word_idx,
                         cache->leaf_dirty_word_count * sizeof(*new_leaf_dirty_word_idx));

            new_scratch0_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_scratch0_bits));
            new_scratch0_idx = (size_t *)malloc(new_word_capacity * sizeof(*new_scratch0_idx));
            new_scratch1_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_scratch1_bits));
            new_scratch1_idx = (size_t *)malloc(new_word_capacity * sizeof(*new_scratch1_idx));
            if ((new_scratch0_bits == NULL) || (new_scratch0_idx == NULL) ||
                (new_scratch1_bits == NULL) || (new_scratch1_idx == NULL))
            {
                err = SSZ_ERR_OVERFLOW;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_internal_resize_token_storage(cache,
                                                                 new_capacity,
                                                                 new_word_capacity,
                                                                 &new_tokens,
                                                                 &new_token_valid_bits,
                                                                 &new_token_capacity,
                                                                 &new_token_word_capacity);
        }

        if (err == SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_free_aligned32(cache->nodes);
            cache->nodes = new_nodes;
            (void)memcpy(cache->level_offsets, new_offsets, sizeof(new_offsets));
            cache->leaf_capacity = new_capacity;
            cache->depth = new_depth;

            free(cache->leaf_dirty_bits);
            free(cache->leaf_dirty_word_idx);
            cache->leaf_dirty_bits = new_leaf_dirty_bits;
            cache->leaf_dirty_word_idx = new_leaf_dirty_word_idx;
            cache->leaf_dirty_word_capacity = new_word_capacity;

            ssz_merkle_cache_internal_dirty_set_free(&cache->scratch_dirty[0]);
            ssz_merkle_cache_internal_dirty_set_free(&cache->scratch_dirty[1]);
            cache->scratch_dirty[0].bits = new_scratch0_bits;
            cache->scratch_dirty[0].word_idx = new_scratch0_idx;
            cache->scratch_dirty[0].word_count = 0u;
            cache->scratch_dirty[0].word_capacity = new_word_capacity;
            cache->scratch_dirty[1].bits = new_scratch1_bits;
            cache->scratch_dirty[1].word_idx = new_scratch1_idx;
            cache->scratch_dirty[1].word_count = 0u;
            cache->scratch_dirty[1].word_capacity = new_word_capacity;

            if (cache->token_storage_ready)
            {
                free(cache->leaf_tokens);
                free(cache->leaf_token_valid_bits);
                cache->leaf_tokens = new_tokens;
                cache->leaf_token_valid_bits = new_token_valid_bits;
                cache->leaf_token_capacity = new_token_capacity;
                cache->leaf_token_word_capacity = new_token_word_capacity;
            }

            ssz_merkle_cache_internal_invalidate_data_root(cache);
        }
        else
        {
            ssz_merkle_cache_internal_free_aligned32(new_nodes);
            free(new_leaf_dirty_bits);
            free(new_leaf_dirty_word_idx);
            free(new_scratch0_bits);
            free(new_scratch0_idx);
            free(new_scratch1_bits);
            free(new_scratch1_idx);
            free(new_tokens);
            free(new_token_valid_bits);
        }
    }

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_capacity_for_count(
    ssz_merkle_cache_t *cache,
    uint64_t required_count)
{
    uint64_t target_capacity = 0u;
    uint64_t old_capacity = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (required_count <= cache->leaf_capacity)
    {
        err = SSZ_SUCCESS;
    }
    else if ((cache->leaf_limit != SSZ_NO_LIMIT) && (required_count > cache->leaf_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        old_capacity = cache->leaf_capacity;
        target_capacity = cache->leaf_capacity;
        while ((target_capacity < required_count) && (err == SSZ_SUCCESS))
        {
            if (target_capacity > (UINT64_MAX / 2u))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                target_capacity <<= 1u;
            }
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_merkle_cache_internal_grow(cache, target_capacity);
        }

        if (err == SSZ_SUCCESS)
        {
            /* A capacity step introduces a new right side and top chain, even if new
               leaves are still zero. Force one right-side path dirty so the root chain
               is refreshed without rebuilding the preserved left subtree. */
            err = ssz_merkle_cache_internal_set_leaf(
                cache, old_capacity, &cache->zero_hashes[0], true);
        }
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

ssz_error_t ssz_merkle_cache_create(
    const ssz_merkle_cache_config_t *config,
    ssz_merkle_cache_t **out_cache)
{
    ssz_merkle_cache_t *cache = NULL;
    uint64_t initial_capacity = 0u;
    size_t leaf_words = 0u;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((config == NULL) || (out_cache == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((config->leaf_limit != SSZ_NO_LIMIT) && (config->initial_leaf_count > config->leaf_limit))
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
    }

    if (err == SSZ_SUCCESS)
    {
        if (config->leaf_limit == SSZ_NO_LIMIT)
        {
            initial_capacity = ssz_next_pow_of_two(config->initial_leaf_count);
        }
        else
        {
            initial_capacity = ssz_next_pow_of_two(config->leaf_limit);
        }
        if (initial_capacity == 0u)
        {
            err = SSZ_ERR_OVERFLOW;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        cache = (ssz_merkle_cache_t *)calloc(1u, sizeof(*cache));
        if (cache == NULL)
        {
            err = SSZ_ERR_OVERFLOW;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        cache->hash_fn = resolved_hash_fn;
        cache->leaf_limit = config->leaf_limit;
        cache->leaf_count = config->initial_leaf_count;
        cache->logical_length = config->logical_length;
        cache->mix_in_length = config->mix_in_length;
        cache->leaf_capacity = initial_capacity;
        cache->depth = ssz_merkle_cache_internal_log2_u64(initial_capacity);
        assert(cache->depth < 64u);

        if (resolved_hash_fn == ssz_hash_default())
        {
            cache->zero_hashes = ssz_hash_default_zero_hashes();
        }
        else
        {
            err = ssz_merkle_cache_internal_build_zero_hashes(resolved_hash_fn, cache->zero_hashes_buf);
            if (err == SSZ_SUCCESS)
            {
                cache->zero_hashes = cache->zero_hashes_buf;
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_compute_level_offsets(
            cache->leaf_capacity, cache->depth, cache->level_offsets);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_allocate_nodes(
            cache->leaf_capacity, cache->depth, cache->level_offsets, &cache->nodes);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_leaf_words_for_capacity(cache->leaf_capacity, &leaf_words);
        if (err == SSZ_SUCCESS)
        {
            cache->leaf_dirty_word_capacity = leaf_words;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        cache->leaf_dirty_bits = (uint64_t *)calloc(leaf_words, sizeof(*cache->leaf_dirty_bits));
        cache->leaf_dirty_word_idx = (size_t *)malloc(leaf_words * sizeof(*cache->leaf_dirty_word_idx));
        if ((cache->leaf_dirty_bits == NULL) || (cache->leaf_dirty_word_idx == NULL))
        {
            err = SSZ_ERR_OVERFLOW;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        ssz_error_t d0_err =
            ssz_merkle_cache_internal_dirty_set_init(&cache->scratch_dirty[0], leaf_words);
        ssz_error_t d1_err =
            ssz_merkle_cache_internal_dirty_set_init(&cache->scratch_dirty[1], leaf_words);

        if ((d0_err != SSZ_SUCCESS) || (d1_err != SSZ_SUCCESS))
        {
            err = (d0_err != SSZ_SUCCESS) ? d0_err : d1_err;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        ssz_merkle_cache_internal_fill_zero_tree(cache);
        cache->data_root_valid = false;
        cache->final_root_valid = false;
        cache->needs_resync = false;

        {
            ssz_error_t root_err = ssz_merkle_cache_internal_refresh_cached_data_root(cache);
            assert(root_err == SSZ_SUCCESS);
            (void)root_err;
        }

        if (cache->mix_in_length)
        {
            err = ssz_mix_in_length_u64(
                &cache->cached_data_root, cache->logical_length, cache->hash_fn, &cache->cached_root);
            if (err == SSZ_SUCCESS)
            {
                cache->final_root_valid = true;
            }
        }
        else
        {
            cache->cached_root = cache->cached_data_root;
            cache->final_root_valid = true;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_cache = cache;
    }
    else
    {
        ssz_merkle_cache_destroy(cache);
    }

    return err;
}

void ssz_merkle_cache_destroy(ssz_merkle_cache_t *cache)
{
    if (cache != NULL)
    {
        ssz_merkle_cache_internal_free_aligned32(cache->nodes);
        cache->nodes = NULL;

        free(cache->leaf_dirty_bits);
        free(cache->leaf_dirty_word_idx);
        cache->leaf_dirty_bits = NULL;
        cache->leaf_dirty_word_idx = NULL;

        ssz_merkle_cache_internal_dirty_set_free(&cache->scratch_dirty[0]);
        ssz_merkle_cache_internal_dirty_set_free(&cache->scratch_dirty[1]);

        ssz_merkle_cache_internal_free_aligned32(cache->scratch_pairs);
        ssz_merkle_cache_internal_free_aligned32(cache->scratch_hashes);
        free(cache->scratch_parent_indices);

        free(cache->leaf_tokens);
        free(cache->leaf_token_valid_bits);

        free(cache);
    }
}

ssz_error_t ssz_merkle_cache_reset(ssz_merkle_cache_t *cache)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_merkle_cache_internal_fill_zero_tree(cache);

        ssz_merkle_cache_internal_clear_dirty(
            cache->leaf_dirty_bits, cache->leaf_dirty_word_idx, &cache->leaf_dirty_word_count);
        ssz_merkle_cache_internal_dirty_set_clear(&cache->scratch_dirty[0]);
        ssz_merkle_cache_internal_dirty_set_clear(&cache->scratch_dirty[1]);

        if (cache->token_storage_ready)
        {
            (void)memset(cache->leaf_tokens, 0, cache->leaf_token_capacity * sizeof(*cache->leaf_tokens));
            (void)memset(cache->leaf_token_valid_bits,
                         0,
                         cache->leaf_token_word_capacity * sizeof(*cache->leaf_token_valid_bits));
        }

        cache->leaf_count = 0u;
        cache->logical_length = 0u;
        cache->needs_resync = false;
        cache->data_root_valid = false;
        cache->final_root_valid = false;

        err = ssz_merkle_cache_internal_refresh_cached_data_root(cache);
    }

    return err;
}

ssz_error_t ssz_merkle_cache_data_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_merkle_cache_internal_compute_data_root_if_needed(cache);
        if (err == SSZ_SUCCESS)
        {
            *out_root = cache->cached_data_root;
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
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
                    &cache->cached_data_root, cache->logical_length, cache->hash_fn, &cache->cached_root);
                if (err == SSZ_SUCCESS)
                {
                    cache->final_root_valid = true;
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

    if (cache == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((root_count != 0u) && (roots == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_internal_add_overflow_u64(start_index, root_count, &end_index))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((cache->leaf_limit != SSZ_NO_LIMIT) && (end_index > cache->leaf_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, end_index);
        if (err == SSZ_SUCCESS)
        {
            for (uint64_t i = 0u; (i < root_count) && (err == SSZ_SUCCESS); i++)
            {
                uint64_t index = start_index + i;

                err = ssz_merkle_cache_internal_set_leaf(cache, index, &roots[i], false);
                if (err == SSZ_SUCCESS)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, index, false);
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            if (end_index > cache->leaf_count)
            {
                cache->leaf_count = end_index;
                ssz_merkle_cache_internal_invalidate_data_root(cache);
            }

            cache->needs_resync = false;
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_zero_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    uint64_t zero_count)
{
    uint64_t end_index = 0u;
    uint64_t max_index = 0u;
    ssz_chunk_t zero_leaf;
    uint64_t old_leaf_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&zero_leaf, 0, sizeof(zero_leaf));
    if (cache == NULL)
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
    else if ((cache->leaf_limit != SSZ_NO_LIMIT) && (end_index > cache->leaf_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else
    {
        old_leaf_count = cache->leaf_count;
        max_index = end_index;
        if (max_index > cache->leaf_capacity)
        {
            max_index = cache->leaf_capacity;
        }

        for (uint64_t i = start_index; (i < max_index) && (err == SSZ_SUCCESS); i++)
        {
            err = ssz_merkle_cache_internal_set_leaf(cache, i, &zero_leaf, false);
        }

        if (err == SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_token_valid_clear_range(cache, start_index, max_index - start_index);

            if ((start_index < old_leaf_count) && (end_index >= old_leaf_count))
            {
                cache->leaf_count = start_index;
                ssz_merkle_cache_internal_invalidate_data_root(cache);
            }

            cache->needs_resync = false;
        }
    }

    return err;
}

ssz_error_t ssz_merkle_cache_set_logical_length(
    ssz_merkle_cache_t *cache,
    uint64_t logical_length)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (cache == NULL)
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

    if (cache == NULL)
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

    if ((err == SSZ_SUCCESS) && (cache->leaf_limit != SSZ_NO_LIMIT) && (chunk_count > cache->leaf_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, chunk_count);
    }

    if (err == SSZ_SUCCESS)
    {
        old_leaf_count = cache->leaf_count;
        for (uint64_t chunk_index = 0u; (chunk_index < chunk_count) && (err == SSZ_SUCCESS); chunk_index++)
        {
            ssz_chunk_t leaf;
            size_t chunk_offset = 0u;
            size_t copy_len = SSZ_BYTES_PER_CHUNK;

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
                    size_t remaining = bytes_len - chunk_offset;
                    if (remaining < copy_len)
                    {
                        copy_len = remaining;
                    }
                    (void)memcpy(leaf.bytes, &bytes[chunk_offset], copy_len);
                }

                err = ssz_merkle_cache_internal_set_leaf(cache, chunk_index, &leaf, false);
                if (err == SSZ_SUCCESS)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, chunk_index, false);
                }
            }
        }
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
        ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, logical_length);
        assert(len_err == SSZ_SUCCESS);
        (void)len_err;

        cache->needs_resync = false;
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

    if (cache == NULL)
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
        err = ssz_merkle_cache_sync_packed_bytes(cache, elements, total_bytes, cache->logical_length);
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

    if (cache == NULL)
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

            if (ssz_internal_mul_overflow_u64(element_limit, (uint64_t)element_size, &limit_bytes) ||
                ssz_internal_add_overflow_u64(limit_bytes, SSZ_BYTES_PER_CHUNK - 1u, &chunk_limit))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                chunk_limit /= SSZ_BYTES_PER_CHUNK;
            }
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

    if (cache == NULL)
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
        if ((err == SSZ_SUCCESS) &&
            ssz_internal_add_overflow_u64(bit_count, 255u, &chunk_limit))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        if (err == SSZ_SUCCESS)
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
                err = ssz_merkle_cache_sync_packed_bytes(cache, bits_le, bitfield_bytes, cache->logical_length);
            }
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

    if (cache == NULL)
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
    const uint64_t *run_tokens)
{
    ssz_error_t err = SSZ_SUCCESS;
    ssz_chunk_t *tmp_roots = NULL;

    if ((cache == NULL) || (codec == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (run_len == 0u)
    {
        err = SSZ_SUCCESS;
    }
    else if ((opts != NULL) && (opts->root_batch != NULL))
    {
        size_t run_len_sz = 0u;
        size_t tmp_bytes = 0u;

        if (!ssz_internal_u64_to_size(run_len, &run_len_sz))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else if (ssz_internal_mul_overflow_size(run_len_sz, sizeof(*tmp_roots), &tmp_bytes))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            tmp_roots = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(tmp_bytes);
            if (tmp_roots == NULL)
            {
                err = SSZ_ERR_OVERFLOW;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = opts->root_batch(opts->ctx, run_start, run_len, tmp_roots);
        }

        for (uint64_t i = 0u; (i < run_len) && (err == SSZ_SUCCESS); i++)
        {
            uint64_t index = run_start + i;

            err = ssz_merkle_cache_internal_set_leaf(cache, index, &tmp_roots[i], false);
            if ((err == SSZ_SUCCESS) && cache->token_storage_ready)
            {
                size_t token_index = 0u;

                if (!ssz_internal_u64_to_size(index, &token_index) ||
                    (token_index >= cache->leaf_token_capacity))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    if (run_tokens != NULL)
                    {
                        cache->leaf_tokens[token_index] = run_tokens[i];
                    }
                    ssz_merkle_cache_internal_token_valid_set(cache, index, true);
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
            if ((err == SSZ_SUCCESS) && cache->token_storage_ready)
            {
                size_t token_index = 0u;

                if (!ssz_internal_u64_to_size(index, &token_index) ||
                    (token_index >= cache->leaf_token_capacity))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    if (run_tokens != NULL)
                    {
                        cache->leaf_tokens[token_index] = run_tokens[i];
                    }
                    ssz_merkle_cache_internal_token_valid_set(cache, index, true);
                }
            }
        }
    }

    ssz_merkle_cache_internal_free_aligned32(tmp_roots);

    return err;
}

static ssz_error_t ssz_merkle_cache_internal_sync_composite_fallback(
    ssz_merkle_cache_t *cache,
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts)
{
    uint64_t old_leaf_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((cache == NULL) || (codec == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        old_leaf_count = cache->leaf_count;
        if ((opts != NULL) && (opts->root_batch != NULL) && (element_count != 0u))
        {
            err = ssz_merkle_cache_internal_sync_composite_run(
                cache, codec, opts, 0u, element_count, cache->leaf_tokens);
            if (err == SSZ_SUCCESS)
            {
                ssz_merkle_cache_internal_token_valid_clear_range(cache, 0u, element_count);
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
                if (err == SSZ_SUCCESS)
                {
                    ssz_merkle_cache_internal_token_valid_set(cache, i, false);
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
    uint64_t old_leaf_count = 0u;
    uint64_t run_start = 0u;
    uint64_t run_len = 0u;
    uint64_t *run_tokens = NULL;
    size_t run_tokens_cap = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    bool mark_needs_resync = false;

    if ((cache == NULL) || (codec == NULL) || (codec->root == NULL))
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
        err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, element_count);
    }

    if (err == SSZ_SUCCESS)
    {
        old_leaf_count = cache->leaf_count;

        if ((opts == NULL) || (opts->token == NULL))
        {
            err = ssz_merkle_cache_internal_sync_composite_fallback(cache, element_count, codec, opts);
            if (err != SSZ_SUCCESS)
            {
                mark_needs_resync = true;
            }
            else
            {
                ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, element_count);
                assert(len_err == SSZ_SUCCESS);
                (void)len_err;
                cache->needs_resync = false;
            }
        }
        else
        {
            err = ssz_merkle_cache_internal_ensure_token_storage(cache);
            if (err == SSZ_SUCCESS)
            {
                for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
                {
                    uint64_t token = 0u;
                    bool unchanged = false;

                    err = opts->token(opts->ctx, i, &token);
                    if (err != SSZ_SUCCESS)
                    {
                        mark_needs_resync = true;
                    }
                    else
                    {
                        if (i < old_leaf_count)
                        {
                            size_t i_sz = 0u;

                            if (!ssz_internal_u64_to_size(i, &i_sz))
                            {
                                err = SSZ_ERR_OVERFLOW;
                                mark_needs_resync = true;
                            }
                            else
                            {
                                unchanged = ssz_merkle_cache_internal_token_valid_get(cache, i) &&
                                            (cache->leaf_tokens[i_sz] == token);
                            }
                        }

                        if ((err == SSZ_SUCCESS) && unchanged)
                        {
                            if (run_len != 0u)
                            {
                                err = ssz_merkle_cache_internal_sync_composite_run(
                                    cache, codec, opts, run_start, run_len, run_tokens);
                                if (err != SSZ_SUCCESS)
                                {
                                    mark_needs_resync = true;
                                }
                                else
                                {
                                    run_len = 0u;
                                }
                            }
                        }
                        else if (err == SSZ_SUCCESS)
                        {
                            if (run_len == 0u)
                            {
                                run_start = i;
                                run_len = 1u;
                            }
                            else if (i == (run_start + run_len))
                            {
                                run_len++;
                            }
                            else
                            {
                                /* intentionally empty */
                            }

                            if (run_len > run_tokens_cap)
                            {
                                size_t new_cap = 0u;
                                ssz_error_t cap_err =
                                    ssz_merkle_cache_internal_grow_capacity(run_tokens_cap, (size_t)run_len, &new_cap);

                                assert(cap_err == SSZ_SUCCESS); /* LCOV_EXCL_LINE */
                                (void)cap_err;

                                {
                                    uint64_t *new_tokens =
                                        (uint64_t *)realloc(run_tokens, new_cap * sizeof(*new_tokens));
                                    if (new_tokens == NULL)
                                    {
                                        err = SSZ_ERR_OVERFLOW;
                                        mark_needs_resync = true;
                                    }
                                    else
                                    {
                                        run_tokens = new_tokens;
                                        run_tokens_cap = new_cap;
                                    }
                                }
                            }

                            if (err == SSZ_SUCCESS)
                            {
                                run_tokens[run_len - 1u] = token;
                            }
                        }
                        else
                        {
                            /* intentionally empty */
                        }
                    }
                }
            }

            if ((err == SSZ_SUCCESS) && (run_len != 0u))
            {
                err = ssz_merkle_cache_internal_sync_composite_run(cache, codec, opts, run_start, run_len, run_tokens);
                if (err != SSZ_SUCCESS)
                {
                    mark_needs_resync = true;
                }
            }

            if ((err == SSZ_SUCCESS) && (element_count < old_leaf_count))
            {
                err = ssz_merkle_cache_zero_range(cache, element_count, old_leaf_count - element_count);
                if (err != SSZ_SUCCESS)
                {
                    mark_needs_resync = true;
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
                ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, element_count);
                assert(len_err == SSZ_SUCCESS);
                (void)len_err;
                cache->needs_resync = false;
            }
        }
    }

    free(run_tokens);
    if ((err != SSZ_SUCCESS) && mark_needs_resync)
    {
        cache->needs_resync = true;
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

    return needs_resync;
}
