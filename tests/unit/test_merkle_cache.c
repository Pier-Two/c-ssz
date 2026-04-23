#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssz.h"

static ssz_chunk_t g_test_merkle_cache_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_test_merkle_cache_scratch = {
    .chunks = g_test_merkle_cache_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

#define ssz_hash_tree_root_bitvector(bits_le, bits_le_len, bit_count, hash_fn, out_root) \
    ssz_hash_tree_root_bitvector(                                                        \
        (bits_le),                                                                       \
        (bits_le_len),                                                                   \
        (bit_count),                                                                     \
        &g_test_merkle_cache_scratch,                                                    \
        (hash_fn),                                                                       \
        (out_root))
#define ssz_hash_tree_root_bitlist(bits_le, bits_le_len, bit_len, bit_limit, hash_fn, out_root) \
    ssz_hash_tree_root_bitlist(                                                                 \
        (bits_le),                                                                              \
        (bits_le_len),                                                                          \
        (bit_len),                                                                              \
        (bit_limit),                                                                            \
        &g_test_merkle_cache_scratch,                                                           \
        (hash_fn),                                                                              \
        (out_root))
#define ssz_hash_tree_root_vector_fixed(elements, element_count, element_size, hash_fn, out_root) \
    ssz_hash_tree_root_vector_fixed(                                                              \
        (elements),                                                                               \
        (element_count),                                                                          \
        (element_size),                                                                           \
        &g_test_merkle_cache_scratch,                                                             \
        (hash_fn),                                                                                \
        (out_root))
#define ssz_hash_tree_root_list_fixed( \
    elements,                          \
    element_count,                     \
    element_limit,                     \
    element_size,                      \
    hash_fn,                           \
    out_root)                          \
    ssz_hash_tree_root_list_fixed(     \
        (elements),                    \
        (element_count),               \
        (element_limit),               \
        (element_size),                \
        &g_test_merkle_cache_scratch,  \
        (hash_fn),                     \
        (out_root))
#define ssz_hash_tree_root_list_roots(roots, count, limit, hash_fn, out_root) \
    ssz_hash_tree_root_list_roots(                                            \
        (roots),                                                              \
        (count),                                                              \
        (limit),                                                              \
        &g_test_merkle_cache_scratch,                                         \
        (hash_fn),                                                            \
        (out_root))
#define ssz_merkleize(chunks, chunk_count, limit, hash_fn, out_root) \
    ssz_merkleize(                                                   \
        (chunks),                                                    \
        (chunk_count),                                               \
        (limit),                                                     \
        &g_test_merkle_cache_scratch,                                \
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

#define ASSERT_CHUNK_EQ(actual, expected)                                                       \
    do                                                                                          \
    {                                                                                           \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) != 0)                 \
        {                                                                                       \
            fprintf(stderr, "Assertion failed at %s:%d: chunk mismatch\n", __FILE__, __LINE__); \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

#define ASSERT_CHUNK_NE(actual, expected)                                       \
    do                                                                          \
    {                                                                           \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) == 0) \
        {                                                                       \
            fprintf(                                                            \
                stderr,                                                         \
                "Assertion failed at %s:%d: chunks unexpectedly equal\n",       \
                __FILE__,                                                       \
                __LINE__);                                                      \
            return false;                                                       \
        }                                                                       \
    } while (0)

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

static ssz_chunk_t *misaligned_chunk_ptr(void *storage)
{
    uintptr_t base = (uintptr_t)storage;
    uintptr_t aligned = (base + (uintptr_t)(SSZ_CHUNK_ALIGNMENT - 1u)) &
                        ~((uintptr_t)SSZ_CHUNK_ALIGNMENT - 1u);
    return (ssz_chunk_t *)(void *)(aligned + 1u);
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
        fixture->gather_pairs = alloc_zeroed(
            fixture->requirements.gather_pairs_count,
            sizeof(*fixture->gather_pairs));
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

typedef struct
{
    size_t hash_calls;
} counting_hash_ctx_t;

static ssz_error_t counting_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    counting_hash_ctx_t *counter = (counting_hash_ctx_t *)ctx;
    if (counter != NULL)
    {
        counter->hash_calls++;
    }
    else
    {
        /* intentionally empty */
    }
    return ssz_hash_sha256(data, data_len, out);
}

typedef struct
{
    const ssz_chunk_t *roots;
    const uint64_t *tokens;
    uint64_t count;
    uint64_t fail_member;
    ssz_error_t fail_err;
    uint64_t root_calls;
    uint64_t token_calls;
    uint64_t batch_calls;
} composite_ctx_t;

