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

    if (alloc_size == 0u)
    {
        alloc_size = 1u;
    }
    if (ssz_internal_add_overflow_size(alloc_size, 32u + sizeof(void *), &total))
    {
        return NULL;
    }

    void *raw = malloc(total);
    if (raw == NULL)
    {
        return NULL;
    }

    {
        uintptr_t aligned = ((uintptr_t)raw + sizeof(void *) + 31u) & ~(uintptr_t)31u; /* NOLINT(misra-c2012-11.6) */
        ((void **)aligned)[-1] = raw; /* NOLINT(misra-c2012-11.6) */
        return (void *)aligned; /* NOLINT(misra-c2012-11.6) */
    }
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
    while (cap < required)
    {
        if (cap > (SIZE_MAX / 2u))
        {
            return SSZ_ERR_OVERFLOW;
        }
        cap <<= 1u;
    }
    *out_capacity = cap;
    return SSZ_SUCCESS;
}

static int ssz_merkle_cache_internal_compare_size_t(const void *a, const void *b)
{
    const size_t va = *(const size_t *)a;
    const size_t vb = *(const size_t *)b;
    if (va < vb)
    {
        return -1;
    }
    if (va > vb)
    {
        return 1;
    }
    return 0;
}

static ssz_error_t ssz_merkle_cache_internal_leaf_words_for_capacity(
    uint64_t leaf_capacity,
    size_t *out_words)
{
    uint64_t words_u64 = 0u;
    if (out_words == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (ssz_internal_add_overflow_u64(leaf_capacity, 63u, &words_u64))
    {
        return SSZ_ERR_OVERFLOW;
    }
    words_u64 /= 64u;
    if (!ssz_internal_u64_to_size(words_u64, out_words))
    {
        return SSZ_ERR_OVERFLOW;
    }
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_node_count(
    uint64_t leaf_capacity,
    size_t *out_node_count)
{
    uint64_t nodes_u64 = 0u;
    if (out_node_count == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (ssz_internal_mul_overflow_u64(leaf_capacity, 2u, &nodes_u64) || (nodes_u64 == 0u))
    {
        return SSZ_ERR_OVERFLOW;
    }
    nodes_u64 -= 1u;
    if (!ssz_internal_u64_to_size(nodes_u64, out_node_count))
    {
        return SSZ_ERR_OVERFLOW;
    }
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_build_zero_hashes(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t out_zero_hashes[64])
{
    if (out_zero_hashes == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    (void)memset(out_zero_hashes[0].bytes, 0, SSZ_BYTES_PER_CHUNK);
    for (size_t depth = 1u; depth < 64u; depth++)
    {
        ssz_error_t err = ssz_hash_2to1(
            hash_fn, &out_zero_hashes[depth - 1u], &out_zero_hashes[depth - 1u], &out_zero_hashes[depth]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_compute_level_offsets(
    uint64_t leaf_capacity,
    uint32_t depth,
    size_t out_offsets[64])
{
    uint64_t width = leaf_capacity;
    size_t running = 0u;

    if (out_offsets == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t level = 0u; level <= depth; level++)
    {
        size_t width_sz = 0u;
        out_offsets[level] = running;
        if (!ssz_internal_u64_to_size(width, &width_sz))
        {
            return SSZ_ERR_OVERFLOW;
        }
        if (ssz_internal_add_overflow_size(running, width_sz, &running))
        {
            return SSZ_ERR_OVERFLOW;
        }
        width >>= 1u;
    }

    return SSZ_SUCCESS;
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

    if ((level_offsets == NULL) || (out_nodes == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_merkle_cache_internal_node_count(leaf_capacity, &node_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    if (ssz_internal_mul_overflow_size(node_count, sizeof(*nodes), &bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }

    nodes = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(bytes);
    if (nodes == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    *out_nodes = nodes;
    (void)depth;
    return SSZ_SUCCESS;
}

static void ssz_merkle_cache_internal_fill_zero_tree(ssz_merkle_cache_t *cache)
{
    uint64_t width = cache->leaf_capacity;
    for (uint32_t level = 0u; level <= cache->depth; level++)
    {
        size_t width_sz = 0u;
        if (!ssz_internal_u64_to_size(width, &width_sz))
        {
            return;
        }

        ssz_chunk_t *level_nodes = cache->nodes + cache->level_offsets[level];
        for (size_t i = 0u; i < width_sz; i++)
        {
            level_nodes[i] = cache->zero_hashes[level];
        }

        width >>= 1u;
    }
}

static ssz_error_t ssz_merkle_cache_internal_dirty_set_init(
    ssz_merkle_cache_dirty_set_t *set,
    size_t word_capacity)
{
    uint64_t *bits = NULL;
    size_t *word_idx = NULL;

    if (set == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    bits = (uint64_t *)calloc(word_capacity, sizeof(*bits));
    if (bits == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    word_idx = (size_t *)malloc(word_capacity * sizeof(*word_idx));
    if (word_idx == NULL)
    {
        free(bits);
        return SSZ_ERR_OVERFLOW;
    }

    set->bits = bits;
    set->word_idx = word_idx;
    set->word_count = 0u;
    set->word_capacity = word_capacity;
    return SSZ_SUCCESS;
}

static void ssz_merkle_cache_internal_dirty_set_free(ssz_merkle_cache_dirty_set_t *set)
{
    if (set == NULL)
    {
        return;
    }

    free(set->bits);
    free(set->word_idx);
    set->bits = NULL;
    set->word_idx = NULL;
    set->word_count = 0u;
    set->word_capacity = 0u;
}

static void ssz_merkle_cache_internal_clear_dirty(
    uint64_t *bits,
    size_t *word_idx,
    size_t *word_count)
{
    if ((bits == NULL) || (word_idx == NULL) || (word_count == NULL))
    {
        return;
    }

    for (size_t i = 0u; i < *word_count; i++)
    {
        bits[word_idx[i]] = 0u;
    }
    *word_count = 0u;
}

static void ssz_merkle_cache_internal_dirty_set_clear(ssz_merkle_cache_dirty_set_t *set)
{
    if (set == NULL)
    {
        return;
    }
    ssz_merkle_cache_internal_clear_dirty(set->bits, set->word_idx, &set->word_count);
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

    if ((bits == NULL) || (word_idx == NULL) || (word_count == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!ssz_internal_u64_to_size(bit_index >> 6u, &word))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (word >= word_capacity)
    {
        return SSZ_ERR_OVERFLOW;
    }

    mask = UINT64_C(1) << (bit_index & 63u);
    prior = bits[word];
    if ((prior & mask) != 0u)
    {
        return SSZ_SUCCESS;
    }

    if (prior == 0u)
    {
        if (*word_count >= word_capacity)
        {
            return SSZ_ERR_OVERFLOW;
        }
        word_idx[*word_count] = word;
        (*word_count)++;
    }

    bits[word] = prior | mask;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_dirty_set_mark(
    ssz_merkle_cache_dirty_set_t *set,
    uint64_t bit_index)
{
    if (set == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_merkle_cache_internal_dirty_mark_bit(
        set->bits, set->word_idx, &set->word_count, set->word_capacity, bit_index);
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

    if ((cache == NULL) || !cache->token_storage_ready || (cache->leaf_token_valid_bits == NULL))
    {
        return false;
    }
    if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        return false;
    }
    if (word >= cache->leaf_token_word_capacity)
    {
        return false;
    }

    mask = UINT64_C(1) << (index & 63u);
    return (cache->leaf_token_valid_bits[word] & mask) != 0u;
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
        return;
    }
    if (!ssz_internal_u64_to_size(index >> 6u, &word))
    {
        return;
    }
    if (word >= cache->leaf_token_word_capacity)
    {
        return;
    }

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

static void ssz_merkle_cache_internal_token_valid_clear_range(
    ssz_merkle_cache_t *cache,
    uint64_t start,
    uint64_t count)
{
    uint64_t end = 0u;
    if ((cache == NULL) || !cache->token_storage_ready)
    {
        return;
    }
    if (count == 0u)
    {
        return;
    }
    if (ssz_internal_add_overflow_u64(start, count, &end))
    {
        return;
    }
    for (uint64_t i = start; i < end; i++)
    {
        ssz_merkle_cache_internal_token_valid_set(cache, i, false);
    }
}

static ssz_error_t ssz_merkle_cache_internal_mark_leaf_dirty(
    ssz_merkle_cache_t *cache,
    uint64_t leaf_index)
{
    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    return ssz_merkle_cache_internal_dirty_mark_bit(cache->leaf_dirty_bits,
                                                    cache->leaf_dirty_word_idx,
                                                    &cache->leaf_dirty_word_count,
                                                    cache->leaf_dirty_word_capacity,
                                                    leaf_index);
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

    if ((cache == NULL) || (leaf == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (leaf_index >= cache->leaf_capacity)
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (!ssz_internal_u64_to_size(leaf_index, &leaf_index_sz))
    {
        return SSZ_ERR_OVERFLOW;
    }

    leaf_slot = cache->nodes + cache->level_offsets[0] + leaf_index_sz;
    changed = (memcmp(leaf_slot->bytes, leaf->bytes, SSZ_BYTES_PER_CHUNK) != 0);
    if (changed)
    {
        *leaf_slot = *leaf;
    }

    if (changed || force_dirty)
    {
        ssz_error_t err = ssz_merkle_cache_internal_mark_leaf_dirty(cache, leaf_index);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_byte_len_to_chunk_count(
    size_t byte_len,
    uint64_t *out_chunk_count)
{
    size_t rounded = 0u;
    if (out_chunk_count == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (byte_len == 0u)
    {
        *out_chunk_count = 0u;
        return SSZ_SUCCESS;
    }
    if (ssz_internal_add_overflow_size(byte_len, SSZ_BYTES_PER_CHUNK - 1u, &rounded))
    {
        return SSZ_ERR_OVERFLOW;
    }
    rounded /= SSZ_BYTES_PER_CHUNK;
    *out_chunk_count = (uint64_t)rounded;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_effective_tree_depth(
    const ssz_merkle_cache_t *cache,
    uint32_t *out_depth)
{
    uint64_t width = 0u;
    uint64_t tree_size = 0u;

    if ((cache == NULL) || (out_depth == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (cache->leaf_limit == SSZ_NO_LIMIT)
    {
        width = cache->leaf_count;
    }
    else
    {
        if (cache->leaf_count > cache->leaf_limit)
        {
            return SSZ_ERR_LIMIT_EXCEEDED;
        }
        width = cache->leaf_limit;
    }

    tree_size = ssz_next_pow_of_two(width);
    if (tree_size == 0u)
    {
        return SSZ_ERR_OVERFLOW;
    }

    *out_depth = ssz_merkle_cache_internal_log2_u64(tree_size);
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_refresh_cached_data_root(ssz_merkle_cache_t *cache)
{
    uint32_t data_depth = 0u;
    ssz_error_t err = ssz_merkle_cache_internal_effective_tree_depth(cache, &data_depth);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    if (data_depth > cache->depth)
    {
        return SSZ_ERR_OVERFLOW;
    }
    cache->cached_data_root = cache->nodes[cache->level_offsets[data_depth]];
    cache->data_root_valid = true;
    cache->final_root_valid = false;
    return SSZ_SUCCESS;
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

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (pair_capacity <= cache->scratch_pair_capacity)
    {
        return SSZ_SUCCESS;
    }

    if (ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_parent_indices), &indices_bytes) ||
        ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_hashes), &hashes_bytes) ||
        ssz_internal_mul_overflow_size(pair_capacity, sizeof(*new_pairs) * 2u, &pairs_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }

    new_pairs = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(pairs_bytes);
    if (new_pairs == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    new_hashes = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(hashes_bytes);
    if (new_hashes == NULL)
    {
        ssz_merkle_cache_internal_free_aligned32(new_pairs);
        return SSZ_ERR_OVERFLOW;
    }

    new_parent_indices = (uint64_t *)malloc(indices_bytes);
    if (new_parent_indices == NULL)
    {
        ssz_merkle_cache_internal_free_aligned32(new_pairs);
        ssz_merkle_cache_internal_free_aligned32(new_hashes);
        return SSZ_ERR_OVERFLOW;
    }

    ssz_merkle_cache_internal_free_aligned32(cache->scratch_pairs);
    ssz_merkle_cache_internal_free_aligned32(cache->scratch_hashes);
    free(cache->scratch_parent_indices);

    cache->scratch_pairs = new_pairs;
    cache->scratch_hashes = new_hashes;
    cache->scratch_parent_indices = new_parent_indices;
    cache->scratch_pair_capacity = pair_capacity;
    return SSZ_SUCCESS;
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

    if ((cache == NULL) || (child_level_nodes == NULL) || (parent_level_nodes == NULL) ||
        (io_gather_count == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (run_len == 0u)
    {
        return SSZ_SUCCESS;
    }

    if (!ssz_internal_u64_to_size(run_start, &run_start_sz) ||
        !ssz_internal_u64_to_size(run_len, &run_len_sz))
    {
        return SSZ_ERR_OVERFLOW;
    }

    if (run_len >= 2u)
    {
        return ssz_hash_2to1_batch(cache->hash_fn,
                                   child_level_nodes + (run_start_sz * 2u),
                                   run_len_sz,
                                   parent_level_nodes + run_start_sz);
    }

    if (*io_gather_count >= cache->scratch_pair_capacity)
    {
        return SSZ_ERR_OVERFLOW;
    }

    cache->scratch_parent_indices[*io_gather_count] = run_start;
    cache->scratch_pairs[*io_gather_count * 2u] = child_level_nodes[run_start_sz * 2u];
    cache->scratch_pairs[((*io_gather_count) * 2u) + 1u] =
        child_level_nodes[(run_start_sz * 2u) + 1u];
    (*io_gather_count)++;
    return SSZ_SUCCESS;
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

    if ((cache == NULL) || (dirty_parents == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (dirty_parents->word_count == 0u)
    {
        return SSZ_SUCCESS;
    }

    qsort(dirty_parents->word_idx,
          dirty_parents->word_count,
          sizeof(dirty_parents->word_idx[0]),
          ssz_merkle_cache_internal_compare_size_t);

    for (size_t wi = 0u; wi < dirty_parents->word_count; wi++)
    {
        size_t word = dirty_parents->word_idx[wi];
        if (word >= dirty_parents->word_capacity)
        {
            return SSZ_ERR_OVERFLOW;
        }
        dirty_count += ssz_merkle_cache_internal_popcount_u64(dirty_parents->bits[word]);
    }

    if (dirty_count == 0u)
    {
        return SSZ_SUCCESS;
    }

    {
        ssz_error_t cap_err = ssz_merkle_cache_internal_ensure_gather_capacity(cache, dirty_count);
        if (cap_err != SSZ_SUCCESS)
        {
            return cap_err;
        }
    }

    child_level_nodes = cache->nodes + cache->level_offsets[level];
    parent_level_nodes = cache->nodes + cache->level_offsets[level + 1u];

    for (size_t wi = 0u; wi < dirty_parents->word_count; wi++)
    {
        size_t word_index = dirty_parents->word_idx[wi];
        uint64_t word_bits = dirty_parents->bits[word_index];
        uint64_t base = 0u;

        if (!ssz_internal_u64_to_size((uint64_t)word_index << 6u, NULL))
        {
            return SSZ_ERR_OVERFLOW;
        }
        base = (uint64_t)word_index << 6u;

        while (word_bits != 0u)
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
                    ssz_error_t flush_err = ssz_merkle_cache_internal_flush_contiguous_run(
                        cache, child_level_nodes, parent_level_nodes, run_start, run_len, &gather_count);
                    if (flush_err != SSZ_SUCCESS)
                    {
                        return flush_err;
                    }
                    run_start = parent_index;
                    run_prev = parent_index;
                    run_len = 1u;
                }
            }
            word_bits &= (word_bits - 1u);
        }
    }

    if (run_active)
    {
        ssz_error_t flush_err = ssz_merkle_cache_internal_flush_contiguous_run(
            cache, child_level_nodes, parent_level_nodes, run_start, run_len, &gather_count);
        if (flush_err != SSZ_SUCCESS)
        {
            return flush_err;
        }
    }

    if (gather_count != 0u)
    {
        ssz_error_t err = ssz_hash_2to1_batch(
            cache->hash_fn, cache->scratch_pairs, gather_count, cache->scratch_hashes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }

        for (size_t i = 0u; i < gather_count; i++)
        {
            size_t parent_index = 0u;
            if (!ssz_internal_u64_to_size(cache->scratch_parent_indices[i], &parent_index))
            {
                return SSZ_ERR_OVERFLOW;
            }
            parent_level_nodes[parent_index] = cache->scratch_hashes[i];
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_build_parent_dirty_set(
    size_t *child_word_idx,
    size_t child_word_count,
    uint64_t *child_bits,
    size_t child_word_capacity,
    uint64_t child_width,
    ssz_merkle_cache_dirty_set_t *out_parent_set)
{
    if ((child_word_idx == NULL) || (child_bits == NULL) || (out_parent_set == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (child_word_count == 0u)
    {
        return SSZ_SUCCESS;
    }

    qsort(child_word_idx,
          child_word_count,
          sizeof(child_word_idx[0]),
          ssz_merkle_cache_internal_compare_size_t);

    for (size_t wi = 0u; wi < child_word_count; wi++)
    {
        size_t word_index = child_word_idx[wi];
        uint64_t word_bits = 0u;
        uint64_t base = 0u;

        if (word_index >= child_word_capacity)
        {
            return SSZ_ERR_OVERFLOW;
        }
        word_bits = child_bits[word_index];
        base = (uint64_t)word_index << 6u;

        while (word_bits != 0u)
        {
            unsigned bit = ssz_merkle_cache_internal_ctz_u64(word_bits);
            uint64_t child_index = base + (uint64_t)bit;
            if (child_index < child_width)
            {
                ssz_error_t mark_err =
                    ssz_merkle_cache_internal_dirty_set_mark(out_parent_set, child_index >> 1u);
                if (mark_err != SSZ_SUCCESS)
                {
                    return mark_err;
                }
            }
            word_bits &= (word_bits - 1u);
        }
    }

    return SSZ_SUCCESS;
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

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (cache->leaf_dirty_word_count == 0u)
    {
        return SSZ_SUCCESS;
    }

    if (cache->depth == 0u)
    {
        ssz_merkle_cache_internal_clear_dirty(
            cache->leaf_dirty_bits, cache->leaf_dirty_word_idx, &cache->leaf_dirty_word_count);
        return SSZ_SUCCESS;
    }

    current_bits = cache->leaf_dirty_bits;
    current_word_idx = cache->leaf_dirty_word_idx;
    current_word_count = cache->leaf_dirty_word_count;
    current_word_capacity = cache->leaf_dirty_word_capacity;
    current_width = cache->leaf_capacity;

    for (uint32_t level = 0u; level < cache->depth; level++)
    {
        uint64_t parent_width = current_width >> 1u;
        ssz_merkle_cache_dirty_set_t *next_set = &cache->scratch_dirty[level & 1u];

        ssz_merkle_cache_internal_dirty_set_clear(next_set);

        if (current_word_count != 0u)
        {
            ssz_error_t build_err = ssz_merkle_cache_internal_build_parent_dirty_set(current_word_idx,
                                                                                      current_word_count,
                                                                                      current_bits,
                                                                                      current_word_capacity,
                                                                                      current_width,
                                                                                      next_set);
            if (build_err != SSZ_SUCCESS)
            {
                return build_err;
            }
        }

        if (next_set->word_count != 0u)
        {
            ssz_error_t hash_err = ssz_merkle_cache_internal_hash_dirty_parents_exact(
                cache, level, next_set, parent_width);
            if (hash_err != SSZ_SUCCESS)
            {
                return hash_err;
            }
        }

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

    if (current_scratch != NULL)
    {
        ssz_merkle_cache_internal_dirty_set_clear(current_scratch);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_token_storage(ssz_merkle_cache_t *cache)
{
    uint64_t *new_tokens = NULL;
    uint64_t *new_valid_bits = NULL;
    size_t requested_token_capacity = 0u;
    size_t requested_word_capacity = 0u;
    size_t copy_tokens = 0u;
    size_t copy_words = 0u;

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!ssz_internal_u64_to_size(cache->leaf_capacity, &requested_token_capacity))
    {
        return SSZ_ERR_OVERFLOW;
    }
    requested_word_capacity = cache->leaf_dirty_word_capacity;
    if (cache->token_storage_ready &&
        (cache->leaf_token_capacity == requested_token_capacity) &&
        (cache->leaf_token_word_capacity == requested_word_capacity))
    {
        return SSZ_SUCCESS;
    }

    new_tokens = (uint64_t *)calloc(requested_token_capacity, sizeof(*new_tokens));
    if (new_tokens == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    new_valid_bits = (uint64_t *)calloc(requested_word_capacity, sizeof(*new_valid_bits));
    if (new_valid_bits == NULL)
    {
        free(new_tokens);
        return SSZ_ERR_OVERFLOW;
    }

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
            (void)memcpy(
                new_valid_bits, cache->leaf_token_valid_bits, copy_words * sizeof(*new_valid_bits));
        }
    }

    free(cache->leaf_tokens);
    free(cache->leaf_token_valid_bits);
    cache->leaf_tokens = new_tokens;
    cache->leaf_token_valid_bits = new_valid_bits;
    cache->leaf_token_capacity = requested_token_capacity;
    cache->leaf_token_word_capacity = requested_word_capacity;
    cache->token_storage_ready = true;
    return SSZ_SUCCESS;
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

    if ((old_cache == NULL) || (out_tokens == NULL) || (out_valid_bits == NULL) ||
        (out_token_capacity == NULL) || (out_token_word_capacity == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!old_cache->token_storage_ready)
    {
        *out_tokens = NULL;
        *out_valid_bits = NULL;
        *out_token_capacity = 0u;
        *out_token_word_capacity = 0u;
        return SSZ_SUCCESS;
    }

    if (!ssz_internal_u64_to_size(new_capacity, &new_token_capacity))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size(new_token_capacity, sizeof(*new_tokens), &token_bytes) ||
        ssz_internal_mul_overflow_size(new_word_capacity, sizeof(*new_valid_bits), &valid_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }

    new_tokens = (uint64_t *)calloc(new_token_capacity, sizeof(*new_tokens));
    if (new_tokens == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

    new_valid_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_valid_bits));
    if (new_valid_bits == NULL)
    {
        free(new_tokens);
        return SSZ_ERR_OVERFLOW;
    }

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
            (void)memcpy(
                new_valid_bits, old_cache->leaf_token_valid_bits, copy_words * sizeof(*new_valid_bits));
        }
    }

    *out_tokens = new_tokens;
    *out_valid_bits = new_valid_bits;
    *out_token_capacity = new_token_capacity;
    *out_token_word_capacity = new_word_capacity;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_grow(ssz_merkle_cache_t *cache, uint64_t new_capacity)
{
    ssz_chunk_t *new_nodes = NULL;
    size_t new_offsets[64] = {0u};
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

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (new_capacity <= cache->leaf_capacity)
    {
        return SSZ_SUCCESS;
    }

    new_depth = ssz_merkle_cache_internal_log2_u64(new_capacity);
    assert(new_depth < 64u);

    {
        ssz_error_t off_err =
            ssz_merkle_cache_internal_compute_level_offsets(new_capacity, new_depth, new_offsets);
        if (off_err != SSZ_SUCCESS)
        {
            return off_err;
        }
    }

    {
        ssz_error_t alloc_err = ssz_merkle_cache_internal_allocate_nodes(
            new_capacity, new_depth, new_offsets, &new_nodes);
        if (alloc_err != SSZ_SUCCESS)
        {
            return alloc_err;
        }
    }

    {
        uint64_t width = new_capacity;
        for (uint32_t level = 0u; level <= new_depth; level++)
        {
            size_t width_sz = 0u;
            if (!ssz_internal_u64_to_size(width, &width_sz))
            {
                ssz_merkle_cache_internal_free_aligned32(new_nodes);
                return SSZ_ERR_OVERFLOW;
            }
            ssz_chunk_t *new_level_nodes = new_nodes + new_offsets[level];
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
                    ssz_merkle_cache_internal_free_aligned32(new_nodes);
                    return SSZ_ERR_OVERFLOW;
                }
                (void)memcpy(new_level_nodes,
                             cache->nodes + cache->level_offsets[level],
                             old_width_sz * sizeof(*new_level_nodes));
            }

            width >>= 1u;
        }
    }

    {
        ssz_error_t words_err =
            ssz_merkle_cache_internal_leaf_words_for_capacity(new_capacity, &new_word_capacity);
        if (words_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_free_aligned32(new_nodes);
            return words_err;
        }
    }

    new_leaf_dirty_bits = (uint64_t *)calloc(new_word_capacity, sizeof(*new_leaf_dirty_bits));
    if (new_leaf_dirty_bits == NULL)
    {
        ssz_merkle_cache_internal_free_aligned32(new_nodes);
        return SSZ_ERR_OVERFLOW;
    }
    new_leaf_dirty_word_idx = (size_t *)malloc(new_word_capacity * sizeof(*new_leaf_dirty_word_idx));
    if (new_leaf_dirty_word_idx == NULL)
    {
        ssz_merkle_cache_internal_free_aligned32(new_nodes);
        free(new_leaf_dirty_bits);
        return SSZ_ERR_OVERFLOW;
    }

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
    if ((new_scratch0_bits == NULL) || (new_scratch0_idx == NULL) || (new_scratch1_bits == NULL) ||
        (new_scratch1_idx == NULL))
    {
        ssz_merkle_cache_internal_free_aligned32(new_nodes);
        free(new_leaf_dirty_bits);
        free(new_leaf_dirty_word_idx);
        free(new_scratch0_bits);
        free(new_scratch0_idx);
        free(new_scratch1_bits);
        free(new_scratch1_idx);
        return SSZ_ERR_OVERFLOW;
    }

    {
        ssz_error_t tok_err = ssz_merkle_cache_internal_resize_token_storage(cache,
                                                                              new_capacity,
                                                                              new_word_capacity,
                                                                              &new_tokens,
                                                                              &new_token_valid_bits,
                                                                              &new_token_capacity,
                                                                              &new_token_word_capacity);
        if (tok_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_internal_free_aligned32(new_nodes);
            free(new_leaf_dirty_bits);
            free(new_leaf_dirty_word_idx);
            free(new_scratch0_bits);
            free(new_scratch0_idx);
            free(new_scratch1_bits);
            free(new_scratch1_idx);
            return tok_err;
        }
    }

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
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_ensure_capacity_for_count(
    ssz_merkle_cache_t *cache,
    uint64_t required_count)
{
    uint64_t target_capacity = 0u;
    uint64_t old_capacity = 0u;

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (required_count <= cache->leaf_capacity)
    {
        return SSZ_SUCCESS;
    }

    if ((cache->leaf_limit != SSZ_NO_LIMIT) && (required_count > cache->leaf_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    old_capacity = cache->leaf_capacity;
    target_capacity = cache->leaf_capacity;
    while (target_capacity < required_count)
    {
        if (target_capacity > (UINT64_MAX / 2u))
        {
            return SSZ_ERR_OVERFLOW;
        }
        target_capacity <<= 1u;
    }

    {
        ssz_error_t grow_err = ssz_merkle_cache_internal_grow(cache, target_capacity);
        if (grow_err != SSZ_SUCCESS)
        {
            return grow_err;
        }
    }

    /* A capacity step introduces a new right side and top chain, even if new
       leaves are still zero. Force one right-side path dirty so the root chain
       is refreshed without rebuilding the preserved left subtree. */
    {
        ssz_error_t mark_err = ssz_merkle_cache_internal_set_leaf(
            cache, old_capacity, &cache->zero_hashes[0], true);
        if (mark_err != SSZ_SUCCESS)
        {
            return mark_err;
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_compute_data_root_if_needed(ssz_merkle_cache_t *cache)
{
    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (cache->data_root_valid)
    {
        return SSZ_SUCCESS;
    }

    {
        ssz_error_t err = ssz_merkle_cache_internal_recompute_data_root(cache);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return ssz_merkle_cache_internal_refresh_cached_data_root(cache);
}

static bool ssz_merkle_cache_internal_limit_matches(
    const ssz_merkle_cache_t *cache,
    uint64_t expected_limit)
{
    if (cache == NULL)
    {
        return false;
    }
    if (cache->leaf_limit == expected_limit)
    {
        return true;
    }
    return false;
}

ssz_error_t ssz_merkle_cache_create(
    const ssz_merkle_cache_config_t *config,
    ssz_merkle_cache_t **out_cache)
{
    ssz_merkle_cache_t *cache = NULL;
    uint64_t initial_capacity = 0u;
    size_t leaf_words = 0u;
    const ssz_hash_fn_t *resolved_hash_fn = NULL;

    if ((config == NULL) || (out_cache == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((config->leaf_limit != SSZ_NO_LIMIT) && (config->initial_leaf_count > config->leaf_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    resolved_hash_fn = ssz_internal_resolve_hash_fn(config->hash_fn);
    if ((resolved_hash_fn == NULL) || (resolved_hash_fn->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

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
        return SSZ_ERR_OVERFLOW;
    }

    cache = (ssz_merkle_cache_t *)calloc(1u, sizeof(*cache));
    if (cache == NULL)
    {
        return SSZ_ERR_OVERFLOW;
    }

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
        ssz_error_t zerr =
            ssz_merkle_cache_internal_build_zero_hashes(resolved_hash_fn, cache->zero_hashes_buf);
        if (zerr != SSZ_SUCCESS)
        {
            ssz_merkle_cache_destroy(cache);
            return zerr;
        }
        cache->zero_hashes = cache->zero_hashes_buf;
    }

    {
        ssz_error_t off_err = ssz_merkle_cache_internal_compute_level_offsets(
            cache->leaf_capacity, cache->depth, cache->level_offsets);
        if (off_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_destroy(cache);
            return off_err;
        }
    }

    {
        ssz_error_t node_err = ssz_merkle_cache_internal_allocate_nodes(
            cache->leaf_capacity, cache->depth, cache->level_offsets, &cache->nodes);
        if (node_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_destroy(cache);
            return node_err;
        }
    }

    {
        ssz_error_t words_err =
            ssz_merkle_cache_internal_leaf_words_for_capacity(cache->leaf_capacity, &leaf_words);
        if (words_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_destroy(cache);
            return words_err;
        }
    }
    cache->leaf_dirty_word_capacity = leaf_words;

    cache->leaf_dirty_bits = (uint64_t *)calloc(leaf_words, sizeof(*cache->leaf_dirty_bits));
    cache->leaf_dirty_word_idx = (size_t *)malloc(leaf_words * sizeof(*cache->leaf_dirty_word_idx));
    if ((cache->leaf_dirty_bits == NULL) || (cache->leaf_dirty_word_idx == NULL))
    {
        ssz_merkle_cache_destroy(cache);
        return SSZ_ERR_OVERFLOW;
    }

    {
        ssz_error_t d0_err =
            ssz_merkle_cache_internal_dirty_set_init(&cache->scratch_dirty[0], leaf_words);
        ssz_error_t d1_err =
            ssz_merkle_cache_internal_dirty_set_init(&cache->scratch_dirty[1], leaf_words);
        if ((d0_err != SSZ_SUCCESS) || (d1_err != SSZ_SUCCESS))
        {
            ssz_merkle_cache_destroy(cache);
            return (d0_err != SSZ_SUCCESS) ? d0_err : d1_err;
        }
    }

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
        ssz_error_t mix_err =
            ssz_mix_in_length_u64(
                &cache->cached_data_root, cache->logical_length, cache->hash_fn, &cache->cached_root);
        if (mix_err != SSZ_SUCCESS)
        {
            ssz_merkle_cache_destroy(cache);
            return mix_err;
        }
        cache->final_root_valid = true;
    }
    else
    {
        cache->cached_root = cache->cached_data_root;
        cache->final_root_valid = true;
    }

    *out_cache = cache;
    return SSZ_SUCCESS;
}

void ssz_merkle_cache_destroy(ssz_merkle_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

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

ssz_error_t ssz_merkle_cache_reset(ssz_merkle_cache_t *cache)
{
    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

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

    return ssz_merkle_cache_internal_refresh_cached_data_root(cache);
}

ssz_error_t ssz_merkle_cache_data_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    if ((cache == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    {
        ssz_error_t err = ssz_merkle_cache_internal_compute_data_root_if_needed(cache);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    *out_root = cache->cached_data_root;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root)
{
    if ((cache == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    {
        ssz_error_t err = ssz_merkle_cache_internal_compute_data_root_if_needed(cache);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    if (!cache->mix_in_length)
    {
        cache->cached_root = cache->cached_data_root;
        cache->final_root_valid = true;
        *out_root = cache->cached_root;
        return SSZ_SUCCESS;
    }

    if (!cache->final_root_valid)
    {
        ssz_error_t err = ssz_mix_in_length_u64(
            &cache->cached_data_root, cache->logical_length, cache->hash_fn, &cache->cached_root);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        cache->final_root_valid = true;
    }

    *out_root = cache->cached_root;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_update_root_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    const ssz_chunk_t *roots,
    uint64_t root_count)
{
    uint64_t end_index = 0u;

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((root_count != 0u) && (roots == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (ssz_internal_add_overflow_u64(start_index, root_count, &end_index))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((cache->leaf_limit != SSZ_NO_LIMIT) && (end_index > cache->leaf_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    {
        ssz_error_t cap_err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, end_index);
        if (cap_err != SSZ_SUCCESS)
        {
            return cap_err;
        }
    }

    for (uint64_t i = 0u; i < root_count; i++)
    {
        uint64_t index = start_index + i;
        ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, index, &roots[i], false);
        if (set_err != SSZ_SUCCESS)
        {
            return set_err;
        }
        ssz_merkle_cache_internal_token_valid_set(cache, index, false);
    }

    if (end_index > cache->leaf_count)
    {
        cache->leaf_count = end_index;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }

    cache->needs_resync = false;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_zero_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    uint64_t zero_count)
{
    uint64_t end_index = 0u;
    uint64_t max_index = 0u;
    const ssz_chunk_t zero_leaf = {
        .bytes = {0u},
    };
    uint64_t old_leaf_count = 0u;

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (zero_count == 0u)
    {
        return SSZ_SUCCESS;
    }
    if (ssz_internal_add_overflow_u64(start_index, zero_count, &end_index))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((cache->leaf_limit != SSZ_NO_LIMIT) && (end_index > cache->leaf_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    old_leaf_count = cache->leaf_count;
    max_index = end_index;
    if (max_index > cache->leaf_capacity)
    {
        max_index = cache->leaf_capacity;
    }

    for (uint64_t i = start_index; i < max_index; i++)
    {
        ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, i, &zero_leaf, false);
        if (set_err != SSZ_SUCCESS)
        {
            return set_err;
        }
    }

    ssz_merkle_cache_internal_token_valid_clear_range(cache, start_index, max_index - start_index);

    if ((start_index < old_leaf_count) && (end_index >= old_leaf_count))
    {
        cache->leaf_count = start_index;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }

    cache->needs_resync = false;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_set_logical_length(
    ssz_merkle_cache_t *cache,
    uint64_t logical_length)
{
    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (cache->logical_length != logical_length)
    {
        cache->logical_length = logical_length;
        ssz_merkle_cache_internal_invalidate_final_root(cache);
    }
    cache->needs_resync = false;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_sync_packed_bytes(
    ssz_merkle_cache_t *cache,
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t logical_length)
{
    uint64_t chunk_count = 0u;
    uint64_t old_leaf_count = 0u;

    if (cache == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((bytes_len != 0u) && (bytes == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    {
        ssz_error_t cc_err =
            ssz_merkle_cache_internal_byte_len_to_chunk_count(bytes_len, &chunk_count);
        if (cc_err != SSZ_SUCCESS)
        {
            return cc_err;
        }
    }

    if ((cache->leaf_limit != SSZ_NO_LIMIT) && (chunk_count > cache->leaf_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    {
        ssz_error_t cap_err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, chunk_count);
        if (cap_err != SSZ_SUCCESS)
        {
            return cap_err;
        }
    }

    old_leaf_count = cache->leaf_count;
    for (uint64_t chunk_index = 0u; chunk_index < chunk_count; chunk_index++)
    {
        ssz_chunk_t leaf;
        size_t chunk_offset = 0u;
        size_t copy_len = SSZ_BYTES_PER_CHUNK;

        (void)memset(leaf.bytes, 0, sizeof(leaf.bytes));
        if (!ssz_internal_u64_to_size(chunk_index, &chunk_offset) ||
            ssz_internal_mul_overflow_size(chunk_offset, SSZ_BYTES_PER_CHUNK, &chunk_offset))
        {
            return SSZ_ERR_OVERFLOW;
        }

        if (chunk_offset < bytes_len)
        {
            size_t remaining = bytes_len - chunk_offset;
            if (remaining < copy_len)
            {
                copy_len = remaining;
            }
            (void)memcpy(leaf.bytes, bytes + chunk_offset, copy_len);
        }

        {
            ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, chunk_index, &leaf, false);
            if (set_err != SSZ_SUCCESS)
            {
                return set_err;
            }
        }
        ssz_merkle_cache_internal_token_valid_set(cache, chunk_index, false);
    }

    if (chunk_count < old_leaf_count)
    {
        ssz_error_t zero_err =
            ssz_merkle_cache_zero_range(cache, chunk_count, old_leaf_count - chunk_count);
        if (zero_err != SSZ_SUCCESS)
        {
            return zero_err;
        }
    }
    else if (chunk_count > old_leaf_count)
    {
        cache->leaf_count = chunk_count;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }
    else
    {
        /* intentionally empty */
    }

    {
        ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, logical_length);
        assert(len_err == SSZ_SUCCESS);
        (void)len_err;
    }

    cache->needs_resync = false;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_merkle_cache_sync_packed_vector_fixed(
    ssz_merkle_cache_t *cache,
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size)
{
    size_t total_bytes = 0u;

    if (cache == NULL)
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
    if (!ssz_merkle_cache_internal_limit_matches(cache, SSZ_NO_LIMIT))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (cache->mix_in_length)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_merkle_cache_sync_packed_bytes(cache, elements, total_bytes, cache->logical_length);
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

    if (cache == NULL)
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

    if (!ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!cache->mix_in_length)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_merkle_cache_sync_packed_bytes(cache, elements, total_bytes, element_count);
}

ssz_error_t ssz_merkle_cache_sync_bitvector(
    ssz_merkle_cache_t *cache,
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count)
{
    size_t bitfield_bytes = 0u;
    uint64_t chunk_limit = 0u;

    if (cache == NULL)
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

    if (!ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (cache->mix_in_length)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_merkle_cache_sync_packed_bytes(cache, bits_le, bitfield_bytes, cache->logical_length);
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

    if (cache == NULL)
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

    if (!ssz_merkle_cache_internal_limit_matches(cache, chunk_limit))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!cache->mix_in_length)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return ssz_merkle_cache_sync_packed_bytes(cache, bits_le, bitfield_bytes, bit_len);
}

static ssz_error_t ssz_merkle_cache_internal_sync_composite_run(
    ssz_merkle_cache_t *cache,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts,
    uint64_t run_start,
    uint64_t run_len,
    const uint64_t *run_tokens)
{
    if ((cache == NULL) || (codec == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (run_len == 0u)
    {
        return SSZ_SUCCESS;
    }

    if ((opts != NULL) && (opts->root_batch != NULL))
    {
        size_t run_len_sz = 0u;
        ssz_chunk_t *tmp_roots = NULL;
        size_t tmp_bytes = 0u;
        if (!ssz_internal_u64_to_size(run_len, &run_len_sz))
        {
            return SSZ_ERR_OVERFLOW;
        }
        if (ssz_internal_mul_overflow_size(run_len_sz, sizeof(*tmp_roots), &tmp_bytes))
        {
            return SSZ_ERR_OVERFLOW;
        }
        tmp_roots = (ssz_chunk_t *)ssz_merkle_cache_internal_alloc_aligned32(tmp_bytes);
        if (tmp_roots == NULL)
        {
            return SSZ_ERR_OVERFLOW;
        }

        {
            ssz_error_t err = opts->root_batch(opts->ctx, run_start, run_len, tmp_roots);
            if (err != SSZ_SUCCESS)
            {
                ssz_merkle_cache_internal_free_aligned32(tmp_roots);
                return err;
            }
        }

        for (uint64_t i = 0u; i < run_len; i++)
        {
            uint64_t index = run_start + i;
            ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, index, &tmp_roots[i], false);
            if (set_err != SSZ_SUCCESS)
            {
                ssz_merkle_cache_internal_free_aligned32(tmp_roots);
                return set_err;
            }
            if (cache->token_storage_ready)
            {
                size_t token_index = 0u;
                if (!ssz_internal_u64_to_size(index, &token_index) ||
                    (token_index >= cache->leaf_token_capacity))
                {
                    ssz_merkle_cache_internal_free_aligned32(tmp_roots);
                    return SSZ_ERR_OVERFLOW;
                }
                if (run_tokens != NULL)
                {
                    cache->leaf_tokens[token_index] = run_tokens[i];
                }
                ssz_merkle_cache_internal_token_valid_set(cache, index, true);
            }
        }

        ssz_merkle_cache_internal_free_aligned32(tmp_roots);
        return SSZ_SUCCESS;
    }

    for (uint64_t i = 0u; i < run_len; i++)
    {
        uint64_t index = run_start + i;
        ssz_chunk_t root;
        ssz_error_t err = codec->root(codec->ctx, index, &root);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        {
            ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, index, &root, false);
            if (set_err != SSZ_SUCCESS)
            {
                return set_err;
            }
        }
        if (cache->token_storage_ready)
        {
            size_t token_index = 0u;
            if (!ssz_internal_u64_to_size(index, &token_index) ||
                (token_index >= cache->leaf_token_capacity))
            {
                return SSZ_ERR_OVERFLOW;
            }
            if (run_tokens != NULL)
            {
                cache->leaf_tokens[token_index] = run_tokens[i];
            }
            ssz_merkle_cache_internal_token_valid_set(cache, index, true);
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_merkle_cache_internal_sync_composite_fallback(
    ssz_merkle_cache_t *cache,
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts)
{
    uint64_t old_leaf_count = cache->leaf_count;

    if ((cache == NULL) || (codec == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((opts != NULL) && (opts->root_batch != NULL) && (element_count != 0u))
    {
        ssz_error_t run_err = ssz_merkle_cache_internal_sync_composite_run(
            cache, codec, opts, 0u, element_count, cache->leaf_tokens);
        if (run_err != SSZ_SUCCESS)
        {
            return run_err;
        }
        ssz_merkle_cache_internal_token_valid_clear_range(cache, 0u, element_count);
    }
    else
    {
        for (uint64_t i = 0u; i < element_count; i++)
        {
            ssz_chunk_t root;
            ssz_error_t err = codec->root(codec->ctx, i, &root);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
            {
                ssz_error_t set_err = ssz_merkle_cache_internal_set_leaf(cache, i, &root, false);
                if (set_err != SSZ_SUCCESS)
                {
                    return set_err;
                }
            }
            ssz_merkle_cache_internal_token_valid_set(cache, i, false);
        }
    }

    if (element_count < old_leaf_count)
    {
        ssz_error_t zero_err =
            ssz_merkle_cache_zero_range(cache, element_count, old_leaf_count - element_count);
        if (zero_err != SSZ_SUCCESS)
        {
            return zero_err;
        }
    }
    else if (element_count > old_leaf_count)
    {
        cache->leaf_count = element_count;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }
    else
    {
        /* intentionally empty */
    }

    return SSZ_SUCCESS;
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

    if ((cache == NULL) || (codec == NULL) || (codec->root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (!ssz_merkle_cache_internal_limit_matches(cache, element_limit))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    {
        ssz_error_t cap_err = ssz_merkle_cache_internal_ensure_capacity_for_count(cache, element_count);
        if (cap_err != SSZ_SUCCESS)
        {
            return cap_err;
        }
    }

    old_leaf_count = cache->leaf_count;

    if ((opts == NULL) || (opts->token == NULL))
    {
        ssz_error_t fallback_err =
            ssz_merkle_cache_internal_sync_composite_fallback(cache, element_count, codec, opts);
        if (fallback_err != SSZ_SUCCESS)
        {
            cache->needs_resync = true;
            return fallback_err;
        }

        {
            ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, element_count);
            assert(len_err == SSZ_SUCCESS);
            (void)len_err;
        }

        cache->needs_resync = false;
        return SSZ_SUCCESS;
    }

    {
        ssz_error_t tok_store_err = ssz_merkle_cache_internal_ensure_token_storage(cache);
        if (tok_store_err != SSZ_SUCCESS)
        {
            return tok_store_err;
        }
    }

    for (uint64_t i = 0u; i < element_count; i++)
    {
        uint64_t token = 0u;
        bool unchanged = false;
        ssz_error_t token_err = opts->token(opts->ctx, i, &token);
        if (token_err != SSZ_SUCCESS)
        {
            cache->needs_resync = true;
            free(run_tokens);
            return token_err;
        }

        if (i < old_leaf_count)
        {
            size_t i_sz = 0u;
            if (!ssz_internal_u64_to_size(i, &i_sz))
            {
                cache->needs_resync = true;
                free(run_tokens);
                return SSZ_ERR_OVERFLOW;
            }
            unchanged = ssz_merkle_cache_internal_token_valid_get(cache, i) &&
                        (cache->leaf_tokens[i_sz] == token);
        }

        if (unchanged)
        {
            if (run_len != 0u)
            {
                ssz_error_t run_err = ssz_merkle_cache_internal_sync_composite_run(
                    cache, codec, opts, run_start, run_len, run_tokens);
                if (run_err != SSZ_SUCCESS)
                {
                    cache->needs_resync = true;
                    free(run_tokens);
                    return run_err;
                }
                run_len = 0u;
            }
            continue;
        }

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
            {
                ssz_error_t cap_err =
                    ssz_merkle_cache_internal_grow_capacity(run_tokens_cap, (size_t)run_len, &new_cap);
                assert(cap_err == SSZ_SUCCESS); /* LCOV_EXCL_LINE */
                (void)cap_err;
            }
            {
                uint64_t *new_tokens = (uint64_t *)realloc(run_tokens, new_cap * sizeof(*new_tokens));
                if (new_tokens == NULL)
                {
                    cache->needs_resync = true;
                    free(run_tokens);
                    return SSZ_ERR_OVERFLOW;
                }
                run_tokens = new_tokens;
                run_tokens_cap = new_cap;
            }
        }

        run_tokens[run_len - 1u] = token;
    }

    if (run_len != 0u)
    {
        ssz_error_t run_err =
            ssz_merkle_cache_internal_sync_composite_run(cache, codec, opts, run_start, run_len, run_tokens);
        if (run_err != SSZ_SUCCESS)
        {
            cache->needs_resync = true;
            free(run_tokens);
            return run_err;
        }
    }

    free(run_tokens);

    if (element_count < old_leaf_count)
    {
        ssz_error_t zero_err =
            ssz_merkle_cache_zero_range(cache, element_count, old_leaf_count - element_count);
        if (zero_err != SSZ_SUCCESS)
        {
            cache->needs_resync = true;
            return zero_err;
        }
    }
    else if (element_count > old_leaf_count)
    {
        cache->leaf_count = element_count;
        ssz_merkle_cache_internal_invalidate_data_root(cache);
    }
    else
    {
        /* intentionally empty */
    }

    {
        ssz_error_t len_err = ssz_merkle_cache_set_logical_length(cache, element_count);
        assert(len_err == SSZ_SUCCESS);
        (void)len_err;
    }

    cache->needs_resync = false;
    return SSZ_SUCCESS;
}

bool ssz_merkle_cache_needs_resync(const ssz_merkle_cache_t *cache)
{
    if (cache == NULL)
    {
        return false;
    }
    return cache->needs_resync;
}
