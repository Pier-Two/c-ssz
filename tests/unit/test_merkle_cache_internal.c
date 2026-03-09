#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssz.h"
#include "ssz_internal.h"

typedef bool (*test_fn_t)(void);

typedef struct
{
    const char *name;
    test_fn_t fn;
} test_case_t;

#define ASSERT_TRUE(cond)                                                                            \
    do                                                                                               \
    {                                                                                                \
        if (!(cond))                                                                                 \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_ERR(expr, expected)                                                                   \
    do                                                                                               \
    {                                                                                                \
        ssz_error_t _actual = (expr);                                                                \
        if (_actual != (expected))                                                                   \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: %s returned %s (%d), expected %s (%d)\n",         \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    #expr,                                                                            \
                    ssz_error_string(_actual),                                                        \
                    (int)_actual,                                                                     \
                    ssz_error_string((expected)),                                                     \
                    (int)(expected));                                                                 \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_U64_EQ(actual, expected)                                                              \
    do                                                                                               \
    {                                                                                                \
        uint64_t _actual = (actual);                                                                 \
        uint64_t _expected = (expected);                                                             \
        if (_actual != _expected)                                                                    \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: %" PRIu64 " != %" PRIu64 "\n",                    \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    _actual,                                                                          \
                    _expected);                                                                       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_SIZE_EQ(actual, expected)                                                             \
    do                                                                                               \
    {                                                                                                \
        size_t _actual = (actual);                                                                   \
        size_t _expected = (expected);                                                               \
        if (_actual != _expected)                                                                    \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: %zu != %zu\n",                                      \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    _actual,                                                                          \
                    _expected);                                                                       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

typedef struct
{
    size_t malloc_calls;
    size_t malloc_fail_at;
    size_t calloc_calls;
    size_t calloc_fail_at;
    size_t realloc_calls;
    size_t realloc_fail_at;
    size_t u64_to_size_calls;
    size_t u64_to_size_fail_at;
    size_t add_overflow_size_calls;
    size_t add_overflow_size_fail_at;
    size_t mul_overflow_size_calls;
    size_t mul_overflow_size_fail_at;
    size_t add_overflow_u64_calls;
    size_t add_overflow_u64_fail_at;
    size_t mul_overflow_u64_calls;
    size_t mul_overflow_u64_fail_at;
    size_t bits_to_bytes_calls;
    size_t bits_to_bytes_fail_at;
    size_t hash_2to1_calls;
    size_t hash_2to1_fail_at;
    ssz_error_t hash_2to1_fail_err;
    size_t hash_2to1_batch_calls;
    size_t hash_2to1_batch_fail_at;
    ssz_error_t hash_2to1_batch_fail_err;
    size_t mix_in_length_calls;
    size_t mix_in_length_fail_at;
    ssz_error_t mix_in_length_fail_err;
} hook_state_t;

static hook_state_t g_hooks;

static void reset_hooks(void)
{
    memset(&g_hooks, 0, sizeof(g_hooks));
    g_hooks.hash_2to1_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.mix_in_length_fail_err = SSZ_ERR_HASH_FAILURE;
}

static bool hook_should_fail(size_t *counter, size_t fail_at)
{
    (*counter)++;
    return (fail_at != 0u) && (*counter == fail_at);
}

static void *hook_malloc(size_t size)
{
    if (hook_should_fail(&g_hooks.malloc_calls, g_hooks.malloc_fail_at))
    {
        return NULL;
    }
    return malloc(size);
}

static void *hook_calloc(size_t count, size_t size)
{
    if (hook_should_fail(&g_hooks.calloc_calls, g_hooks.calloc_fail_at))
    {
        return NULL;
    }
    return calloc(count, size);
}

static void *hook_realloc(void *ptr, size_t size)
{
    if (hook_should_fail(&g_hooks.realloc_calls, g_hooks.realloc_fail_at))
    {
        return NULL;
    }
    return realloc(ptr, size);
}

static void hook_free(void *ptr)
{
    free(ptr);
}

static bool hook_u64_to_size(uint64_t value, size_t *out)
{
    if (hook_should_fail(&g_hooks.u64_to_size_calls, g_hooks.u64_to_size_fail_at))
    {
        return false;
    }
    return ssz_internal_u64_to_size(value, out);
}

static bool hook_add_overflow_size(size_t a, size_t b, size_t *out)
{
    if (hook_should_fail(&g_hooks.add_overflow_size_calls, g_hooks.add_overflow_size_fail_at))
    {
        return true;
    }
    return ssz_internal_add_overflow_size(a, b, out);
}

static bool hook_mul_overflow_size(size_t a, size_t b, size_t *out)
{
    if (hook_should_fail(&g_hooks.mul_overflow_size_calls, g_hooks.mul_overflow_size_fail_at))
    {
        return true;
    }
    return ssz_internal_mul_overflow_size(a, b, out);
}

static bool hook_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (hook_should_fail(&g_hooks.add_overflow_u64_calls, g_hooks.add_overflow_u64_fail_at))
    {
        return true;
    }
    return ssz_internal_add_overflow_u64(a, b, out);
}

static bool hook_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (hook_should_fail(&g_hooks.mul_overflow_u64_calls, g_hooks.mul_overflow_u64_fail_at))
    {
        return true;
    }
    return ssz_internal_mul_overflow_u64(a, b, out);
}

static bool hook_bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    if (hook_should_fail(&g_hooks.bits_to_bytes_calls, g_hooks.bits_to_bytes_fail_at))
    {
        return false;
    }
    return ssz_internal_bits_to_bytes(bit_count, out_bytes);
}

static ssz_error_t hook_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    if (hook_should_fail(&g_hooks.hash_2to1_calls, g_hooks.hash_2to1_fail_at))
    {
        return g_hooks.hash_2to1_fail_err;
    }
    return ssz_hash_2to1(hash_fn, left, right, out);
}

static ssz_error_t hook_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    if (hook_should_fail(&g_hooks.hash_2to1_batch_calls, g_hooks.hash_2to1_batch_fail_at))
    {
        return g_hooks.hash_2to1_batch_fail_err;
    }
    return ssz_hash_2to1_batch(hash_fn, pairs, pair_count, out);
}

static ssz_error_t hook_mix_in_length_u64(
    const ssz_chunk_t *root,
    uint64_t length,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    if (hook_should_fail(&g_hooks.mix_in_length_calls, g_hooks.mix_in_length_fail_at))
    {
        return g_hooks.mix_in_length_fail_err;
    }
    return ssz_mix_in_length_u64(root, length, hash_fn, out_root);
}

#define malloc hook_malloc
#define calloc hook_calloc
#define realloc hook_realloc
#define free hook_free
#define ssz_internal_u64_to_size hook_u64_to_size
#define ssz_internal_add_overflow_size hook_add_overflow_size
#define ssz_internal_mul_overflow_size hook_mul_overflow_size
#define ssz_internal_add_overflow_u64 hook_add_overflow_u64
#define ssz_internal_mul_overflow_u64 hook_mul_overflow_u64
#define ssz_internal_bits_to_bytes hook_bits_to_bytes
#define ssz_hash_2to1 hook_hash_2to1
#define ssz_hash_2to1_batch hook_hash_2to1_batch
#define ssz_mix_in_length_u64 hook_mix_in_length_u64
#include "ssz_merkle_cache.c"
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef ssz_internal_u64_to_size
#undef ssz_internal_add_overflow_size
#undef ssz_internal_mul_overflow_size
#undef ssz_internal_add_overflow_u64
#undef ssz_internal_mul_overflow_u64
#undef ssz_internal_bits_to_bytes
#undef ssz_hash_2to1
#undef ssz_hash_2to1_batch
#undef ssz_mix_in_length_u64