static ssz_error_t composite_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    composite_ctx_t *composite = (composite_ctx_t *)ctx;
    if ((composite == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    composite->root_calls++;
    if (member_id == composite->fail_member)
    {
        return composite->fail_err;
    }
    if (member_id >= composite->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_root = composite->roots[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t composite_root_batch(
    const void *ctx,
    uint64_t start_index,
    uint64_t count,
    ssz_chunk_t *out_roots)
{
    composite_ctx_t *composite = (composite_ctx_t *)ctx;
    if ((composite == NULL) || (out_roots == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((start_index + count) > composite->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((composite->fail_member >= start_index) && (composite->fail_member < (start_index + count)))
    {
        return composite->fail_err;
    }
    composite->batch_calls++;
    for (uint64_t i = 0u; i < count; i++)
    {
        out_roots[i] = composite->roots[start_index + i];
    }
    return SSZ_SUCCESS;
}

static ssz_error_t composite_token(const void *ctx, uint64_t member_id, uint64_t *out_token)
{
    composite_ctx_t *composite = (composite_ctx_t *)ctx;
    if ((composite == NULL) || (out_token == NULL) || (composite->tokens == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    composite->token_calls++;
    if (member_id == composite->fail_member)
    {
        return composite->fail_err;
    }
    if (member_id >= composite->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_token = composite->tokens[member_id];
    return SSZ_SUCCESS;
}

static bool test_bind_reset_lifecycle(void)
{
    cache_fixture_t unbounded;
    cache_fixture_t bounded;
    ssz_chunk_t got_data;
    ssz_chunk_t got_root;
    ssz_chunk_t expected_data;
    ssz_chunk_t expected_root;
    const ssz_merkle_cache_config_t cfg_unbounded =
        make_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);
    const ssz_merkle_cache_config_t cfg_bounded = make_cache_config(0u, 8u, 0u, 0u, true, NULL);

    ASSERT_ERR(cache_fixture_init(&unbounded, &cfg_unbounded, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&unbounded.cache, &got_data), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_data, zero_chunk());
    ASSERT_ERR(ssz_merkle_cache_root(&unbounded.cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, zero_chunk());
    cache_fixture_cleanup(&unbounded);

    ASSERT_ERR(cache_fixture_init(&bounded, &cfg_bounded, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(NULL, 0u, 8u, NULL, &expected_data), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&expected_data, 0u, NULL, &expected_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&bounded.cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, expected_root);

    {
        const ssz_chunk_t leaf = make_chunk(0x22u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(&bounded.cache, 0u, &leaf, 1u), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_set_logical_length(&bounded.cache, 1u), SSZ_SUCCESS);
    }
    ASSERT_ERR(ssz_merkle_cache_reset(&bounded.cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(NULL, 0u, 8u, NULL, &expected_data), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&expected_data, 0u, NULL, &expected_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&bounded.cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, expected_root);

    cache_fixture_cleanup(&bounded);
    return true;
}

static bool test_single_leaf_update_and_root_query(void)
{
    cache_fixture_t fixture;
    ssz_chunk_t root;
    const ssz_chunk_t leaf = make_chunk(0x10u);
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 0u, &leaf, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, leaf);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, leaf);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_multiple_scattered_leaf_updates_and_root_correctness(void)
{
    cache_fixture_t fixture;
    ssz_chunk_t leaves[8];
    ssz_chunk_t root;
    ssz_chunk_t expected;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 8u, 0u, 0u, false, NULL);

    for (size_t i = 0u; i < 8u; i++)
    {
        leaves[i] = zero_chunk();
    }
    leaves[1] = make_chunk(0x21u);
    leaves[5] = make_chunk(0x45u);
    leaves[7] = make_chunk(0x67u);

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 1u, &leaves[1], 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 5u, &leaves[5], 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 7u, &leaves[7], 1u), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves, 8u, 8u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_unchanged_query_returns_o1_cached_result(void)
{
    counting_hash_ctx_t counter = {0u};
    const ssz_hash_fn_t hash_fn = {
        .hash = counting_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &counter,
    };
    const ssz_merkle_cache_config_t cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 8u, 2u, true, &hash_fn);
    cache_fixture_t fixture;
    ssz_chunk_t roots[2] = {make_chunk(0x11u), make_chunk(0x33u)};
    ssz_chunk_t out;

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 0u, roots, 2u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(&fixture.cache, 2u), SSZ_SUCCESS);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls > 0u);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 0u);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 0u);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_sync_packed_bytes_correctness(void)
{
    cache_fixture_t fixture;
    uint8_t bytes[57];
    ssz_chunk_t root;
    ssz_chunk_t expected;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, false, NULL);

    for (size_t i = 0u; i < sizeof(bytes); i++)
    {
        bytes[i] = (uint8_t)(0x80u + (uint8_t)i);
    }

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkle_cache_sync_packed_bytes(&fixture.cache, bytes, sizeof(bytes), 0u),
        SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(bytes, sizeof(bytes), 1u, NULL, &expected),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(&fixture.cache, bytes, 10u, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_vector_fixed(bytes, 10u, 1u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_bitvector_and_bitlist_tail_validation(void)
{
    {
        cache_fixture_t fixture;
        const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 1u, 0u, 0u, false, NULL);
        const uint8_t valid_bits[2] = {0x55u, 0x01u};
        const uint8_t invalid_bits[2] = {0x55u, 0xC1u};
        ssz_chunk_t got;
        ssz_chunk_t expected;

        ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
        ASSERT_ERR(
            ssz_merkle_cache_sync_bitvector(&fixture.cache, valid_bits, sizeof(valid_bits), 10u),
            SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
        ASSERT_ERR(
            ssz_hash_tree_root_bitvector(valid_bits, sizeof(valid_bits), 10u, NULL, &expected),
            SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(got, expected);

        ASSERT_ERR(
            ssz_merkle_cache_sync_bitvector(
                &fixture.cache,
                invalid_bits,
                sizeof(invalid_bits),
                10u),
            SSZ_ERR_ENCODING_INVALID);
        cache_fixture_cleanup(&fixture);
    }

    {
        cache_fixture_t fixture;
        const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 1u, 0u, 0u, true, NULL);
        const uint8_t valid_bits[2] = {0x55u, 0x01u};
        const uint8_t invalid_bits[2] = {0x55u, 0x02u};
        ssz_chunk_t got;
        ssz_chunk_t expected;

        ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
        ASSERT_ERR(
            ssz_merkle_cache_sync_bitlist(&fixture.cache, valid_bits, sizeof(valid_bits), 10u, 10u),
            SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
        ASSERT_ERR(
            ssz_hash_tree_root_bitlist(valid_bits, sizeof(valid_bits), 10u, 10u, NULL, &expected),
            SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(got, expected);

        ASSERT_ERR(
            ssz_merkle_cache_sync_bitlist(
                &fixture.cache,
                invalid_bits,
                sizeof(invalid_bits),
                9u,
                10u),
            SSZ_ERR_ENCODING_INVALID);
        cache_fixture_cleanup(&fixture);
    }

    return true;
}

static bool test_composite_sync_safe_fallback(void)
{
    cache_fixture_t fixture;
    ssz_chunk_t roots[4] = {
        make_chunk(0x11u),
        make_chunk(0x22u),
        make_chunk(0x33u),
        make_chunk(0x44u),
    };
    composite_ctx_t composite = {
        .roots = roots,
        .tokens = NULL,
        .count = 4u,
        .fail_member = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    const ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 4u, 0u, 0u, true, NULL);
    ssz_chunk_t got;
    ssz_chunk_t expected;

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, NULL), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    composite.fail_member = 1u;
    roots[0] = make_chunk(0xA1u);
    roots[1] = make_chunk(0xB2u);
    roots[2] = make_chunk(0xC3u);
    ASSERT_ERR(
        ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, NULL),
        SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(&fixture.cache));

    composite.fail_member = UINT64_MAX;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, NULL), SSZ_SUCCESS);
    ASSERT_TRUE(!ssz_merkle_cache_needs_resync(&fixture.cache));
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_composite_token_batch_path(void)
{
    cache_fixture_t fixture;
    ssz_chunk_t roots[4] = {
        make_chunk(0x31u),
        make_chunk(0x32u),
        make_chunk(0x33u),
        make_chunk(0x34u),
    };
    uint64_t tokens[4] = {11u, 22u, 33u, 44u};
    composite_ctx_t composite = {
        .roots = roots,
        .tokens = tokens,
        .count = 4u,
        .fail_member = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    const ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 4u, 0u, 0u, true, NULL);
    ssz_merkle_cache_sync_composite_opts_t opts;
    ssz_chunk_t got;
    ssz_chunk_t expected;

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, true), SSZ_SUCCESS);
    opts =
        make_composite_opts(&composite, composite_token, composite_root_batch, &fixture.workspace);

    ASSERT_ERR(ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts), SSZ_SUCCESS);
    ASSERT_TRUE(composite.batch_calls == 1u);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    ASSERT_ERR(ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts), SSZ_SUCCESS);
    ASSERT_TRUE(composite.batch_calls == 1u);

    roots[1] = make_chunk(0xA2u);
    tokens[1] = 55u;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts), SSZ_SUCCESS);
    ASSERT_TRUE(composite.batch_calls == 2u);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_exact_migrate_preserves_roots_and_extends_capacity(void)
{
    counting_hash_ctx_t counter = {0u};
    const ssz_hash_fn_t hash_fn = {
        .hash = counting_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &counter,
    };
    const ssz_merkle_cache_config_t small_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, false, &hash_fn);
    const ssz_merkle_cache_config_t large_cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, &hash_fn);
    cache_fixture_t small_fixture;
    cache_fixture_t large_fixture;
    ssz_chunk_t leaves[5];
    ssz_chunk_t first_root;
    ssz_chunk_t migrated_root;
    ssz_chunk_t expected;

    leaves[0] = make_chunk(0x10u);
    leaves[1] = make_chunk(0x20u);
    leaves[2] = make_chunk(0x30u);
    leaves[3] = make_chunk(0x40u);
    leaves[4] = make_chunk(0x50u);

    ASSERT_ERR(cache_fixture_init(&small_fixture, &small_cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkle_cache_update_root_range(&small_fixture.cache, 0u, leaves, 4u),
        SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&small_fixture.cache, &first_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves, 4u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(first_root, expected);
    ASSERT_ERR(
        ssz_merkle_cache_update_root_range(&small_fixture.cache, 4u, &leaves[4], 1u),
        SSZ_ERR_LIMIT_EXCEEDED);

    ASSERT_ERR(cache_fixture_init(&large_fixture, &large_cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkle_cache_migrate_into(
            &small_fixture.cache,
            &large_cfg,
            &large_fixture.storage,
            &large_fixture.cache),
        SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&large_fixture.cache, &migrated_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(migrated_root, first_root);

    ASSERT_ERR(
        ssz_merkle_cache_update_root_range(&large_fixture.cache, 4u, &leaves[4], 1u),
        SSZ_SUCCESS);
    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_data_root(&large_fixture.cache, &migrated_root), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 3u);
    ASSERT_ERR(ssz_merkleize(leaves, 5u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(migrated_root, expected);

    cache_fixture_cleanup(&small_fixture);
    cache_fixture_cleanup(&large_fixture);
    return true;
}

static bool test_cached_vs_stateless_equivalence(void)
{
#define ELEMENT_LIMIT 40u
    const size_t element_size = 1u;
    const uint64_t counts[] = {0u, 1u, 15u, 32u, 33u, ELEMENT_LIMIT};
    uint8_t bytes[ELEMENT_LIMIT];
    cache_fixture_t fixture;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 2u, 0u, 0u, true, NULL);

    for (size_t i = 0u; i < sizeof(bytes); i++)
    {
        bytes[i] = (uint8_t)(i ^ 0x5Au);
    }

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    for (size_t i = 0u; i < (sizeof(counts) / sizeof(counts[0])); i++)
    {
        uint64_t count = counts[i];
        ssz_chunk_t cached_root;
        ssz_chunk_t expected_root;

        ASSERT_ERR(
            ssz_merkle_cache_sync_packed_list_fixed(
                &fixture.cache,
                bytes,
                count,
                ELEMENT_LIMIT,
                element_size),
            SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &cached_root), SSZ_SUCCESS);
        ASSERT_ERR(
            ssz_hash_tree_root_list_fixed(
                bytes,
                count,
                ELEMENT_LIMIT,
                element_size,
                NULL,
                &expected_root),
            SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(cached_root, expected_root);
    }

    cache_fixture_cleanup(&fixture);
#undef ELEMENT_LIMIT
    return true;
}

static bool test_zero_range_and_logical_length_changes(void)
{
    cache_fixture_t fixture;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, true, NULL);
    ssz_chunk_t leaves[2] = {make_chunk(0xABu), make_chunk(0xCDu)};
    ssz_chunk_t data_root;
    ssz_chunk_t root_len2;
    ssz_chunk_t root_len9;
    ssz_chunk_t expected;

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 0u, leaves, 2u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(&fixture.cache, 2u), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &data_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves, 2u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, expected);

    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &root_len2), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&expected, 2u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len2, expected);

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(&fixture.cache, 9u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &root_len9), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&data_root, 9u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len9, expected);
    ASSERT_CHUNK_NE(root_len2, root_len9);

    ASSERT_ERR(ssz_merkle_cache_zero_range(&fixture.cache, 1u, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &data_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, leaves[0]);

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(&fixture.cache, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &root_len2), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&leaves[0], 1u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len2, expected);

    ASSERT_ERR(ssz_merkle_cache_zero_range(&fixture.cache, 0u, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &data_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, zero_chunk());

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(&fixture.cache, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, &root_len2), SSZ_SUCCESS);
    {
        ssz_chunk_t zero = zero_chunk();
        ASSERT_ERR(ssz_mix_in_length_u64(&zero, 0u, NULL, &expected), SSZ_SUCCESS);
    }
    ASSERT_CHUNK_EQ(root_len2, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_multi_word_dirty_triggers_qsort(void)
{
    cache_fixture_t fixture;
    const ssz_merkle_cache_config_t cfg =
        make_cache_config(0u, SSZ_NO_LIMIT, 256u, 0u, false, NULL);
    ssz_chunk_t leaves[256];
    ssz_chunk_t root1;
    ssz_chunk_t root2;
    ssz_chunk_t expected;

    for (size_t i = 0u; i < 256u; i++)
    {
        leaves[i] = make_chunk((uint8_t)(i & 0xFFu));
    }

    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 0u, leaves, 256u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root1), SSZ_SUCCESS);

    {
        ssz_chunk_t v = make_chunk(0xF1u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 1u, &v, 1u), SSZ_SUCCESS);
        leaves[1] = v;

        v = make_chunk(0xF2u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 65u, &v, 1u), SSZ_SUCCESS);
        leaves[65] = v;

        v = make_chunk(0xF3u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 129u, &v, 1u), SSZ_SUCCESS);
        leaves[129] = v;

        v = make_chunk(0xF4u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(&fixture.cache, 193u, &v, 1u), SSZ_SUCCESS);
        leaves[193] = v;
    }

    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, &root2), SSZ_SUCCESS);
    ASSERT_CHUNK_NE(root1, root2);

    ASSERT_ERR(ssz_merkleize(leaves, 256u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root2, expected);

    cache_fixture_cleanup(&fixture);
    return true;
}

