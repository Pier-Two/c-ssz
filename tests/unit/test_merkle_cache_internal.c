#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssz_internal.h"

#include "ssz.h"

static ssz_chunk_t g_test_merkle_cache_internal_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_test_merkle_cache_internal_scratch = {
    .chunks = g_test_merkle_cache_internal_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

#define ssz_merkleize(chunks, chunk_count, limit, hash_fn, out_root) \
    ssz_merkleize(                                                   \
        (chunks),                                                    \
        (chunk_count),                                               \
        (limit),                                                     \
        &g_test_merkle_cache_internal_scratch,                       \
        (hash_fn),                                                   \
        (out_root))

typedef bool (*test_fn_t)(void);

typedef struct
{
    const char *name;
    test_fn_t fn;
} test_case_t;

#define ASSERT_TRUE(cond)                                                                  \
    do                                                                                     \
    {                                                                                      \
        if (!(cond))                                                                       \
        {                                                                                  \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            return false;                                                                  \
        }                                                                                  \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_ERR(expr, expected)                                                    \
    do                                                                                \
    {                                                                                 \
        ssz_error_t _actual = (expr);                                                 \
        if (_actual != (expected))                                                    \
        {                                                                             \
            fprintf(                                                                  \
                stderr,                                                               \
                "Assertion failed at %s:%d: %s returned %s (%d), expected %s (%d)\n", \
                __FILE__,                                                             \
                __LINE__,                                                             \
                #expr,                                                                \
                ssz_error_string(_actual),                                            \
                (int)_actual,                                                         \
                ssz_error_string((expected)),                                         \
                (int)(expected));                                                     \
            return false;                                                             \
        }                                                                             \
    } while (0)

#define ASSERT_U64_EQ(actual, expected)                                    \
    do                                                                     \
    {                                                                      \
        uint64_t _actual = (actual);                                       \
        uint64_t _expected = (expected);                                   \
        if (_actual != _expected)                                          \
        {                                                                  \
            fprintf(                                                       \
                stderr,                                                    \
                "Assertion failed at %s:%d: %" PRIu64 " != %" PRIu64 "\n", \
                __FILE__,                                                  \
                __LINE__,                                                  \
                _actual,                                                   \
                _expected);                                                \
            return false;                                                  \
        }                                                                  \
    } while (0)

#define ASSERT_SIZE_EQ(actual, expected)                   \
    do                                                     \
    {                                                      \
        size_t _actual = (actual);                         \
        size_t _expected = (expected);                     \
        if (_actual != _expected)                          \
        {                                                  \
            fprintf(                                       \
                stderr,                                    \
                "Assertion failed at %s:%d: %zu != %zu\n", \
                __FILE__,                                  \
                __LINE__,                                  \
                _actual,                                   \
                _expected);                                \
            return false;                                  \
        }                                                  \
    } while (0)

#define ASSERT_CHUNK_EQ(actual, expected)                                                       \
    do                                                                                          \
    {                                                                                           \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) != 0)                 \
        {                                                                                       \
            fprintf(stderr, "Assertion failed at %s:%d: chunk mismatch\n", __FILE__, __LINE__); \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

typedef struct
{
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
    size_t hash_2to1_batch_inplace_calls;
    size_t hash_2to1_batch_inplace_fail_at;
    ssz_error_t hash_2to1_batch_inplace_fail_err;
    size_t mix_in_length_calls;
    size_t mix_in_length_fail_at;
    ssz_error_t mix_in_length_fail_err;
} hook_state_t;

static hook_state_t g_hooks;

static void reset_hooks(void)
{
    (void)memset(&g_hooks, 0, sizeof(g_hooks));
    g_hooks.hash_2to1_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_inplace_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.mix_in_length_fail_err = SSZ_ERR_HASH_FAILURE;
}

static bool hook_should_fail(size_t *counter, size_t fail_at)
{
    (*counter)++;
    return (fail_at != 0u) && (*counter == fail_at);
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

static ssz_error_t hook_hash_2to1_batch_inplace(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *nodes,
    size_t pair_count)
{
    if (hook_should_fail(
            &g_hooks.hash_2to1_batch_inplace_calls,
            g_hooks.hash_2to1_batch_inplace_fail_at))
    {
        return g_hooks.hash_2to1_batch_inplace_fail_err;
    }
    return ssz_hash_2to1_batch_inplace(hash_fn, nodes, pair_count);
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

#define ssz_internal_u64_to_size       hook_u64_to_size
#define ssz_internal_add_overflow_size hook_add_overflow_size
#define ssz_internal_mul_overflow_size hook_mul_overflow_size
#define ssz_internal_add_overflow_u64  hook_add_overflow_u64
#define ssz_internal_mul_overflow_u64  hook_mul_overflow_u64
#define ssz_internal_bits_to_bytes     hook_bits_to_bytes
#define ssz_hash_2to1                  hook_hash_2to1
#define ssz_hash_2to1_batch            hook_hash_2to1_batch
#define ssz_hash_2to1_batch_inplace    hook_hash_2to1_batch_inplace
#define ssz_mix_in_length_u64          hook_mix_in_length_u64
#include "ssz_merkle_cache.c"
#undef ssz_internal_u64_to_size
#undef ssz_internal_add_overflow_size
#undef ssz_internal_mul_overflow_size
#undef ssz_internal_add_overflow_u64
#undef ssz_internal_mul_overflow_u64
#undef ssz_internal_bits_to_bytes
#undef ssz_hash_2to1
#undef ssz_hash_2to1_batch
#undef ssz_hash_2to1_batch_inplace
#undef ssz_mix_in_length_u64

typedef struct
{
    ssz_merkle_cache_t cache;
    ssz_merkle_cache_requirements_t requirements;
    ssz_merkle_cache_storage_t storage;
    ssz_merkle_cache_sync_workspace_t workspace;
    ssz_chunk_t *nodes;
    uint64_t *leaf_dirty_bits;
    size_t *leaf_dirty_word_idx;
    uint64_t *parent_dirty_bits[2];
    size_t *parent_dirty_word_idx[2];
    ssz_chunk_t *gather_pairs;
    ssz_chunk_t *gather_hashes;
    size_t *gather_parent_indices;
    uint64_t *token_values;
    uint64_t *token_valid_bits;
    ssz_chunk_t *root_batch_roots;
} cache_fixture_t;

typedef struct
{
    const ssz_chunk_t *roots;
    const uint64_t *tokens;
    uint64_t count;
    uint64_t fail_index;
    ssz_error_t fail_err;
    uint64_t root_calls;
    uint64_t token_calls;
    uint64_t batch_calls;
} composite_fixture_t;

typedef struct
{
    composite_fixture_t composite;
    uint64_t root_fail_index;
    ssz_error_t root_fail_err;
} composite_root_fail_fixture_t;

static void *alloc_zeroed(size_t count, size_t element_size)
{
    void *ptr = NULL;

    if (count != 0u)
    {
        ptr = calloc(count, element_size);
    }
    else
    {
        ptr = NULL;
    }

    return ptr;
}

static void cache_fixture_cleanup(cache_fixture_t *fixture)
{
    if (fixture != NULL)
    {
        free(fixture->nodes);
        free(fixture->leaf_dirty_bits);
        free(fixture->leaf_dirty_word_idx);
        free(fixture->parent_dirty_bits[0]);
        free(fixture->parent_dirty_bits[1]);
        free(fixture->parent_dirty_word_idx[0]);
        free(fixture->parent_dirty_word_idx[1]);
        free(fixture->gather_pairs);
        free(fixture->gather_hashes);
        free(fixture->gather_parent_indices);
        free(fixture->token_values);
        free(fixture->token_valid_bits);
        free(fixture->root_batch_roots);
        (void)memset(fixture, 0, sizeof(*fixture));
    }
    else
    {
        /* intentionally empty */
    }
}

static ssz_merkle_cache_config_t make_cache_config(
    uint64_t initial_leaf_count,
    uint64_t leaf_limit,
    uint64_t reserved_leaf_capacity,
    uint64_t logical_length,
    bool mix_in_length,
    const ssz_hash_fn_t *hash_fn)
{
    ssz_merkle_cache_config_t config;

    (void)memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.initial_leaf_count = initial_leaf_count;
    config.leaf_limit = leaf_limit;
    config.reserved_leaf_capacity = reserved_leaf_capacity;
    config.logical_length = logical_length;
    config.mix_in_length = mix_in_length;
    config.hash_fn = hash_fn;

    return config;
}

static ssz_merkle_cache_sync_composite_opts_t make_composite_opts(
    const void *ctx,
    ssz_merkle_cache_token_fn_t token,
    ssz_merkle_cache_root_batch_fn_t root_batch,
    ssz_merkle_cache_sync_workspace_t *workspace)
{
    ssz_merkle_cache_sync_composite_opts_t opts;

    (void)memset(&opts, 0, sizeof(opts));
    opts.struct_size = sizeof(opts);
    opts.ctx = ctx;
    opts.token = token;
    opts.root_batch = root_batch;
    opts.workspace = workspace;

    return opts;
}

static ssz_error_t cache_fixture_init(
    cache_fixture_t *fixture,
    const ssz_merkle_cache_config_t *config,
    bool with_tokens)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((fixture == NULL) || (config == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(fixture, 0, sizeof(*fixture));
        err = ssz_merkle_cache_requirements(config, &fixture->requirements);
    }

    if (err == SSZ_SUCCESS)
    {
        fixture->nodes = alloc_zeroed(fixture->requirements.nodes_count, sizeof(*fixture->nodes));
        fixture->leaf_dirty_bits =
            alloc_zeroed(fixture->requirements.leaf_dirty_words, sizeof(*fixture->leaf_dirty_bits));
        fixture->leaf_dirty_word_idx = alloc_zeroed(
            fixture->requirements.leaf_dirty_words,
            sizeof(*fixture->leaf_dirty_word_idx));
        fixture->parent_dirty_bits[0] = alloc_zeroed(
            fixture->requirements.parent_dirty_words,
            sizeof(*fixture->parent_dirty_bits[0]));
        fixture->parent_dirty_bits[1] = alloc_zeroed(
            fixture->requirements.parent_dirty_words,
            sizeof(*fixture->parent_dirty_bits[1]));
        fixture->parent_dirty_word_idx[0] = alloc_zeroed(
            fixture->requirements.parent_dirty_words,
            sizeof(*fixture->parent_dirty_word_idx[0]));
        fixture->parent_dirty_word_idx[1] = alloc_zeroed(
            fixture->requirements.parent_dirty_words,
            sizeof(*fixture->parent_dirty_word_idx[1]));
        fixture->gather_pairs =
            alloc_zeroed(fixture->requirements.gather_pairs_count, sizeof(*fixture->gather_pairs));
        fixture->gather_hashes = alloc_zeroed(
            fixture->requirements.gather_hashes_count,
            sizeof(*fixture->gather_hashes));
        fixture->gather_parent_indices = alloc_zeroed(
            fixture->requirements.gather_parent_indices_count,
            sizeof(*fixture->gather_parent_indices));
        fixture->root_batch_roots = alloc_zeroed(
            fixture->requirements.root_batch_roots_count,
            sizeof(*fixture->root_batch_roots));

        if (with_tokens)
        {
            fixture->token_values = alloc_zeroed(
                fixture->requirements.token_values_count,
                sizeof(*fixture->token_values));
            fixture->token_valid_bits = alloc_zeroed(
                fixture->requirements.token_valid_words,
                sizeof(*fixture->token_valid_bits));
        }
        else
        {
            fixture->token_values = NULL;
            fixture->token_valid_bits = NULL;
        }

        if (((fixture->requirements.nodes_count != 0u) && (fixture->nodes == NULL)) ||
            ((fixture->requirements.leaf_dirty_words != 0u) &&
             ((fixture->leaf_dirty_bits == NULL) || (fixture->leaf_dirty_word_idx == NULL))) ||
            ((fixture->requirements.parent_dirty_words != 0u) &&
             ((fixture->parent_dirty_bits[0] == NULL) || (fixture->parent_dirty_bits[1] == NULL) ||
              (fixture->parent_dirty_word_idx[0] == NULL) ||
              (fixture->parent_dirty_word_idx[1] == NULL))) ||
            ((fixture->requirements.gather_pairs_count != 0u) &&
             ((fixture->gather_pairs == NULL) || (fixture->gather_parent_indices == NULL))) ||
            ((fixture->requirements.gather_hashes_count != 0u) &&
             (fixture->gather_hashes == NULL)) ||
            ((fixture->requirements.root_batch_roots_count != 0u) &&
             (fixture->root_batch_roots == NULL)) ||
            (with_tokens && (((fixture->requirements.token_values_count != 0u) &&
                              (fixture->token_values == NULL)) ||
                             ((fixture->requirements.token_valid_words != 0u) &&
                              (fixture->token_valid_bits == NULL)))))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            fixture->storage.struct_size = sizeof(fixture->storage);
            fixture->storage.nodes = fixture->nodes;
            fixture->storage.nodes_count = fixture->requirements.nodes_count;
            fixture->storage.leaf_dirty_bits = fixture->leaf_dirty_bits;
            fixture->storage.leaf_dirty_words = fixture->requirements.leaf_dirty_words;
            fixture->storage.leaf_dirty_word_idx = fixture->leaf_dirty_word_idx;
            fixture->storage.leaf_dirty_word_idx_count = fixture->requirements.leaf_dirty_words;
            fixture->storage.parent_dirty_bits[0] = fixture->parent_dirty_bits[0];
            fixture->storage.parent_dirty_bits[1] = fixture->parent_dirty_bits[1];
            fixture->storage.parent_dirty_words = fixture->requirements.parent_dirty_words;
            fixture->storage.parent_dirty_word_idx[0] = fixture->parent_dirty_word_idx[0];
            fixture->storage.parent_dirty_word_idx[1] = fixture->parent_dirty_word_idx[1];
            fixture->storage.parent_dirty_word_idx_count = fixture->requirements.parent_dirty_words;
            fixture->storage.gather_pairs = fixture->gather_pairs;
            fixture->storage.gather_pairs_count = fixture->requirements.gather_pairs_count;
            fixture->storage.gather_hashes = fixture->gather_hashes;
            fixture->storage.gather_hashes_count = fixture->requirements.gather_hashes_count;
            fixture->storage.gather_parent_indices = fixture->gather_parent_indices;
            fixture->storage.gather_parent_indices_count =
                fixture->requirements.gather_parent_indices_count;
            fixture->storage.token_values = fixture->token_values;
            fixture->storage.token_values_count =
                with_tokens ? fixture->requirements.token_values_count : 0u;
            fixture->storage.token_valid_bits = fixture->token_valid_bits;
            fixture->storage.token_valid_words =
                with_tokens ? fixture->requirements.token_valid_words : 0u;

            fixture->workspace.struct_size = sizeof(fixture->workspace);
            fixture->workspace.root_batch_roots = fixture->root_batch_roots;
            fixture->workspace.root_batch_roots_count =
                fixture->requirements.root_batch_roots_count;

            err = ssz_merkle_cache_bind(config, &fixture->storage, &fixture->cache);
        }
    }
    else
    {
        /* intentionally empty */
    }

    if (err != SSZ_SUCCESS)
    {
        cache_fixture_cleanup(fixture);
    }
    else
    {
        /* intentionally empty */
    }

    return err;
}

static ssz_chunk_t make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_chunk_t zero_chunk(void)
{
    ssz_chunk_t chunk;
    (void)memset(chunk.bytes, 0, sizeof(chunk.bytes));
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

static ssz_error_t composite_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    composite_fixture_t *fixture = (composite_fixture_t *)ctx;
    if ((fixture == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    fixture->root_calls++;
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
    composite_fixture_t *fixture = (composite_fixture_t *)ctx;
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
    fixture->batch_calls++;
    for (uint64_t i = 0u; i < count; i++)
    {
        out_roots[i] = fixture->roots[start_index + i];
    }
    return SSZ_SUCCESS;
}

static ssz_error_t composite_token(const void *ctx, uint64_t member_id, uint64_t *out_token)
{
    composite_fixture_t *fixture = (composite_fixture_t *)ctx;
    if ((fixture == NULL) || (out_token == NULL) || (fixture->tokens == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    fixture->token_calls++;
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

static ssz_error_t composite_root_fail_only(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    composite_root_fail_fixture_t *fixture = (composite_root_fail_fixture_t *)ctx;
    if ((fixture == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    fixture->composite.root_calls++;
    if (member_id == fixture->root_fail_index)
    {
        return fixture->root_fail_err;
    }
    if (member_id >= fixture->composite.count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_root = fixture->composite.roots[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t composite_token_fail_only(const void *ctx, uint64_t member_id, uint64_t *out_token)
{
    composite_root_fail_fixture_t *fixture = (composite_root_fail_fixture_t *)ctx;
    if ((fixture == NULL) || (out_token == NULL) || (fixture->composite.tokens == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    fixture->composite.token_calls++;
    if (member_id == fixture->composite.fail_index)
    {
        return fixture->composite.fail_err;
    }
    if (member_id >= fixture->composite.count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_token = fixture->composite.tokens[member_id];
    return SSZ_SUCCESS;
}

static void init_manual_cache(
    ssz_merkle_cache_t *cache,
    ssz_chunk_t *nodes,
    uint64_t leaf_capacity,
    uint64_t mutable_leaf_capacity,
    uint32_t depth,
    uint64_t *leaf_dirty_bits,
    size_t *leaf_dirty_word_idx,
    size_t leaf_dirty_word_capacity,
    uint64_t *parent_dirty_bits0,
    size_t *parent_dirty_word_idx0,
    uint64_t *parent_dirty_bits1,
    size_t *parent_dirty_word_idx1,
    size_t parent_dirty_word_capacity,
    ssz_chunk_t *gather_pairs,
    ssz_chunk_t *gather_hashes,
    size_t *gather_parent_indices,
    size_t gather_pair_capacity,
    bool use_gather_inplace)
{
    (void)memset(cache, 0, sizeof(*cache));
    cache->struct_size = sizeof(*cache);
    cache->hash_fn = ssz_hash_default();
    cache->zero_hashes = ssz_hash_default_zero_hashes();
    cache->nodes = nodes;
    cache->leaf_capacity = leaf_capacity;
    cache->mutable_leaf_capacity = mutable_leaf_capacity;
    cache->depth = depth;
    cache->leaf_limit = SSZ_NO_LIMIT;
    cache->leaf_dirty_bits = leaf_dirty_bits;
    cache->leaf_dirty_word_idx = leaf_dirty_word_idx;
    cache->leaf_dirty_word_capacity = leaf_dirty_word_capacity;
    cache->parent_dirty_bits[0] = parent_dirty_bits0;
    cache->parent_dirty_bits[1] = parent_dirty_bits1;
    cache->parent_dirty_word_idx[0] = parent_dirty_word_idx0;
    cache->parent_dirty_word_idx[1] = parent_dirty_word_idx1;
    cache->parent_dirty_word_capacity = parent_dirty_word_capacity;
    cache->gather_pairs = gather_pairs;
    cache->gather_hashes = gather_hashes;
    cache->gather_parent_indices = gather_parent_indices;
    cache->gather_pair_capacity = gather_pair_capacity;
    cache->use_gather_inplace = use_gather_inplace;
    (void)
        ssz_merkle_cache_internal_compute_level_offsets(leaf_capacity, depth, cache->level_offsets);
}

static bool test_requirements_and_bind_paths(void)
{
    ssz_merkle_cache_requirements_t req;
    ssz_merkle_cache_requirements_t custom_req;
    const ssz_hash_fn_t custom_hash = {
        .hash = passthrough_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    const ssz_merkle_cache_config_t bounded_cfg = make_cache_config(0u, 5u, 0u, 0u, true, NULL);
    const ssz_merkle_cache_config_t custom_cfg =
        make_cache_config(0u, 5u, 0u, 0u, true, &custom_hash);
    const ssz_merkle_cache_config_t bad_exact_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 6u, 0u, false, NULL);
    const ssz_merkle_cache_config_t too_small_exact_cfg =
        make_cache_config(9u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);
    cache_fixture_t fixture;
    composite_fixture_t composite = {0};
    ssz_chunk_t roots[2] = {make_chunk(0x11u), make_chunk(0x22u)};
    uint64_t tokens[2] = {1u, 2u};
    const ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    ssz_merkle_cache_sync_composite_opts_t token_opts;
    ssz_merkle_cache_storage_t storage;
    ssz_merkle_cache_t cache;

    ASSERT_ERR(ssz_merkle_cache_requirements(&bounded_cfg, &req), SSZ_SUCCESS);
    ASSERT_U64_EQ(req.physical_leaf_capacity, 8u);
    ASSERT_U64_EQ(req.mutable_leaf_capacity, 5u);
    ASSERT_U64_EQ(req.depth, 3u);
    ASSERT_SIZE_EQ(req.nodes_count, 15u);
    ASSERT_SIZE_EQ(req.leaf_dirty_words, 1u);
    ASSERT_SIZE_EQ(req.parent_dirty_words, 1u);
    ASSERT_SIZE_EQ(req.gather_pair_capacity, 2u);
    ASSERT_SIZE_EQ(req.gather_pairs_count, 4u);
    ASSERT_SIZE_EQ(req.gather_hashes_count, 0u);
    ASSERT_SIZE_EQ(req.gather_parent_indices_count, 2u);
    ASSERT_SIZE_EQ(req.token_values_count, 5u);
    ASSERT_SIZE_EQ(req.token_valid_words, 1u);
    ASSERT_SIZE_EQ(req.root_batch_roots_count, 5u);

    ASSERT_ERR(ssz_merkle_cache_requirements(&custom_cfg, &custom_req), SSZ_SUCCESS);
    ASSERT_SIZE_EQ(custom_req.gather_hashes_count, 2u);
    ASSERT_ERR(ssz_merkle_cache_requirements(&bad_exact_cfg, &req), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkle_cache_requirements(&too_small_exact_cfg, &req), SSZ_ERR_LIMIT_EXCEEDED);

    ASSERT_ERR(cache_fixture_init(&fixture, &custom_cfg, false), SSZ_SUCCESS);
    storage = fixture.storage;
    storage.gather_hashes = NULL;
    ASSERT_ERR(ssz_merkle_cache_bind(&custom_cfg, &storage, &cache), SSZ_ERR_BUFFER_TOO_SMALL);
    storage = fixture.storage;
    storage.struct_size = 0u;
    ASSERT_ERR(ssz_merkle_cache_bind(&custom_cfg, &storage, &cache), SSZ_ERR_INVALID_ARGUMENT);

    composite.roots = roots;
    composite.tokens = tokens;
    composite.count = 2u;
    composite.fail_index = UINT64_MAX;
    composite.fail_err = SSZ_ERR_HASH_FAILURE;
    token_opts = make_composite_opts(&composite, composite_token, NULL, NULL);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&fixture.cache, 2u, 5u, &codec, &token_opts),
        SSZ_ERR_INVALID_ARGUMENT);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_dirty_and_token_helpers(void)
{
    ssz_merkle_cache_dirty_set_t set;
    ssz_merkle_cache_t cache;
    ssz_chunk_t nodes[1] = {0};
    uint64_t leaf_dirty_bits[1] = {0u};
    size_t leaf_dirty_word_idx[1] = {0u};
    uint64_t parent_dirty_bits[2][1] = {{0u}, {0u}};
    size_t parent_dirty_word_idx[2][1] = {{0u}, {0u}};
    uint64_t token_valid_bits[1] = {0u};
    uint64_t token_values[1] = {0u};
    ssz_chunk_t leaf = make_chunk(0x44u);

    (void)memset(&set, 0, sizeof(set));
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_mark(NULL, 0u), SSZ_ERR_INVALID_ARGUMENT);
    ssz_merkle_cache_internal_bind_dirty_set(
        &set,
        leaf_dirty_bits,
        leaf_dirty_word_idx,
        &cache.leaf_dirty_word_count,
        1u);
    cache.leaf_dirty_word_count = 0u;
    ASSERT_ERR(ssz_merkle_cache_internal_dirty_set_mark(&set, 0u), SSZ_SUCCESS);
    ASSERT_SIZE_EQ(cache.leaf_dirty_word_count, 1u);
    ssz_merkle_cache_internal_dirty_set_clear(&set);
    ASSERT_SIZE_EQ(cache.leaf_dirty_word_count, 0u);

    init_manual_cache(
        &cache,
        nodes,
        1u,
        1u,
        0u,
        leaf_dirty_bits,
        leaf_dirty_word_idx,
        1u,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        parent_dirty_bits[1],
        parent_dirty_word_idx[1],
        0u,
        NULL,
        NULL,
        NULL,
        0u,
        true);

    ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));
    cache.token_storage_ready = true;
    cache.token_values = token_values;
    cache.token_valid_bits = token_valid_bits;
    cache.token_capacity = 1u;
    cache.token_word_capacity = 1u;
    ssz_merkle_cache_internal_token_valid_set(&cache, 0u, true);
    ASSERT_TRUE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));
    ssz_merkle_cache_internal_token_valid_clear_range(&cache, 0u, 1u);
    ASSERT_FALSE(ssz_merkle_cache_internal_token_valid_get(&cache, 0u));

    ASSERT_ERR(
        ssz_merkle_cache_internal_set_leaf(NULL, 0u, &leaf, false),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_merkle_cache_internal_set_leaf(&cache, 1u, &leaf, false),
        SSZ_ERR_LIMIT_EXCEEDED);
    cache.data_root_valid = true;
    cache.final_root_valid = true;
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, false), SSZ_SUCCESS);
    ASSERT_FALSE(cache.data_root_valid);
    ASSERT_FALSE(cache.final_root_valid);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaf, false), SSZ_SUCCESS);
    ASSERT_SIZE_EQ(cache.leaf_dirty_word_count, 1u);

    return true;
}

static bool test_hash_dirty_parent_paths(void)
{
    ssz_merkle_cache_t cache;
    ssz_merkle_cache_dirty_set_t dirty_set;
    ssz_chunk_t nodes[7];
    uint64_t leaf_dirty_bits[1] = {0u};
    size_t leaf_dirty_word_idx[1] = {0u};
    uint64_t parent_dirty_bits[2][1] = {{0u}, {0u}};
    size_t parent_dirty_word_idx[2][1] = {{0u}, {0u}};
    ssz_chunk_t gather_pairs[2];
    ssz_chunk_t gather_hashes[1];
    size_t gather_parent_indices[1] = {0u};
    ssz_chunk_t expected_parent;
    const ssz_hash_fn_t custom_hash = {
        .hash = passthrough_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };

    for (size_t i = 0u; i < 7u; i++)
    {
        nodes[i] = make_chunk((uint8_t)(0x10u + i));
    }

    init_manual_cache(
        &cache,
        nodes,
        4u,
        4u,
        2u,
        leaf_dirty_bits,
        leaf_dirty_word_idx,
        1u,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        parent_dirty_bits[1],
        parent_dirty_word_idx[1],
        1u,
        gather_pairs,
        gather_hashes,
        gather_parent_indices,
        1u,
        true);
    parent_dirty_bits[0][0] = UINT64_C(0x1);
    cache.parent_dirty_word_count[0] = 1u;
    ssz_merkle_cache_internal_bind_dirty_set(
        &dirty_set,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        &cache.parent_dirty_word_count[0],
        1u);
    parent_dirty_word_idx[0][0] = 0u;

    ASSERT_ERR(ssz_hash_2to1(cache.hash_fn, &nodes[0], &nodes[1], &expected_parent), SSZ_SUCCESS);
    reset_hooks();
    ASSERT_ERR(
        ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
        SSZ_SUCCESS);
    ASSERT_SIZE_EQ(g_hooks.hash_2to1_batch_inplace_calls, 1u);
    ASSERT_SIZE_EQ(g_hooks.hash_2to1_batch_calls, 0u);
    ASSERT_CHUNK_EQ(nodes[4], expected_parent);

    parent_dirty_bits[0][0] = UINT64_C(0x1);
    cache.parent_dirty_word_count[0] = 1u;
    reset_hooks();
    g_hooks.hash_2to1_batch_inplace_fail_at = 1u;
    ASSERT_ERR(
        ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
        SSZ_ERR_HASH_FAILURE);

    init_manual_cache(
        &cache,
        nodes,
        4u,
        4u,
        2u,
        leaf_dirty_bits,
        leaf_dirty_word_idx,
        1u,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        parent_dirty_bits[1],
        parent_dirty_word_idx[1],
        1u,
        gather_pairs,
        gather_hashes,
        gather_parent_indices,
        1u,
        false);
    cache.hash_fn = &custom_hash;
    cache.zero_hashes = cache.zero_hashes_buf;
    ASSERT_ERR(
        ssz_merkle_cache_internal_build_zero_hashes(&custom_hash, cache.zero_hashes_buf),
        SSZ_SUCCESS);
    parent_dirty_bits[0][0] = UINT64_C(0x1);
    cache.parent_dirty_word_count[0] = 1u;
    ssz_merkle_cache_internal_bind_dirty_set(
        &dirty_set,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        &cache.parent_dirty_word_count[0],
        1u);
    parent_dirty_word_idx[0][0] = 0u;
    reset_hooks();
    ASSERT_ERR(
        ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
        SSZ_SUCCESS);
    ASSERT_SIZE_EQ(g_hooks.hash_2to1_batch_calls, 1u);
    ASSERT_SIZE_EQ(g_hooks.hash_2to1_batch_inplace_calls, 0u);

    parent_dirty_bits[0][0] = UINT64_C(0x1);
    cache.parent_dirty_word_count[0] = 1u;
    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    ASSERT_ERR(
        ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
        SSZ_ERR_HASH_FAILURE);

    parent_dirty_bits[0][0] = UINT64_C(0x3);
    cache.parent_dirty_word_count[0] = 1u;
    reset_hooks();
    ASSERT_ERR(
        ssz_merkle_cache_internal_hash_dirty_parents_exact(&cache, 0u, &dirty_set, 2u),
        SSZ_SUCCESS);
    ASSERT_SIZE_EQ(g_hooks.hash_2to1_batch_calls, 1u);

    return true;
}

static bool test_recompute_and_public_error_paths(void)
{
    ssz_merkle_cache_t cache;
    ssz_chunk_t nodes[7];
    uint64_t leaf_dirty_bits[1] = {0u};
    size_t leaf_dirty_word_idx[1] = {0u};
    uint64_t parent_dirty_bits[2][1] = {{0u}, {0u}};
    size_t parent_dirty_word_idx[2][1] = {{0u}, {0u}};
    ssz_chunk_t gather_pairs[2];
    size_t gather_parent_indices[1] = {0u};
    ssz_chunk_t leaves[4];
    ssz_chunk_t expected;
    ssz_chunk_t expected_leaves[3];
    cache_fixture_t fixture;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, true, NULL);
    const uint8_t bits_ok[2] = {0x55u, 0x01u};
    const uint8_t bits_bad[2] = {0x55u, 0x80u};

    for (size_t i = 0u; i < 7u; i++)
    {
        nodes[i] = zero_chunk();
    }

    init_manual_cache(
        &cache,
        nodes,
        4u,
        4u,
        2u,
        leaf_dirty_bits,
        leaf_dirty_word_idx,
        1u,
        parent_dirty_bits[0],
        parent_dirty_word_idx[0],
        parent_dirty_bits[1],
        parent_dirty_word_idx[1],
        1u,
        gather_pairs,
        NULL,
        gather_parent_indices,
        1u,
        true);
    ssz_merkle_cache_internal_fill_zero_tree(&cache);
    leaves[0] = make_chunk(0x01u);
    leaves[1] = make_chunk(0x11u);
    leaves[2] = make_chunk(0x21u);
    leaves[3] = make_chunk(0x31u);

    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 0u, &leaves[0], false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 2u, &leaves[2], false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_SUCCESS);
    cache.leaf_count = 3u;
    ASSERT_ERR(ssz_merkle_cache_internal_refresh_cached_data_root(&cache), SSZ_SUCCESS);
    expected_leaves[0] = leaves[0];
    expected_leaves[1] = zero_chunk();
    expected_leaves[2] = leaves[2];
    ASSERT_ERR(ssz_merkleize(expected_leaves, 3u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(cache.cached_data_root, expected);

    reset_hooks();
    g_hooks.hash_2to1_batch_inplace_fail_at = 1u;
    ASSERT_ERR(ssz_merkle_cache_internal_set_leaf(&cache, 1u, &leaves[1], true), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_internal_recompute_data_root(&cache), SSZ_ERR_HASH_FAILURE);

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(NULL, &expected), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_merkle_cache_update_root_range(&fixture.cache, 4u, &leaves[0], 1u),
        SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(ssz_merkle_cache_zero_range(&fixture.cache, 4u, 1u), SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(
        ssz_merkle_cache_sync_bitvector(&fixture.cache, bits_bad, sizeof(bits_bad), 9u),
        SSZ_ERR_ENCODING_INVALID);
    reset_hooks();
    g_hooks.bits_to_bytes_fail_at = 1u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_bitvector(&fixture.cache, bits_ok, sizeof(bits_ok), 8u),
        SSZ_ERR_OVERFLOW);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_composite_and_migration_paths(void)
{
    ssz_chunk_t roots[3] = {
        make_chunk(0x10u),
        make_chunk(0x20u),
        make_chunk(0x30u),
    };
    uint64_t tokens[3] = {1u, 2u, 3u};
    composite_fixture_t fixture_ctx = {
        .roots = roots,
        .tokens = tokens,
        .count = 3u,
        .fail_index = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    const ssz_member_codec_t codec = {
        .ctx = &fixture_ctx,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_config_t exact_small_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, false, NULL);
    const ssz_merkle_cache_config_t exact_large_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);
    const ssz_merkle_cache_config_t bounded_cfg = make_cache_config(0u, 4u, 0u, 0u, true, NULL);
    cache_fixture_t small_fixture;
    cache_fixture_t large_fixture;
    cache_fixture_t no_token_fixture;
    ssz_merkle_cache_sync_composite_opts_t token_only_opts;
    ssz_merkle_cache_sync_composite_opts_t batch_opts;
    ssz_merkle_cache_sync_composite_opts_t bad_batch_opts;
    ssz_merkle_cache_sync_workspace_t tiny_workspace;

    ASSERT_ERR(cache_fixture_init(&small_fixture, &exact_small_cfg, true), SSZ_SUCCESS);
    token_only_opts = make_composite_opts(&fixture_ctx, composite_token, NULL, NULL);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &small_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &codec,
            &token_only_opts),
        SSZ_SUCCESS);

    fixture_ctx.root_calls = 0u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &small_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &codec,
            &token_only_opts),
        SSZ_SUCCESS);
    ASSERT_SIZE_EQ(fixture_ctx.root_calls, 0u);

    fixture_ctx.fail_index = 1u;
    tokens[1] = 7u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &small_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &codec,
            &token_only_opts),
        SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(&small_fixture.cache));
    fixture_ctx.fail_index = UINT64_MAX;
    tokens[1] = 2u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &small_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &codec,
            &token_only_opts),
        SSZ_SUCCESS);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&small_fixture.cache));

    ASSERT_ERR(cache_fixture_init(&no_token_fixture, &bounded_cfg, false), SSZ_SUCCESS);
    batch_opts = make_composite_opts(
        &fixture_ctx,
        composite_token,
        composite_root_batch,
        &no_token_fixture.workspace);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&no_token_fixture.cache, 3u, 4u, &codec, &batch_opts),
        SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(cache_fixture_init(&large_fixture, &bounded_cfg, true), SSZ_SUCCESS);
    batch_opts = make_composite_opts(
        &fixture_ctx,
        composite_token,
        composite_root_batch,
        &large_fixture.workspace);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&large_fixture.cache, 3u, 4u, &codec, &batch_opts),
        SSZ_SUCCESS);

    tiny_workspace.struct_size = sizeof(tiny_workspace);
    tiny_workspace.root_batch_roots = large_fixture.root_batch_roots;
    tiny_workspace.root_batch_roots_count = 1u;
    bad_batch_opts =
        make_composite_opts(&fixture_ctx, composite_token, composite_root_batch, &tiny_workspace);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&large_fixture.cache, 2u, 4u, &codec, &bad_batch_opts),
        SSZ_ERR_BUFFER_TOO_SMALL);

    cache_fixture_cleanup(&large_fixture);
    ASSERT_ERR(cache_fixture_init(&large_fixture, &exact_large_cfg, true), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkle_cache_migrate_into(
            &small_fixture.cache,
            &exact_large_cfg,
            &large_fixture.storage,
            &large_fixture.cache),
        SSZ_SUCCESS);
    fixture_ctx.root_calls = 0u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &large_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &codec,
            &token_only_opts),
        SSZ_SUCCESS);
    ASSERT_SIZE_EQ(fixture_ctx.root_calls, 0u);
    ASSERT_ERR(
        ssz_merkle_cache_migrate_into(
            &large_fixture.cache,
            &exact_small_cfg,
            &small_fixture.storage,
            &small_fixture.cache),
        SSZ_ERR_BUFFER_TOO_SMALL);

    cache_fixture_cleanup(&small_fixture);
    cache_fixture_cleanup(&large_fixture);
    cache_fixture_cleanup(&no_token_fixture);
    return true;
}