static ssz_chunk_t make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_error_t passthrough_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    return ssz_hash_sha256(data, data_len, out);
}

static void init_manual_cache(
    ssz_merkle_cache_t *cache,
    ssz_chunk_t *nodes,
    uint64_t leaf_capacity,
    uint32_t depth,
    uint64_t *leaf_dirty_bits,
    size_t *leaf_dirty_word_idx,
    size_t leaf_dirty_word_capacity)
{
    memset(cache, 0, sizeof(*cache));
    cache->hash_fn = ssz_hash_default();
    cache->zero_hashes = ssz_hash_default_zero_hashes();
    cache->nodes = nodes;
    cache->leaf_capacity = leaf_capacity;
    cache->depth = depth;
    cache->leaf_limit = SSZ_NO_LIMIT;
    cache->leaf_dirty_bits = leaf_dirty_bits;
    cache->leaf_dirty_word_idx = leaf_dirty_word_idx;
    cache->leaf_dirty_word_capacity = leaf_dirty_word_capacity;
    cache->level_offsets[0] = 0u;
    for (uint32_t i = 1u; i < 64u; i++)
    {
        cache->level_offsets[i] = cache->level_offsets[i - 1u] + (size_t)(leaf_capacity >> (i - 1u));
    }
}

typedef struct
{
    const ssz_chunk_t *roots;
    const uint64_t *tokens;
    uint64_t count;
    uint64_t fail_index;
    ssz_error_t fail_err;
} composite_fixture_t;

static ssz_error_t composite_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const composite_fixture_t *fixture = (const composite_fixture_t *)ctx;
    if ((fixture == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == fixture->fail_index)
    {
        return fixture->fail_err;
    }
    if (member_id >= fixture->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_root = fixture->roots[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t composite_root_batch(
    const void *ctx,
    uint64_t start_index,
    uint64_t count,
    ssz_chunk_t *out_roots)
{
    const composite_fixture_t *fixture = (const composite_fixture_t *)ctx;
    if ((fixture == NULL) || (out_roots == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((start_index + count) > fixture->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((fixture->fail_index >= start_index) && (fixture->fail_index < (start_index + count)))
    {
        return fixture->fail_err;
    }
    for (uint64_t i = 0u; i < count; i++)
    {
        out_roots[i] = fixture->roots[start_index + i];
    }
    return SSZ_SUCCESS;
}

static ssz_error_t composite_token(const void *ctx, uint64_t member_id, uint64_t *out_token)
{
    const composite_fixture_t *fixture = (const composite_fixture_t *)ctx;
    if ((fixture == NULL) || (out_token == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == fixture->fail_index)
    {
        return fixture->fail_err;
    }
    if (member_id >= fixture->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_token = fixture->tokens[member_id];
    return SSZ_SUCCESS;
}

static bool test_basic_internal_helpers(void)
{
    size_t out_size = 0u;
    size_t offsets[64] = {0u};
    ssz_chunk_t zero_hashes[64];
    ssz_chunk_t *nodes = NULL;
    void *ptr = NULL;
    ssz_merkle_cache_t cache;
    ssz_chunk_t node_store[1];

    reset_hooks();
    ptr = ssz_merkle_cache_internal_alloc_aligned32(0u);
    ASSERT_TRUE(ptr != NULL);
    ASSERT_TRUE((((uintptr_t)ptr) & (uintptr_t)31u) == 0u);
    ssz_merkle_cache_internal_free_aligned32(ptr);
    ssz_merkle_cache_internal_free_aligned32(NULL);

    reset_hooks();
    g_hooks.add_overflow_size_fail_at = 1u;
    ASSERT_TRUE(ssz_merkle_cache_internal_alloc_aligned32(16u) == NULL);

    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_TRUE(ssz_merkle_cache_internal_alloc_aligned32(16u) == NULL);

    {
        size_t a = 1u;
        size_t b = 2u;
        ASSERT_TRUE(ssz_merkle_cache_internal_compare_size_t(&a, &b) < 0);
        ASSERT_TRUE(ssz_merkle_cache_internal_compare_size_t(&b, &a) > 0);
        ASSERT_TRUE(ssz_merkle_cache_internal_compare_size_t(&a, &a) == 0);
        ASSERT_U64_EQ(ssz_merkle_cache_internal_log2_u64(8u), 3u);
        ASSERT_SIZE_EQ(ssz_merkle_cache_internal_popcount_u64(UINT64_C(0x15)), 3u);
        ASSERT_U64_EQ(ssz_merkle_cache_internal_ctz_u64(8u), 3u);
    }

    ASSERT_ERR(ssz_merkle_cache_internal_leaf_words_for_capacity(8u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_leaf_words_for_capacity(8u, &out_size), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_leaf_words_for_capacity(8u, &out_size), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_internal_leaf_words_for_capacity(65u, &out_size), SSZ_SUCCESS);
    ASSERT_SIZE_EQ(out_size, 2u);

    ASSERT_ERR(ssz_merkle_cache_internal_node_count(8u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_node_count(0u, &out_size), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_node_count(8u, &out_size), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_build_zero_hashes(ssz_hash_default(), NULL), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.hash_2to1_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_build_zero_hashes(ssz_hash_default(), zero_hashes),
               SSZ_ERR_HASH_FAILURE);
    ASSERT_ERR(ssz_merkle_cache_internal_build_zero_hashes(ssz_hash_default(), zero_hashes), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_internal_compute_level_offsets(8u, 3u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_compute_level_offsets(8u, 3u, offsets), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.add_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_compute_level_offsets(8u, 3u, offsets), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_internal_compute_level_offsets(8u, 3u, offsets), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(8u, 3u, NULL, &nodes), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(8u, 3u, offsets, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(0u, 0u, offsets, &nodes), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(8u, 3u, offsets, &nodes), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(8u, 3u, offsets, &nodes), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_internal_allocate_nodes(8u, 3u, offsets, &nodes), SSZ_SUCCESS);
    ASSERT_TRUE(nodes != NULL);
    ssz_merkle_cache_internal_free_aligned32(nodes);

    init_manual_cache(&cache, node_store, 1u, 0u, NULL, NULL, 0u);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ssz_merkle_cache_internal_fill_zero_tree(&cache);

    ASSERT_ERR(ssz_merkle_cache_internal_byte_len_to_chunk_count(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    {
        uint64_t chunk_count = 99u;
        ASSERT_ERR(ssz_merkle_cache_internal_byte_len_to_chunk_count(0u, &chunk_count), SSZ_SUCCESS);
        ASSERT_U64_EQ(chunk_count, 0u);
    }
    reset_hooks();
    g_hooks.add_overflow_size_fail_at = 1u;
    {
        uint64_t chunk_count = 0u;
        ASSERT_ERR(ssz_merkle_cache_internal_byte_len_to_chunk_count(8u, &chunk_count), SSZ_ERR_OVERFLOW);
    }

    return true;
}

static bool test_dirty_and_token_helpers(void)
{
    ssz_merkle_cache_dirty_set_t set = {0};
    uint64_t bits[1] = {0u};
    size_t word_idx[1] = {0u};
    ssz_chunk_t nodes[1];
    ssz_merkle_cache_t cache;
    ssz_chunk_t leaf = make_chunk(0x22u);

    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_init(NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.calloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_init(&set, 1u), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_init(&set, 1u), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_init(&set, 1u), SSZ_SUCCESS);
    ssz_merkle_cache_internal_dirty_set_free(NULL);
    ssz_merkle_cache_internal_dirty_set_free(&set);

    ssz_merkle_cache_internal_clear_dirty(NULL, word_idx, &set.word_count);
    ssz_merkle_cache_internal_clear_dirty(bits, NULL, &set.word_count);
    ssz_merkle_cache_internal_clear_dirty(bits, word_idx, NULL);
    ssz_merkle_cache_internal_dirty_set_clear(NULL);

    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(NULL, word_idx, &set.word_count, 1u, 0u),
               SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(bits, word_idx, &set.word_count, 1u, 0u),
               SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(bits, word_idx, &set.word_count, 1u, 64u),
               SSZ_ERR_OVERFLOW);

    bits[0] = 0u;
    set.word_count = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(bits, word_idx, &set.word_count, 1u, 0u),
               SSZ_ERR_OVERFLOW);

    bits[0] = 0u;
    set.word_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(bits, word_idx, &set.word_count, 1u, 0u),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_mark_bit(bits, word_idx, &set.word_count, 1u, 0u),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_mark(NULL, 0u), SSZ_ERR_INVALID_ARGUMENT);

    memset(&cache, 0, sizeof(cache));
    ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));
    cache.token_storage_ready = true;
    ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));

    {
        uint64_t valid_bits[1] = {0u};
        cache.leaf_token_valid_bits = valid_bits;
        cache.leaf_token_word_capacity = 1u;
        ssz_merkle_cache_internal_token_valid_set(&cache, 0u, true);
        ASSERT_TRUE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));
        ssz_merkle_cache_internal_token_valid_set(&cache, 0u, false);
        ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));

        reset_hooks();
        g_hooks.u64_to_size_fail_at = 1u;
        ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));

        reset_hooks();
        g_hooks.u64_to_size_fail_at = 1u;
        ssz_merkle_cache_internal_token_valid_set(&cache, 0u, true);

        cache.leaf_token_word_capacity = 0u;
        ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));
        ssz_merkle_cache_internal_token_valid_set(&cache, 0u, true);
    }

    ssz_merkle_cache_internal_token_valid_clear_range(NULL, 0u, 1u);
    memset(&cache, 0, sizeof(cache));
    ssz_merkle_cache_internal_token_valid_clear_range(&cache, 0u, 1u);
    cache.token_storage_ready = true;
    ssz_merkle_cache_internal_token_valid_clear_range(&cache, 0u, 0u);
    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ssz_merkle_cache_internal_token_valid_clear_range(&cache, UINT64_MAX, 1u);

    ASSERT_ERR(ssz_merkle_cache_internal_mark_leaf_dirty(NULL, 0u), SSZ_ERR_INVALID_ARGUMENT);

    init_manual_cache(&cache, nodes, 1u, 0u, bits, word_idx, 1u);
    nodes[0] = leaf;
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(NULL, 0u, &leaf, false), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, NULL, false), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 1u, &leaf, false), SSZ_ERR_LIMIT_EXCEEDED);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, false), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 1u, 0u, bits, word_idx, 0u);
    nodes[0] = make_chunk(0x01u);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, false), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 1u, 0u, bits, word_idx, 1u);
    nodes[0] = leaf;
    cache.data_root_valid = true;
    cache.final_root_valid = true;
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, false), SSZ_SUCCESS);
    ASSERT_TRUE(cache.data_root_valid);
    ASSERT_TRUE(cache.final_root_valid);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, true), SSZ_SUCCESS);
    ASSERT_FALSE(cache.data_root_valid);
    ASSERT_FALSE(cache.final_root_valid);

    return true;
}