static bool test_alignment_rejection_paths(void)
{
    cache_fixture_t fixture;
    const ssz_merkle_cache_config_t cfg = make_cache_config(0u, 4u, 0u, 0u, false, NULL);
    const ssz_chunk_t leaf = make_chunk(0x5Au);
    composite_ctx_t composite = {
        .roots = &leaf,
        .tokens = NULL,
        .count = 1u,
        .fail_member = UINT64_MAX,
        .fail_err = SSZ_SUCCESS,
        .root_calls = 0u,
        .token_calls = 0u,
        .batch_calls = 0u,
    };
    const ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    ssz_merkle_cache_storage_t storage;
    ssz_merkle_cache_t cache;
    ssz_merkle_cache_sync_composite_opts_t opts;
    uint8_t out_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t roots_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t *nodes_raw = NULL;
    uint8_t *gather_pairs_raw = NULL;
    uint8_t *gather_hashes_raw = NULL;
    uint8_t *workspace_raw = NULL;

    ASSERT_TRUE(SSZ_CHUNK_ALIGNMENT > 1u);
    ASSERT_ERR(cache_fixture_init(&fixture, &cfg, false), SSZ_SUCCESS);

    nodes_raw = (uint8_t *)alloc_zeroed(
        (fixture.requirements.nodes_count * sizeof(ssz_chunk_t)) + SSZ_CHUNK_ALIGNMENT,
        1u);
    ASSERT_TRUE(nodes_raw != NULL);
    storage = fixture.storage;
    storage.nodes = misaligned_chunk_ptr(nodes_raw);
    ASSERT_ERR(ssz_merkle_cache_bind(&cfg, &storage, &cache), SSZ_ERR_ALIGNMENT_INVALID);

    if (fixture.requirements.gather_pairs_count != 0u)
    {
        gather_pairs_raw = (uint8_t *)alloc_zeroed(
            (fixture.requirements.gather_pairs_count * sizeof(ssz_chunk_t)) + SSZ_CHUNK_ALIGNMENT,
            1u);
        ASSERT_TRUE(gather_pairs_raw != NULL);
        storage = fixture.storage;
        storage.gather_pairs = misaligned_chunk_ptr(gather_pairs_raw);
        ASSERT_ERR(ssz_merkle_cache_bind(&cfg, &storage, &cache), SSZ_ERR_ALIGNMENT_INVALID);
    }

    if (fixture.requirements.gather_hashes_count != 0u)
    {
        gather_hashes_raw = (uint8_t *)alloc_zeroed(
            (fixture.requirements.gather_hashes_count * sizeof(ssz_chunk_t)) + SSZ_CHUNK_ALIGNMENT,
            1u);
        ASSERT_TRUE(gather_hashes_raw != NULL);
        storage = fixture.storage;
        storage.gather_hashes = misaligned_chunk_ptr(gather_hashes_raw);
        ASSERT_ERR(ssz_merkle_cache_bind(&cfg, &storage, &cache), SSZ_ERR_ALIGNMENT_INVALID);
    }

    ASSERT_ERR(ssz_merkle_cache_data_root(&fixture.cache, misaligned_chunk_ptr(out_raw)),
               SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR(ssz_merkle_cache_root(&fixture.cache, misaligned_chunk_ptr(out_raw)),
               SSZ_ERR_ALIGNMENT_INVALID);

    (void)memcpy((void *)misaligned_chunk_ptr(roots_raw), &leaf, sizeof(leaf));
    ASSERT_ERR(ssz_merkle_cache_update_root_range(
                   &fixture.cache,
                   0u,
                   (const ssz_chunk_t *)(const void *)misaligned_chunk_ptr(roots_raw),
                   1u),
               SSZ_ERR_ALIGNMENT_INVALID);

    if (fixture.requirements.root_batch_roots_count != 0u)
    {
        workspace_raw = (uint8_t *)alloc_zeroed(
            (fixture.requirements.root_batch_roots_count * sizeof(ssz_chunk_t)) +
                SSZ_CHUNK_ALIGNMENT,
            1u);
        ASSERT_TRUE(workspace_raw != NULL);
        fixture.workspace.root_batch_roots = misaligned_chunk_ptr(workspace_raw);
        opts = make_composite_opts(&composite, NULL, composite_root_batch, &fixture.workspace);

        ASSERT_ERR(ssz_merkle_cache_sync_composite(
                       &fixture.cache,
                       1u,
                       cfg.leaf_limit,
                       &codec,
                       &opts),
                   SSZ_ERR_ALIGNMENT_INVALID);

        fixture.workspace.root_batch_roots = fixture.root_batch_roots;
    }

    free(nodes_raw);
    free(gather_pairs_raw);
    free(gather_hashes_raw);
    free(workspace_raw);
    cache_fixture_cleanup(&fixture);
    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"bind_reset_lifecycle", test_bind_reset_lifecycle},
        {"single_leaf_update_and_root_query", test_single_leaf_update_and_root_query},
        {"multiple_scattered_leaf_updates_and_root_correctness",
         test_multiple_scattered_leaf_updates_and_root_correctness},
        {"unchanged_query_returns_o1_cached_result", test_unchanged_query_returns_o1_cached_result},
        {"sync_packed_bytes_correctness", test_sync_packed_bytes_correctness},
        {"bitvector_and_bitlist_tail_validation", test_bitvector_and_bitlist_tail_validation},
        {"composite_sync_safe_fallback", test_composite_sync_safe_fallback},
        {"composite_token_batch_path", test_composite_token_batch_path},
        {"exact_migrate_preserves_roots_and_extends_capacity",
         test_exact_migrate_preserves_roots_and_extends_capacity},
        {"cached_vs_stateless_equivalence", test_cached_vs_stateless_equivalence},
        {"zero_range_and_logical_length_changes", test_zero_range_and_logical_length_changes},
        {"multi_word_dirty_triggers_qsort", test_multi_word_dirty_triggers_qsort},
        {"alignment_rejection_paths", test_alignment_rejection_paths},
    };

    for (size_t i = 0u; i < (sizeof(tests) / sizeof(tests[0])); i++)
    {
        if (!tests[i].fn())
        {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    printf("All merkle-cache tests passed (%zu tests)\n", sizeof(tests) / sizeof(tests[0]));
    return 0;
}