static bool test_internal_helpers_and_requirements_edge_cases(void)
{
    size_t values[2] = {1u, 0u};
    ssz_merkle_cache_requirements_t requirements;
    const ssz_merkle_cache_config_t single_leaf_cfg = make_cache_config(0u, 1u, 0u, 0u, true, NULL);
    const ssz_merkle_cache_config_t overflow_cfg = make_cache_config(
        0u,
        (UINT64_C(1) << 63) + 1u,
        0u,
        0u,
        true,
        NULL);

    ssz_merkle_cache_internal_sort_size_t_asc(values, 2u);
    ASSERT_SIZE_EQ(values[0], 0u);
    ASSERT_SIZE_EQ(values[1], 1u);

    ASSERT_ERR(ssz_merkle_cache_requirements(&single_leaf_cfg, &requirements), SSZ_SUCCESS);
    ASSERT_U64_EQ(requirements.physical_leaf_capacity, 1u);
    ASSERT_U64_EQ(requirements.mutable_leaf_capacity, 1u);
    ASSERT_U64_EQ(requirements.depth, 0u);
    ASSERT_SIZE_EQ(requirements.parent_dirty_words, 0u);
    ASSERT_SIZE_EQ(requirements.gather_pair_capacity, 0u);
    ASSERT_SIZE_EQ(requirements.gather_pairs_count, 0u);
    ASSERT_SIZE_EQ(requirements.gather_parent_indices_count, 0u);

    ASSERT_ERR(ssz_merkle_cache_requirements(&overflow_cfg, &requirements), SSZ_ERR_OVERFLOW);
    return true;
}