static bool test_depth_gather_and_recompute_paths(void)
{
    uint32_t out_depth = 0u;
    ssz_merkle_cache_t cache;
    ssz_chunk_t nodes[7];
    uint64_t dirty_bits[1] = {0u};
    size_t dirty_idx[1] = {0u};
    uint64_t scratch_bits[1] = {0u};
    size_t scratch_idx[1] = {0u};
    size_t gather_count = 0u;
    ssz_chunk_t child_nodes[4];
    ssz_chunk_t parent_nodes[2];
    ssz_merkle_cache_dirty_set_t dirty_set = {0};
    ssz_merkle_cache_dirty_set_t out_parent = {0};

    ASSERT_ERR(ssz_merkle_cache_internal_effective_tree_depth(NULL, &out_depth), SSZ_ERR_INVALID_ARGUMENT);
    memset(&cache, 0, sizeof(cache));
    ASSERT_ERR(ssz_merkle_cache_internal_effective_tree_depth(&cache, NULL), SSZ_ERR_INVALID_ARGUMENT);

    cache.leaf_limit = 4u;
    cache.leaf_count = 5u;
    ASSERT_ERR(ssz_merkle_cache_internal_effective_tree_depth(&cache, &out_depth), SSZ_ERR_LIMIT_EXCEEDED);

    cache.leaf_limit = SSZ_NO_LIMIT;
    cache.leaf_count = (UINT64_C(1) << 63u) + 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_effective_tree_depth(&cache, &out_depth), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 2u, 0u, dirty_bits, dirty_idx, 1u);
    cache.leaf_limit = 1u;
    cache.leaf_count = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_refresh_cached_data_root(&cache), SSZ_ERR_LIMIT_EXCEEDED);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.leaf_limit = SSZ_NO_LIMIT;
    cache.leaf_count = 8u;
    ASSERT_ERR(ssz_merkle_cache_internal_refresh_cached_data_root(&cache), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    memset(&cache, 0, sizeof(cache));
    cache.scratch_pair_capacity = 4u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 0u), SSZ_SUCCESS);

    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 8u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 8u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.malloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 8u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.malloc_fail_at = 3u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 8u), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 2u), SSZ_SUCCESS);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_pairs);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_hashes);
    hook_free(cache.scratch_parent_indices);

    init_manual_cache(&cache, nodes, 4u, 2u, dirty_bits, dirty_idx, 1u);
    cache.scratch_pair_capacity = 1u;
    cache.scratch_pairs =
        ssz_merkle_cache_internal_alloc_aligned32(2u * sizeof(*cache.scratch_pairs));
    cache.scratch_hashes =
        ssz_merkle_cache_internal_alloc_aligned32(1u * sizeof(*cache.scratch_hashes));
    cache.scratch_parent_indices = hook_calloc(1u, sizeof(*cache.scratch_parent_indices));
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(NULL, child_nodes, parent_nodes, 0u, 1u, &gather_count),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, NULL, parent_nodes, 0u, 1u, &gather_count),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, child_nodes, parent_nodes, 0u, 0u, &gather_count),
               SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, child_nodes, parent_nodes, 0u, 1u, &gather_count),
               SSZ_ERR_OVERFLOW);

    cache.scratch_pair_capacity = 0u;
    gather_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, child_nodes, parent_nodes, 0u, 1u, &gather_count),
               SSZ_ERR_OVERFLOW);

    cache.scratch_pair_capacity = 1u;
    gather_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, child_nodes, parent_nodes, 0u, 1u, &gather_count),
               SSZ_SUCCESS);
    gather_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_flush_contiguous_run(&cache, child_nodes, parent_nodes, 0u, 2u, &gather_count),
               SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(NULL, 0u, &dirty_set, 1u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, NULL, 1u),
               SSZ_ERR_INVALID_ARGUMENT);

    dirty_set.word_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u), SSZ_SUCCESS);

    dirty_set.bits = dirty_bits;
    dirty_set.word_idx = dirty_idx;
    dirty_set.word_capacity = 1u;
    dirty_set.word_count = 1u;
    dirty_idx[0] = 3u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u), SSZ_ERR_OVERFLOW);

    dirty_idx[0] = 0u;
    dirty_bits[0] = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u), SSZ_SUCCESS);

    dirty_bits[0] = UINT64_C(0xB);
    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
               SSZ_ERR_HASH_FAILURE);

    dirty_bits[0] = UINT64_C(0x3);
    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
               SSZ_ERR_HASH_FAILURE);

    dirty_bits[0] = UINT64_C(0x5);
    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
               SSZ_ERR_HASH_FAILURE);

    dirty_bits[0] = UINT64_C(0x5);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
               SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(NULL, 1u, dirty_bits, 1u, 4u, &out_parent),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(dirty_idx, 1u, NULL, 1u, 4u, &out_parent),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(dirty_idx, 1u, dirty_bits, 1u, 4u, NULL),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(dirty_idx, 0u, dirty_bits, 1u, 4u, &out_parent),
               SSZ_SUCCESS);

    out_parent.bits = dirty_bits;
    out_parent.word_idx = dirty_idx;
    out_parent.word_capacity = 0u;
    out_parent.word_count = 0u;
    dirty_idx[0] = 1u;
    dirty_bits[0] = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(dirty_idx, 1u, dirty_bits, 1u, 4u, &out_parent),
               SSZ_ERR_OVERFLOW);

    dirty_idx[0] = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_build_parent_dirty_set(dirty_idx, 1u, dirty_bits, 1u, 4u, &out_parent),
               SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(NULL), SSZ_ERR_INVALID_ARGUMENT);
    init_manual_cache(&cache, nodes, 1u, 0u, dirty_bits, dirty_idx, 1u);
    cache.leaf_dirty_word_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_SUCCESS);

    cache.leaf_dirty_word_count = 1u;
    dirty_bits[0] = 1u;
    dirty_idx[0] = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_SUCCESS);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.scratch_dirty[0].bits = scratch_bits;
    cache.scratch_dirty[0].word_idx = scratch_idx;
    cache.scratch_dirty[0].word_capacity = 0u;
    cache.leaf_dirty_word_count = 1u;
    dirty_bits[0] = 1u;
    dirty_idx[0] = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.scratch_dirty[0].bits = scratch_bits;
    cache.scratch_dirty[0].word_idx = scratch_idx;
    cache.scratch_dirty[0].word_capacity = 1u;
    cache.scratch_dirty[0].word_count = 0u;
    cache.leaf_dirty_word_count = 1u;
    dirty_bits[0] = 1u;
    dirty_idx[0] = 0u;
    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_ERR_HASH_FAILURE);

    ssz_merkle_cache_internal_free_aligned32(cache.scratch_pairs);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_hashes);
    hook_free(cache.scratch_parent_indices);

    return true;
}

static bool test_storage_and_growth_paths(void)
{
    ssz_merkle_cache_t cache;
    ssz_chunk_t nodes[3];
    uint64_t dirty_bits[1] = {0u};
    size_t dirty_idx[1] = {0u};
    uint64_t tokens[2] = {11u, 22u};
    uint64_t valid_bits[1] = {UINT64_C(0x3)};
    uint64_t *out_tokens = NULL;
    uint64_t *out_valid_bits = NULL;
    size_t out_token_capacity = 0u;
    size_t out_token_word_capacity = 0u;

    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(NULL), SSZ_ERR_INVALID_ARGUMENT);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.calloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.calloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_SUCCESS);
    hook_free(cache.leaf_tokens);
    hook_free(cache.leaf_token_valid_bits);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.token_storage_ready = true;
    cache.leaf_tokens = hook_calloc(2u, sizeof(*cache.leaf_tokens));
    cache.leaf_token_valid_bits = hook_calloc(1u, sizeof(*cache.leaf_token_valid_bits));
    cache.leaf_tokens[0] = tokens[0];
    cache.leaf_tokens[1] = tokens[1];
    cache.leaf_token_valid_bits[0] = valid_bits[0];
    cache.leaf_token_capacity = 2u;
    cache.leaf_token_word_capacity = 1u;
    cache.leaf_capacity = 4u;
    cache.leaf_dirty_word_capacity = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_SUCCESS);
    ASSERT_U64_EQ(cache.leaf_tokens[0], 11u);
    ASSERT_TRUE((cache.leaf_token_valid_bits[0] & 1u) != 0u);
    hook_free(cache.leaf_tokens);
    hook_free(cache.leaf_token_valid_bits);

    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(NULL, 4u, 1u, &out_tokens, &out_valid_bits,
                                                              &out_token_capacity, &out_token_word_capacity),
               SSZ_ERR_INVALID_ARGUMENT);

    memset(&cache, 0, sizeof(cache));
    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_SUCCESS);
    ASSERT_TRUE(out_tokens == NULL);

    cache.token_storage_ready = true;
    cache.leaf_tokens = tokens;
    cache.leaf_token_valid_bits = valid_bits;
    cache.leaf_token_capacity = 2u;
    cache.leaf_token_word_capacity = 1u;

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.calloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.calloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                              4u,
                                                              1u,
                                                              &out_tokens,
                                                              &out_valid_bits,
                                                              &out_token_capacity,
                                                              &out_token_word_capacity),
               SSZ_SUCCESS);
    ASSERT_U64_EQ(out_tokens[0], 11u);
    hook_free(out_tokens);
    hook_free(out_valid_bits);

    ASSERT_ERR(ssz_merkle_cache_internal_grow(NULL, 4u), SSZ_ERR_INVALID_ARGUMENT);
    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 2u), SSZ_SUCCESS);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    reset_hooks();
    g_hooks.calloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    reset_hooks();
    g_hooks.malloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    reset_hooks();
    g_hooks.calloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    {
        ssz_merkle_cache_t *real_cache = NULL;
        const ssz_merkle_cache_config_t cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = SSZ_NO_LIMIT,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&cfg, &real_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(real_cache), SSZ_SUCCESS);
        real_cache->leaf_tokens[0] = 7u;
        real_cache->leaf_token_valid_bits[0] = 1u;
        ASSERT_ERR(ssz_merkle_cache_internal_grow(real_cache, 2u), SSZ_SUCCESS);
        ASSERT_U64_EQ(real_cache->leaf_tokens[0], 7u);
        ASSERT_TRUE((real_cache->leaf_token_valid_bits[0] & 1u) != 0u);
        ssz_merkle_cache_destroy(real_cache);
    }

    ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(&cache, 1u), SSZ_SUCCESS);

    cache.leaf_limit = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(&cache, 3u), SSZ_ERR_LIMIT_EXCEEDED);

    cache.leaf_limit = SSZ_NO_LIMIT;
    cache.leaf_capacity = (UINT64_MAX / 2u) + 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(&cache, UINT64_MAX), SSZ_ERR_OVERFLOW);

    return true;
}