static bool test_sync_packed_vector_fixed_comprehensive(void)
{
    cache_fixture_t exact_fixture;
    cache_fixture_t bounded_fixture;
    cache_fixture_t mixed_fixture;
    ssz_merkle_cache_t unbound_cache;
    ssz_chunk_t expected_first;
    ssz_chunk_t expected_second;
    const ssz_merkle_cache_config_t exact_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 4u, 77u, false, NULL);
    const ssz_merkle_cache_config_t bounded_cfg = make_cache_config(0u, 4u, 0u, 0u, false, NULL);
    const ssz_merkle_cache_config_t mixed_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, true, NULL);
    uint8_t elements[40] = {0};
    size_t leaf_base = 0u;

    (void)memset(&exact_fixture, 0, sizeof(exact_fixture));
    (void)memset(&bounded_fixture, 0, sizeof(bounded_fixture));
    (void)memset(&mixed_fixture, 0, sizeof(mixed_fixture));
    (void)memset(&unbound_cache, 0, sizeof(unbound_cache));

    for (size_t i = 0u; i < sizeof(elements); i++)
    {
        elements[i] = (uint8_t)(0x40u + i);
    }

    ASSERT_ERR(cache_fixture_init(&exact_fixture, &exact_cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(cache_fixture_init(&bounded_fixture, &bounded_cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(cache_fixture_init(&mixed_fixture, &mixed_cfg, false), SSZ_SUCCESS);

    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&unbound_cache, elements, 1u, 1u),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&exact_fixture.cache, elements, 0u, 1u),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&exact_fixture.cache, elements, 1u, 0u),
        SSZ_ERR_SCHEMA_INVALID);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&exact_fixture.cache, elements, 1u, 1u),
        SSZ_ERR_OVERFLOW);
    ASSERT_SIZE_EQ(g_hooks.u64_to_size_calls, 1u);

    reset_hooks();
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(
            &exact_fixture.cache,
            elements,
            (uint64_t)SIZE_MAX,
            2u),
        SSZ_ERR_OVERFLOW);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&exact_fixture.cache, NULL, 1u, 1u),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&bounded_fixture.cache, elements, 1u, 1u),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&mixed_fixture.cache, elements, 1u, 1u),
        SSZ_ERR_INVALID_ARGUMENT);

    exact_fixture.cache.needs_resync = true;
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_vector_fixed(&exact_fixture.cache, elements, 2u, 20u),
        SSZ_SUCCESS);
    ASSERT_U64_EQ(exact_fixture.cache.leaf_count, 2u);
    ASSERT_U64_EQ(exact_fixture.cache.logical_length, 77u);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&exact_fixture.cache));

    expected_first = zero_chunk();
    expected_second = zero_chunk();
    (void)memcpy(expected_first.bytes, elements, SSZ_BYTES_PER_CHUNK);
    (void)memcpy(
        expected_second.bytes,
        &elements[SSZ_BYTES_PER_CHUNK],
        sizeof(elements) - SSZ_BYTES_PER_CHUNK);
    leaf_base = (size_t)exact_fixture.cache.level_offsets[0];
    ASSERT_CHUNK_EQ(exact_fixture.cache.nodes[leaf_base], expected_first);
    ASSERT_CHUNK_EQ(exact_fixture.cache.nodes[leaf_base + 1u], expected_second);

    cache_fixture_cleanup(&exact_fixture);
    cache_fixture_cleanup(&bounded_fixture);
    cache_fixture_cleanup(&mixed_fixture);
    return true;
}