static bool test_public_api_error_paths(void)
{
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t out_root;
    ssz_chunk_t leaf = make_chunk(0x40u);
    uint8_t bytes[8] = {0u};
    const uint8_t bits_ok[2] = {0x55u, 0x01u};
    const uint8_t bits_bad[2] = {0x55u, 0x80u};
    const ssz_merkle_cache_config_t list_cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 4u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    const ssz_merkle_cache_config_t vector_cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    ASSERT_ERR(ssz_merkle_cache_create(NULL, &cache), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, NULL), SSZ_ERR_INVALID_ARGUMENT);

    {
        const ssz_merkle_cache_config_t bad_cfg = {
            .initial_leaf_count = 5u,
            .leaf_limit = 4u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bad_cfg, &cache), SSZ_ERR_LIMIT_EXCEEDED);
    }

    {
        const ssz_hash_fn_t bad_hash = {
            .hash = NULL,
            .hash_2to1 = NULL,
            .hash_2to1_batch = NULL,
            .ctx = NULL,
        };
        const ssz_merkle_cache_config_t bad_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 4u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = &bad_hash,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bad_cfg, &cache), SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const ssz_merkle_cache_config_t huge_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = (UINT64_C(1) << 63u) + 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&huge_cfg, &cache), SSZ_ERR_OVERFLOW);
    }

    reset_hooks();
    g_hooks.calloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &cache), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.hash_2to1_fail_at = 1u;
    {
        const ssz_hash_fn_t custom_hash = {
            .hash = passthrough_hash,
            .hash_2to1 = NULL,
            .hash_2to1_batch = NULL,
            .ctx = NULL,
        };
        const ssz_merkle_cache_config_t custom_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 4u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = &custom_hash,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&custom_cfg, &cache), SSZ_ERR_HASH_FAILURE);
    }

    reset_hooks();
    g_hooks.mix_in_length_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_create(&list_cfg, &cache), SSZ_ERR_HASH_FAILURE);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(NULL, &out_root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_root(NULL, &out_root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_root(cache, NULL), SSZ_ERR_INVALID_ARGUMENT);

    cache->data_root_valid = false;
    cache->leaf_limit = 1u;
    cache->leaf_count = 2u;
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &out_root), SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &out_root), SSZ_ERR_LIMIT_EXCEEDED);
    cache->leaf_limit = SSZ_NO_LIMIT;
    cache->leaf_count = 0u;

    ASSERT_ERR(ssz_merkle_cache_update_root_range(NULL, 0u, &leaf, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, UINT64_MAX, &leaf, 1u), SSZ_ERR_OVERFLOW);

    {
        const ssz_merkle_cache_config_t bounded_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ssz_merkle_cache_destroy(cache);
        cache = NULL;
        ASSERT_ERR(ssz_merkle_cache_create(&bounded_cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, &leaf, 2u), SSZ_ERR_LIMIT_EXCEEDED);
        reset_hooks();
        g_hooks.u64_to_size_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, &leaf, 1u), SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_destroy(cache);
        cache = NULL;
    }

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_zero_range(NULL, 0u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_zero_range(cache, 0u, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_zero_range(cache, UINT64_MAX, 1u), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_zero_range(cache, 0u, 1u), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 1u), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(NULL, bytes, sizeof(bytes), 0u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, NULL, 1u, 0u), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.add_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, bytes, sizeof(bytes), 0u), SSZ_ERR_OVERFLOW);

    {
        const ssz_merkle_cache_config_t one_leaf_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ssz_merkle_cache_destroy(cache);
        cache = NULL;
        ASSERT_ERR(ssz_merkle_cache_create(&one_leaf_cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, bytes, sizeof(bytes) * 8u, 0u), SSZ_ERR_LIMIT_EXCEEDED);
        reset_hooks();
        g_hooks.mul_overflow_size_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, bytes, sizeof(bytes), 0u), SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_destroy(cache);
        cache = NULL;
    }

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(NULL, bytes, 1u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, bytes, 0u, 1u), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, bytes, 1u, 0u), SSZ_ERR_SCHEMA_INVALID);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, bytes, 1u, 1u), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, bytes, 1u, 1u), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, NULL, 1u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ssz_merkle_cache_destroy(cache);

    ASSERT_ERR(ssz_merkle_cache_create(&list_cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(cache, bytes, 1u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(NULL, bytes, 1u, 4u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, bytes, 1u, 4u, 0u), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, bytes, 5u, 4u, 1u), SSZ_ERR_LIMIT_EXCEEDED);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, bytes, 1u, 4u, 1u), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, bytes, 1u, 4u, 1u), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, NULL, 1u, 4u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    reset_hooks();
    g_hooks.mul_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(cache, bytes, 1u, 4u, 1u), SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(cache);

    {
        const ssz_merkle_cache_config_t bitvector_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitvector_cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(NULL, bits_ok, sizeof(bits_ok), 8u), SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, bits_ok, sizeof(bits_ok), 0u), SSZ_ERR_SCHEMA_INVALID);
        reset_hooks();
        g_hooks.bits_to_bytes_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, bits_ok, sizeof(bits_ok), 8u), SSZ_ERR_OVERFLOW);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, NULL, sizeof(bits_ok), 8u), SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, bits_bad, sizeof(bits_bad), 9u), SSZ_ERR_ENCODING_INVALID);
        reset_hooks();
        g_hooks.add_overflow_u64_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, bits_ok, sizeof(bits_ok), 8u), SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_destroy(cache);
    }

    {
        const ssz_merkle_cache_config_t bitlist_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = true,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitlist_cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(NULL, bits_ok, sizeof(bits_ok), 8u, 8u), SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, bits_ok, sizeof(bits_ok), 9u, 8u), SSZ_ERR_LIMIT_EXCEEDED);
        reset_hooks();
        g_hooks.bits_to_bytes_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, bits_ok, sizeof(bits_ok), 8u, 8u), SSZ_ERR_OVERFLOW);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, NULL, sizeof(bits_ok), 8u, 8u), SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, bits_bad, sizeof(bits_bad), 9u, 9u), SSZ_ERR_ENCODING_INVALID);
        reset_hooks();
        g_hooks.add_overflow_u64_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, bits_ok, sizeof(bits_ok), 8u, 8u), SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_destroy(cache);
    }

    return true;
}

static bool test_composite_paths(void)
{
    ssz_chunk_t roots[4] = {
        make_chunk(0x10u),
        make_chunk(0x20u),
        make_chunk(0x30u),
        make_chunk(0x40u),
    };
    uint64_t tokens[4] = {1u, 2u, 3u, 4u};
    composite_fixture_t fixture = {
        .roots = roots,
        .tokens = tokens,
        .count = 4u,
        .fail_index = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
    };
    const ssz_member_codec_t codec = {
        .ctx = &fixture,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_sync_composite_opts_t batch_opts = {
        .ctx = &fixture,
        .token = composite_token,
        .root_batch = composite_root_batch,
    };
    const ssz_merkle_cache_sync_composite_opts_t token_only_opts = {
        .ctx = &fixture,
        .token = composite_token,
        .root_batch = NULL,
    };
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 4u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    ssz_merkle_cache_t *cache = NULL;

    ASSERT_ERR(ssz_merkle_cache_sync_composite(NULL, 1u, 4u, &codec, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 5u, 4u, &codec, NULL), SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 1u, 3u, &codec, NULL), SSZ_ERR_INVALID_ARGUMENT);

    fixture.fail_index = 0u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 2u, 4u, &codec, NULL), SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(cache));
    fixture.fail_index = UINT64_MAX;

    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 2u, 4u, &codec, NULL), SSZ_SUCCESS);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(cache));

    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(NULL, &codec, NULL, 0u, 1u, NULL),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, NULL, NULL, 0u, 1u, NULL),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, NULL, 0u, 0u, NULL),
               SSZ_SUCCESS);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, &batch_opts, 0u, 1u, tokens),
               SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, &batch_opts, 0u, 1u, tokens),
               SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, &batch_opts, 0u, 1u, tokens),
               SSZ_ERR_OVERFLOW);

    fixture.fail_index = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, &batch_opts, 0u, 2u, tokens),
               SSZ_ERR_HASH_FAILURE);
    fixture.fail_index = UINT64_MAX;

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, NULL, 0u, 1u, tokens),
               SSZ_ERR_OVERFLOW);

    fixture.fail_index = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(cache, &codec, NULL, 0u, 2u, tokens),
               SSZ_ERR_HASH_FAILURE);
    fixture.fail_index = UINT64_MAX;

    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(cache, 1u, NULL, NULL),
               SSZ_ERR_INVALID_ARGUMENT);

    fixture.fail_index = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(cache, 2u, &codec, NULL),
               SSZ_ERR_HASH_FAILURE);
    fixture.fail_index = UINT64_MAX;

    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(cache, 2u, &codec, &batch_opts), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 2u, 4u, &codec, &token_only_opts), SSZ_SUCCESS);
    fixture.fail_index = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 2u, 4u, &codec, &token_only_opts), SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(cache));
    fixture.fail_index = UINT64_MAX;

    reset_hooks();
    tokens[0] = 9u;
    g_hooks.realloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 2u, 4u, &codec, &token_only_opts), SSZ_ERR_OVERFLOW);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(cache));
    tokens[0] = 1u;

    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 3u, 4u, &codec, &batch_opts), SSZ_SUCCESS);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(cache));

    ssz_merkle_cache_destroy(cache);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(NULL));
    return true;
}