static bool test_composite_sync_edge_cases(void)
{
    ssz_chunk_t fallback_roots[3] = {
        make_chunk(0x10u),
        make_chunk(0x20u),
        make_chunk(0x30u),
    };
    ssz_chunk_t tail_roots[3] = {
        make_chunk(0x41u),
        make_chunk(0x42u),
        make_chunk(0x43u),
    };
    ssz_chunk_t shrink_roots[3] = {
        make_chunk(0x51u),
        make_chunk(0x52u),
        make_chunk(0x53u),
    };
    uint64_t tail_tokens[3] = {11u, 22u, 33u};
    uint64_t shrink_tokens[3] = {101u, 202u, 303u};
    composite_fixture_t fallback_ctx = {
        .roots = fallback_roots,
        .tokens = NULL,
        .count = 3u,
        .fail_index = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    composite_root_fail_fixture_t tail_ctx = {
        .composite =
            {
                .roots = tail_roots,
                .tokens = tail_tokens,
                .count = 3u,
                .fail_index = UINT64_MAX,
                .fail_err = SSZ_ERR_HASH_FAILURE,
                .root_calls = 0u,
                .token_calls = 0u,
                .batch_calls = 0u,
            },
        .root_fail_index = UINT64_MAX,
        .root_fail_err = SSZ_ERR_HASH_FAILURE,
    };
    composite_fixture_t shrink_ctx = {
        .roots = shrink_roots,
        .tokens = shrink_tokens,
        .count = 3u,
        .fail_index = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    const ssz_member_codec_t fallback_codec = {
        .ctx = &fallback_ctx,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_member_codec_t tail_codec = {
        .ctx = &tail_ctx,
        .write = NULL,
        .read = NULL,
        .root = composite_root_fail_only,
    };
    const ssz_member_codec_t shrink_codec = {
        .ctx = &shrink_ctx,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_config_t bounded_cfg = make_cache_config(0u, 4u, 0u, 0u, true, NULL);
    const ssz_merkle_cache_config_t exact_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, false, NULL);
    cache_fixture_t fallback_fixture;
    cache_fixture_t tail_fixture;
    cache_fixture_t shrink_fixture;
    ssz_merkle_cache_sync_composite_opts_t fallback_opts;
    ssz_merkle_cache_sync_composite_opts_t tail_opts;
    ssz_merkle_cache_sync_composite_opts_t shrink_opts;

    (void)memset(&fallback_fixture, 0, sizeof(fallback_fixture));
    (void)memset(&tail_fixture, 0, sizeof(tail_fixture));
    (void)memset(&shrink_fixture, 0, sizeof(shrink_fixture));

    ASSERT_ERR(cache_fixture_init(&fallback_fixture, &bounded_cfg, false), SSZ_SUCCESS);
    fallback_opts = make_composite_opts(&fallback_ctx, NULL, composite_root_batch, &fallback_fixture.workspace);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&fallback_fixture.cache, 3u, 4u, &fallback_codec, &fallback_opts),
        SSZ_SUCCESS);
    ASSERT_U64_EQ(fallback_fixture.cache.leaf_count, 3u);
    ASSERT_U64_EQ(fallback_fixture.cache.logical_length, 3u);
    ASSERT_U64_EQ(fallback_ctx.batch_calls, 1u);
    ASSERT_U64_EQ(fallback_ctx.root_calls, 0u);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&fallback_fixture.cache));

    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&fallback_fixture.cache, 2u, 4u, &fallback_codec, &fallback_opts),
        SSZ_SUCCESS);
    ASSERT_U64_EQ(fallback_fixture.cache.leaf_count, 2u);
    ASSERT_U64_EQ(fallback_fixture.cache.logical_length, 2u);
    ASSERT_U64_EQ(fallback_ctx.batch_calls, 2u);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&fallback_fixture.cache));

    ASSERT_ERR(cache_fixture_init(&tail_fixture, &exact_cfg, true), SSZ_SUCCESS);
    tail_opts = make_composite_opts(&tail_ctx, composite_token_fail_only, NULL, NULL);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &tail_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &tail_codec,
            &tail_opts),
        SSZ_SUCCESS);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&tail_fixture.cache));

    tail_ctx.composite.root_calls = 0u;
    tail_ctx.composite.token_calls = 0u;
    tail_roots[2] = make_chunk(0x7au);
    tail_tokens[2] = 99u;
    tail_ctx.root_fail_index = 2u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &tail_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &tail_codec,
            &tail_opts),
        SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(&tail_fixture.cache));
    ASSERT_U64_EQ(tail_ctx.composite.root_calls, 1u);
    ASSERT_U64_EQ(tail_ctx.composite.token_calls, 3u);

    ASSERT_ERR(cache_fixture_init(&shrink_fixture, &exact_cfg, true), SSZ_SUCCESS);
    shrink_opts = make_composite_opts(&shrink_ctx, composite_token, NULL, NULL);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &shrink_fixture.cache,
            3u,
            SSZ_NO_LIMIT,
            &shrink_codec,
            &shrink_opts),
        SSZ_SUCCESS);
    ASSERT_FALSE(ssz_merkle_cache_needs_resync(&shrink_fixture.cache));

    reset_hooks();
    g_hooks.add_overflow_u64_fail_at = 1u;
    shrink_ctx.root_calls = 0u;
    shrink_ctx.token_calls = 0u;
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(
            &shrink_fixture.cache,
            2u,
            SSZ_NO_LIMIT,
            &shrink_codec,
            &shrink_opts),
        SSZ_ERR_OVERFLOW);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(&shrink_fixture.cache));
    ASSERT_U64_EQ(shrink_ctx.root_calls, 0u);
    ASSERT_U64_EQ(shrink_ctx.token_calls, 2u);

    cache_fixture_cleanup(&fallback_fixture);
    cache_fixture_cleanup(&tail_fixture);
    cache_fixture_cleanup(&shrink_fixture);
    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"requirements_and_bind_paths", test_requirements_and_bind_paths},
        {"dirty_and_token_helpers", test_dirty_and_token_helpers},
        {"hash_dirty_parent_paths", test_hash_dirty_parent_paths},
        {"recompute_and_public_error_paths", test_recompute_and_public_error_paths},
        {"composite_and_migration_paths", test_composite_and_migration_paths},
        {"internal_helpers_and_requirements_edge_cases", test_internal_helpers_and_requirements_edge_cases},
        {"sync_packed_vector_fixed_comprehensive", test_sync_packed_vector_fixed_comprehensive},
        {"composite_sync_edge_cases", test_composite_sync_edge_cases},
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

    printf(
        "[OK] %zu/%zu merkle_cache_internal tests passed\n",
        sizeof(tests) / sizeof(tests[0]),
        sizeof(tests) / sizeof(tests[0]));
    return 0;
}