static bool test_remaining_targeted_branches(void)
{
    const ssz_merkle_cache_config_t vector_cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };
    const ssz_merkle_cache_config_t list_cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 4u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    ssz_merkle_cache_t cache;
    ssz_chunk_t nodes[7];
    uint64_t dirty_bits[1] = {0u};
    size_t dirty_idx[1] = {0u};
    uint64_t scratch_bits[1] = {0u};
    size_t scratch_idx[1] = {0u};
    ssz_merkle_cache_dirty_set_t dirty_set = {0};
    ssz_merkle_cache_t *heap_cache = NULL;
    ssz_chunk_t leaf = make_chunk(0x66u);
    uint8_t bytes[64] = {0u};
    uint8_t bits[2] = {0x55u, 0x01u};
    ssz_chunk_t roots[3] = {make_chunk(0x01u), make_chunk(0x11u), make_chunk(0x21u)};
    uint64_t token_values[3] = {1u, 2u, 3u};
    composite_fixture_t fixture = {
        .roots = roots,
        .tokens = token_values,
        .count = 3u,
        .fail_index = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
    };
    composite_fixture_t root_fail_fixture = {
        .roots = roots,
        .tokens = token_values,
        .count = 3u,
        .fail_index = 0u,
        .fail_err = SSZ_ERR_HASH_FAILURE,
    };
    const ssz_member_codec_t codec = {
        .ctx = &fixture,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_member_codec_t root_fail_codec = {
        .ctx = &root_fail_fixture,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_sync_composite_opts_t batch_opts = {
        .ctx = &fixture,
        .token = composite_token,
        .root_batch = composite_root_batch,
    };
    const ssz_merkle_cache_sync_composite_opts_t token_only_opts = {
        .ctx = &fixture,
        .token = composite_token,
        .root_batch = NULL,
    };

    memset(bytes, 0xAB, sizeof(bytes));

    {
        uint64_t valid_bits[1] = {UINT64_C(0x3)};
        ssz_merkle_cache_t token_cache;
        memset(&token_cache, 0, sizeof(token_cache));
        token_cache.token_storage_ready = true;
        token_cache.leaf_token_valid_bits = valid_bits;
        token_cache.leaf_token_word_capacity = 1u;
        ssz_merkle_cache_internal_token_valid_clear_range(&token_cache, 0u, 2u);
        ASSERT_TRUE(valid_bits[0] == 0u);
    }

    memset(&cache, 0, sizeof(cache));
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 0u), SSZ_SUCCESS);
    cache.scratch_pair_capacity = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_gather_capacity(&cache, 0u), SSZ_SUCCESS);

    init_manual_cache(&cache, nodes, 4u, 2u, dirty_bits, dirty_idx, 1u);
    dirty_set.bits = dirty_bits;
    dirty_set.word_idx = dirty_idx;
    dirty_set.word_capacity = 1u;
    dirty_set.word_count = 1u;
    dirty_bits[0] = UINT64_C(0x1);
    dirty_idx[0] = 0u;
    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
               SSZ_ERR_OVERFLOW);

    init_manual_cache(&cache, nodes, 4u, 2u, dirty_bits, dirty_idx, 1u);
    cache.scratch_pair_capacity = 1u;
    cache.scratch_pairs =
        ssz_merkle_cache_internal_alloc_aligned32(2u * sizeof(*cache.scratch_pairs));
    cache.scratch_hashes =
        ssz_merkle_cache_internal_alloc_aligned32(1u * sizeof(*cache.scratch_hashes));
    cache.scratch_parent_indices = hook_calloc(1u, sizeof(*cache.scratch_parent_indices));
    dirty_set.bits = dirty_bits;
    dirty_set.word_idx = dirty_idx;
    dirty_set.word_capacity = 1u;
    dirty_set.word_count = 1u;
    dirty_bits[0] = UINT64_C(0x1);
    dirty_idx[0] = 0u;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
               SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_pairs);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_hashes);
    hook_free(cache.scratch_parent_indices);

    init_manual_cache(&cache, nodes, 4u, 2u, dirty_bits, dirty_idx, 1u);
    cache.scratch_pair_capacity = 2u;
    cache.scratch_pairs =
        ssz_merkle_cache_internal_alloc_aligned32(4u * sizeof(*cache.scratch_pairs));
    cache.scratch_hashes =
        ssz_merkle_cache_internal_alloc_aligned32(2u * sizeof(*cache.scratch_hashes));
    cache.scratch_parent_indices = hook_calloc(2u, sizeof(*cache.scratch_parent_indices));
    dirty_set.bits = dirty_bits;
    dirty_set.word_idx = dirty_idx;
    dirty_set.word_capacity = 1u;
    dirty_set.word_count = 1u;
    dirty_bits[0] = UINT64_C(0x5);
    dirty_idx[0] = 0u;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
               SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_pairs);
    ssz_merkle_cache_internal_free_aligned32(cache.scratch_hashes);
    hook_free(cache.scratch_parent_indices);

    for (size_t fail_at = 1u; fail_at <= 6u; fail_at++)
    {
        init_manual_cache(&cache, nodes, 4u, 2u, dirty_bits, dirty_idx, 1u);
        cache.scratch_pair_capacity = 2u;
        cache.scratch_pairs =
            ssz_merkle_cache_internal_alloc_aligned32(4u * sizeof(*cache.scratch_pairs));
        cache.scratch_hashes =
            ssz_merkle_cache_internal_alloc_aligned32(2u * sizeof(*cache.scratch_hashes));
        cache.scratch_parent_indices = hook_calloc(2u, sizeof(*cache.scratch_parent_indices));
        dirty_set.bits = dirty_bits;
        dirty_set.word_idx = dirty_idx;
        dirty_set.word_capacity = 1u;
        dirty_set.word_count = 1u;
        dirty_bits[0] = UINT64_C(0x5);
        dirty_idx[0] = 0u;
        reset_hooks();
        g_hooks.u64_to_size_fail_at = fail_at;
        ASSERT_ERR(ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 4u),
                   SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_internal_free_aligned32(cache.scratch_pairs);
        ssz_merkle_cache_internal_free_aligned32(cache.scratch_hashes);
        hook_free(cache.scratch_parent_indices);
    }

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.token_storage_ready = true;
    cache.leaf_tokens = hook_calloc(8u, sizeof(*cache.leaf_tokens));
    cache.leaf_token_valid_bits = hook_calloc(2u, sizeof(*cache.leaf_token_valid_bits));
    cache.leaf_token_capacity = 8u;
    cache.leaf_token_word_capacity = 2u;
    cache.leaf_capacity = 4u;
    cache.leaf_dirty_word_capacity = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(&cache), SSZ_SUCCESS);
    hook_free(cache.leaf_tokens);
    hook_free(cache.leaf_token_valid_bits);

    memset(&cache, 0, sizeof(cache));
    cache.token_storage_ready = true;
    cache.leaf_tokens = hook_calloc(8u, sizeof(*cache.leaf_tokens));
    cache.leaf_token_valid_bits = hook_calloc(2u, sizeof(*cache.leaf_token_valid_bits));
    cache.leaf_token_capacity = 8u;
    cache.leaf_token_word_capacity = 2u;
    {
        uint64_t *resized_tokens = NULL;
        uint64_t *resized_valid_bits = NULL;
        size_t resized_capacity = 0u;
        size_t resized_word_capacity = 0u;
        ASSERT_ERR(ssz_merkle_cache_internal_resize_token_storage(&cache,
                                                                  4u,
                                                                  1u,
                                                                  &resized_tokens,
                                                                  &resized_valid_bits,
                                                                  &resized_capacity,
                                                                  &resized_word_capacity),
                   SSZ_SUCCESS);
        hook_free(resized_tokens);
        hook_free(resized_valid_bits);
    }
    hook_free(cache.leaf_tokens);
    hook_free(cache.leaf_token_valid_bits);

    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 4u;
    ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);

    for (size_t fail_at = 5u; fail_at <= 6u; fail_at++)
    {
        init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
        reset_hooks();
        g_hooks.u64_to_size_fail_at = fail_at;
        ASSERT_ERR(ssz_merkle_cache_internal_grow(&cache, 4u), SSZ_ERR_OVERFLOW);
    }

    {
        ssz_merkle_cache_t *grow_cache = NULL;
        ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &grow_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(grow_cache), SSZ_SUCCESS);
        reset_hooks();
        g_hooks.calloc_fail_at = 4u;
        ASSERT_ERR(ssz_merkle_cache_internal_grow(grow_cache, 2u), SSZ_ERR_OVERFLOW);
        ssz_merkle_cache_destroy(grow_cache);
    }

    {
        ssz_merkle_cache_t *cap_cache = NULL;
        ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &cap_cache), SSZ_SUCCESS);
        reset_hooks();
        g_hooks.malloc_fail_at = 1u;
        ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(cap_cache, 2u), SSZ_ERR_OVERFLOW);
        for (size_t fail_at = 7u; fail_at <= 8u; fail_at++)
        {
            reset_hooks();
            g_hooks.u64_to_size_fail_at = fail_at;
            ASSERT_ERR(ssz_merkle_cache_internal_ensure_capacity_for_count(cap_cache, 2u),
                       SSZ_ERR_OVERFLOW);
        }
        ssz_merkle_cache_destroy(cap_cache);
    }

    ASSERT_ERR(ssz_merkle_cache_internal_compute_data_root_if_needed(NULL), SSZ_ERR_INVALID_ARGUMENT);
    init_manual_cache(&cache, nodes, 2u, 1u, dirty_bits, dirty_idx, 1u);
    cache.leaf_dirty_word_count = 1u;
    dirty_bits[0] = 1u;
    dirty_idx[0] = 0u;
    cache.scratch_dirty[0].bits = scratch_bits;
    cache.scratch_dirty[0].word_idx = scratch_idx;
    cache.scratch_dirty[0].word_capacity = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_compute_data_root_if_needed(&cache), SSZ_ERR_OVERFLOW);
    ASSERT_FALSE(ssz_merkle_cache_internal_limit_matches(NULL, 0u));

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.calloc_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.calloc_fail_at = 3u;
    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(NULL);
    ASSERT_ERR(ssz_merkle_cache_reset(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_internal_ensure_token_storage(heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_reset(heap_cache), SSZ_SUCCESS);
    ssz_merkle_cache_destroy(heap_cache);
    heap_cache = NULL;

    ASSERT_ERR(ssz_merkle_cache_create(&list_cfg, &heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(heap_cache, 1u), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.mix_in_length_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_root(heap_cache, &leaf), SSZ_ERR_HASH_FAILURE);
    ssz_merkle_cache_destroy(heap_cache);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_update_root_range(heap_cache, 0u, &leaf, 2u), SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(heap_cache);

    {
        const ssz_merkle_cache_config_t bounded_one = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bounded_one, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_zero_range(heap_cache, 1u, 1u), SSZ_ERR_LIMIT_EXCEEDED);
        ssz_merkle_cache_destroy(heap_cache);
    }

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_zero_range(heap_cache, 0u, 2u), SSZ_SUCCESS);
    ssz_merkle_cache_destroy(heap_cache);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(heap_cache, bytes, sizeof(bytes), 0u), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(heap_cache, bytes, 1u, 0u), SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(heap_cache, bytes, 64u, 0u), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(heap_cache, bytes, 1u, 0u), SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(heap_cache);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(heap_cache, bytes, 1u, 1u), SSZ_SUCCESS);
    ssz_merkle_cache_destroy(heap_cache);

    {
        const ssz_merkle_cache_config_t vector_mixed_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = SSZ_NO_LIMIT,
            .logical_length = 0u,
            .mix_in_length = true,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&vector_mixed_cfg, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_packed_vector_fixed(heap_cache, bytes, 1u, 1u), SSZ_ERR_INVALID_ARGUMENT);
        ssz_merkle_cache_destroy(heap_cache);
    }

    ASSERT_ERR(ssz_merkle_cache_create(&list_cfg, &heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(heap_cache, bytes, 1u, 40u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ssz_merkle_cache_destroy(heap_cache);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(heap_cache, bytes, 1u, SSZ_NO_LIMIT, 1u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_bitvector(heap_cache, bits, sizeof(bits), 8u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_sync_bitlist(heap_cache, bits, sizeof(bits), 8u, 8u), SSZ_ERR_INVALID_ARGUMENT);
    ssz_merkle_cache_destroy(heap_cache);

    {
        const ssz_merkle_cache_config_t bitvector_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitvector_cfg, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(heap_cache, bits, sizeof(bits), 257u), SSZ_ERR_INVALID_ARGUMENT);
        ssz_merkle_cache_destroy(heap_cache);
    }

    {
        const ssz_merkle_cache_config_t bitvector_mixed_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = true,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitvector_mixed_cfg, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(heap_cache, bits, sizeof(bits), 8u), SSZ_ERR_INVALID_ARGUMENT);
        ssz_merkle_cache_destroy(heap_cache);
    }

    {
        const ssz_merkle_cache_config_t bitlist_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = true,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitlist_cfg, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(heap_cache, bits, sizeof(bits), 8u, 257u),
                   SSZ_ERR_INVALID_ARGUMENT);
        ssz_merkle_cache_destroy(heap_cache);
    }

    {
        const ssz_merkle_cache_config_t bitlist_unmixed_cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ASSERT_ERR(ssz_merkle_cache_create(&bitlist_unmixed_cfg, &heap_cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(heap_cache, bits, sizeof(bits), 8u, 8u),
                   SSZ_ERR_INVALID_ARGUMENT);
        ssz_merkle_cache_destroy(heap_cache);
    }

    ASSERT_ERR(ssz_merkle_cache_create(&list_cfg, &heap_cache), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(heap_cache, &codec, &batch_opts, 0u, 1u, token_values),
               SSZ_ERR_OVERFLOW);
    reset_hooks();
    heap_cache->token_storage_ready = true;
    heap_cache->leaf_tokens = hook_calloc(1u, sizeof(*heap_cache->leaf_tokens));
    heap_cache->leaf_token_valid_bits = hook_calloc(1u, sizeof(*heap_cache->leaf_token_valid_bits));
    heap_cache->leaf_token_capacity = 0u;
    heap_cache->leaf_token_word_capacity = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(heap_cache, &codec, &batch_opts, 0u, 1u, token_values),
               SSZ_ERR_OVERFLOW);
    hook_free(heap_cache->leaf_tokens);
    hook_free(heap_cache->leaf_token_valid_bits);
    heap_cache->leaf_tokens = NULL;
    heap_cache->leaf_token_valid_bits = NULL;
    heap_cache->token_storage_ready = true;
    heap_cache->leaf_tokens = hook_calloc(1u, sizeof(*heap_cache->leaf_tokens));
    heap_cache->leaf_token_valid_bits = hook_calloc(1u, sizeof(*heap_cache->leaf_token_valid_bits));
    heap_cache->leaf_token_capacity = 0u;
    heap_cache->leaf_token_word_capacity = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_run(heap_cache, &codec, NULL, 0u, 1u, token_values),
               SSZ_ERR_OVERFLOW);
    hook_free(heap_cache->leaf_tokens);
    hook_free(heap_cache->leaf_token_valid_bits);
    heap_cache->leaf_tokens = NULL;
    heap_cache->leaf_token_valid_bits = NULL;
    heap_cache->leaf_token_capacity = 0u;
    heap_cache->leaf_token_word_capacity = 0u;
    heap_cache->token_storage_ready = false;

    fixture.fail_index = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(heap_cache, 2u, &codec, &batch_opts),
               SSZ_ERR_HASH_FAILURE);
    fixture.fail_index = UINT64_MAX;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(heap_cache, 1u, &codec, NULL),
               SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(heap_cache, 0u, roots, 2u), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_sync_composite_fallback(heap_cache, 0u, &codec, NULL),
               SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(heap_cache);

    ASSERT_ERR(ssz_merkle_cache_create(&vector_cfg, &heap_cache), SSZ_SUCCESS);
    reset_hooks();
    g_hooks.malloc_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 2u, SSZ_NO_LIMIT, &codec, NULL),
               SSZ_ERR_OVERFLOW);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 1u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    token_values[0] = 9u;
    token_values[1] = 2u;
    token_values[2] = 3u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    token_values[0] = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);

    token_values[0] = 9u;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    token_values[0] = 9u;
    token_values[1] = 2u;
    token_values[2] = 3u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &root_fail_codec, &token_only_opts),
               SSZ_ERR_HASH_FAILURE);

    token_values[0] = 9u;
    token_values[2] = 7u;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 4u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    token_values[0] = 9u;
    token_values[1] = 2u;
    token_values[2] = 7u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    token_values[0] = 1u;
    token_values[2] = 3u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);

    token_values[0] = 9u;
    token_values[1] = 2u;
    token_values[2] = 7u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &root_fail_codec, &token_only_opts),
               SSZ_ERR_HASH_FAILURE);

    token_values[0] = 9u;
    token_values[1] = 8u;
    token_values[2] = 7u;
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 4u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    token_values[0] = 9u;
    token_values[1] = 8u;
    token_values[2] = 7u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &root_fail_codec, &token_only_opts),
               SSZ_ERR_HASH_FAILURE);
    token_values[0] = 1u;
    token_values[1] = 2u;
    token_values[2] = 3u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 3u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);

    token_values[0] = 1u;
    token_values[1] = 2u;
    token_values[2] = 3u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 2u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    reset_hooks();
    g_hooks.u64_to_size_fail_at = 2u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 1u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);

    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 2u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    token_values[0] = 9u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 1u, SSZ_NO_LIMIT, &root_fail_codec, &token_only_opts),
               SSZ_ERR_HASH_FAILURE);
    token_values[0] = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 2u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_SUCCESS);
    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(heap_cache, 1u, SSZ_NO_LIMIT, &codec, &token_only_opts),
               SSZ_ERR_OVERFLOW);
    ssz_merkle_cache_destroy(heap_cache);

    return true;
}

static bool test_grow_capacity(void)
{
    size_t out = 0u;

    /* From zero capacity, should pick initial cap of 8 and double to fit. */
    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(0u, 1u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 8u);

    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(0u, 9u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 16u);

    /* From existing capacity, doubles to fit. */
    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(4u, 5u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 8u);

    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(8u, 17u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 32u);

    /* Already sufficient — returns current. */
    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(16u, 16u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 16u);

    /* Overflow: required is so large that doubling overflows SIZE_MAX. */
    ASSERT_ERR(ssz_merkle_cache_internal_grow_capacity(SIZE_MAX / 2u + 1u, SIZE_MAX, &out),
               SSZ_ERR_OVERFLOW);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"basic_internal_helpers", test_basic_internal_helpers},
        {"dirty_and_token_helpers", test_dirty_and_token_helpers},
        {"depth_gather_and_recompute_paths", test_depth_gather_and_recompute_paths},
        {"storage_and_growth_paths", test_storage_and_growth_paths},
        {"public_api_error_paths", test_public_api_error_paths},
        {"composite_paths", test_composite_paths},
        {"remaining_targeted_branches", test_remaining_targeted_branches},
        {"grow_capacity", test_grow_capacity},
    };

    for (size_t i = 0u; i < (sizeof(tests) / sizeof(tests[0])); i++)
    {
        reset_hooks();
        if (!tests[i].fn())
        {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    printf("[OK] %zu/%zu merkle_cache_internal tests passed\n",
           sizeof(tests) / sizeof(tests[0]),
           sizeof(tests) / sizeof(tests[0]));
    return 0;
}
