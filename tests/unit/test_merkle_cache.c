#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ssz.h"

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

#define ASSERT_CHUNK_EQ(actual, expected)                                                            \
    do                                                                                               \
    {                                                                                                \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) != 0)                    \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: chunk mismatch\n", __FILE__, __LINE__);   \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_CHUNK_NE(actual, expected)                                                            \
    do                                                                                               \
    {                                                                                                \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) == 0)                    \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: chunks unexpectedly equal\n",                       \
                    __FILE__,                                                                         \
                    __LINE__);                                                                        \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

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
    memset(chunk.bytes, 0, sizeof(chunk.bytes));
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
    return ssz_hash_sha256(data, data_len, out);
}

typedef struct
{
    const ssz_chunk_t *roots;
    uint64_t count;
    uint64_t fail_member;
    ssz_error_t fail_err;
    uint64_t root_calls;
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

static bool test_create_destroy_lifecycle(void)
{
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t got_data;
    ssz_chunk_t got_root;
    ssz_chunk_t expected_data;
    ssz_chunk_t expected_root;

    const ssz_merkle_cache_config_t cfg_unbounded = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    ASSERT_ERR(ssz_merkle_cache_create(&cfg_unbounded, &cache), SSZ_SUCCESS);
    ASSERT_TRUE(cache != NULL);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &got_data), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_data, zero_chunk());
    ASSERT_ERR(ssz_merkle_cache_root(cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, zero_chunk());
    ssz_merkle_cache_destroy(cache);

    cache = NULL;
    const ssz_merkle_cache_config_t cfg_bounded = {
        .initial_leaf_count = 0u,
        .leaf_limit = 8u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };

    ASSERT_ERR(ssz_merkle_cache_create(&cfg_bounded, &cache), SSZ_SUCCESS);
    ASSERT_TRUE(cache != NULL);

    ASSERT_ERR(ssz_merkleize(NULL, 0u, 8u, NULL, &expected_data), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length(&expected_data, 0u, NULL, &expected_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, expected_root);

    {
        const ssz_chunk_t leaf = make_chunk(0x22u);
        ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, &leaf, 1u), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 1u), SSZ_SUCCESS);
    }
    ASSERT_ERR(ssz_merkle_cache_reset(cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(NULL, 0u, 8u, NULL, &expected_data), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length(&expected_data, 0u, NULL, &expected_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &got_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got_root, expected_root);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_single_leaf_update_and_root_query(void)
{
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t root;
    const ssz_chunk_t leaf = make_chunk(0x10u);
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, &leaf, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, leaf);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, leaf);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_multiple_scattered_leaf_updates_and_root_correctness(void)
{
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t leaves[8];
    ssz_chunk_t root;
    ssz_chunk_t expected;
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 8u,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    for (size_t i = 0u; i < 8u; i++)
    {
        leaves[i] = zero_chunk();
    }
    leaves[1] = make_chunk(0x21u);
    leaves[5] = make_chunk(0x45u);
    leaves[7] = make_chunk(0x67u);

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 1u, &leaves[1], 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 5u, &leaves[5], 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 7u, &leaves[7], 1u), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves, 8u, 8u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    ssz_merkle_cache_destroy(cache);
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
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 2u,
        .mix_in_length = true,
        .hash_fn = &hash_fn,
    };
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t roots[2] = {make_chunk(0x11u), make_chunk(0x33u)};
    ssz_chunk_t out;

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, roots, 2u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 2u), SSZ_SUCCESS);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_root(cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls > 0u);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_root(cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 0u);

    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &out), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 0u);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_sync_packed_bytes_correctness(void)
{
    ssz_merkle_cache_t *cache = NULL;
    uint8_t bytes[57];
    ssz_chunk_t root;
    ssz_chunk_t expected;
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    for (size_t i = 0u; i < sizeof(bytes); i++)
    {
        bytes[i] = (uint8_t)(0x80u + (uint8_t)i);
    }

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, bytes, sizeof(bytes), 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_vector_fixed(bytes, sizeof(bytes), 1u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    ASSERT_ERR(ssz_merkle_cache_sync_packed_bytes(cache, bytes, 10u, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_vector_fixed(bytes, 10u, 1u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_bitvector_and_bitlist_tail_validation(void)
{
    {
        const ssz_merkle_cache_config_t cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        const uint8_t valid_bits[2] = {0x55u, 0x01u};
        const uint8_t invalid_bits[2] = {0x55u, 0xC1u};
        ssz_merkle_cache_t *cache = NULL;
        ssz_chunk_t got;
        ssz_chunk_t expected;

        ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, valid_bits, sizeof(valid_bits), 10u), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(cache, &got), SSZ_SUCCESS);
        ASSERT_ERR(ssz_hash_tree_root_bitvector(valid_bits, sizeof(valid_bits), 10u, NULL, &expected),
                   SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(got, expected);

        ASSERT_ERR(ssz_merkle_cache_sync_bitvector(cache, invalid_bits, sizeof(invalid_bits), 10u),
                   SSZ_ERR_ENCODING_INVALID);
        ssz_merkle_cache_destroy(cache);
    }

    {
        const ssz_merkle_cache_config_t cfg = {
            .initial_leaf_count = 0u,
            .leaf_limit = 1u,
            .logical_length = 0u,
            .mix_in_length = true,
            .hash_fn = NULL,
        };
        const uint8_t valid_bits[2] = {0x55u, 0x01u};
        const uint8_t invalid_bits[2] = {0x55u, 0x02u};
        ssz_merkle_cache_t *cache = NULL;
        ssz_chunk_t got;
        ssz_chunk_t expected;

        ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, valid_bits, sizeof(valid_bits), 10u, 10u),
                   SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(cache, &got), SSZ_SUCCESS);
        ASSERT_ERR(ssz_hash_tree_root_bitlist(valid_bits, sizeof(valid_bits), 10u, 10u, NULL, &expected),
                   SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(got, expected);

        ASSERT_ERR(ssz_merkle_cache_sync_bitlist(cache, invalid_bits, sizeof(invalid_bits), 9u, 10u),
                   SSZ_ERR_ENCODING_INVALID);
        ssz_merkle_cache_destroy(cache);
    }

    return true;
}

static bool test_composite_sync_safe_fallback(void)
{
    ssz_chunk_t roots[4] = {
        make_chunk(0x11u),
        make_chunk(0x22u),
        make_chunk(0x33u),
        make_chunk(0x44u),
    };
    composite_ctx_t composite = {
        .roots = roots,
        .count = 4u,
        .fail_member = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
        .root_calls = 0u,
    };
    const ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = composite_root,
    };
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 4u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t got;
    ssz_chunk_t expected;

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 3u, 4u, &codec, NULL), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    composite.fail_member = 1u;
    roots[0] = make_chunk(0xA1u);
    roots[1] = make_chunk(0xB2u);
    roots[2] = make_chunk(0xC3u);
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 3u, 4u, &codec, NULL), SSZ_ERR_HASH_FAILURE);
    ASSERT_TRUE(ssz_merkle_cache_needs_resync(cache));

    composite.fail_member = UINT64_MAX;
    ASSERT_ERR(ssz_merkle_cache_sync_composite(cache, 3u, 4u, &codec, NULL), SSZ_SUCCESS);
    ASSERT_TRUE(!ssz_merkle_cache_needs_resync(cache));
    ASSERT_ERR(ssz_merkle_cache_root(cache, &got), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(got, expected);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_no_limit_growth_preserves_left_subtree_work(void)
{
    counting_hash_ctx_t counter = {0u};
    const ssz_hash_fn_t hash_fn = {
        .hash = counting_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &counter,
    };
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = false,
        .hash_fn = &hash_fn,
    };
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t leaves5[5];
    ssz_chunk_t first_root;
    ssz_chunk_t grown_root;
    ssz_chunk_t expected;

    leaves5[0] = make_chunk(0x10u);
    leaves5[1] = make_chunk(0x20u);
    leaves5[2] = make_chunk(0x30u);
    leaves5[3] = make_chunk(0x40u);
    leaves5[4] = zero_chunk();

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, leaves5, 4u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &first_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves5, 4u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(first_root, expected);

    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 4u, &leaves5[4], 1u), SSZ_SUCCESS);
    counter.hash_calls = 0u;
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &grown_root), SSZ_SUCCESS);
    ASSERT_TRUE(counter.hash_calls == 3u);

    ASSERT_ERR(ssz_merkleize(leaves5, 5u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(grown_root, expected);
    ASSERT_CHUNK_NE(first_root, grown_root);

    ssz_merkle_cache_destroy(cache);
    return true;
}

static bool test_cached_vs_stateless_equivalence(void)
{
#define ELEMENT_LIMIT 40u
    const size_t element_size = 1u;
    const uint64_t counts[] = {0u, 1u, 15u, 32u, 33u, ELEMENT_LIMIT};
    uint8_t bytes[ELEMENT_LIMIT];
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = 2u,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    ssz_merkle_cache_t *cache = NULL;

    for (size_t i = 0u; i < sizeof(bytes); i++)
    {
        bytes[i] = (uint8_t)(i ^ 0x5Au);
    }

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    for (size_t i = 0u; i < (sizeof(counts) / sizeof(counts[0])); i++)
    {
        uint64_t count = counts[i];
        ssz_chunk_t cached_root;
        ssz_chunk_t expected_root;

        ASSERT_ERR(ssz_merkle_cache_sync_packed_list_fixed(
                       cache, bytes, count, ELEMENT_LIMIT, element_size),
                   SSZ_SUCCESS);
        ASSERT_ERR(ssz_merkle_cache_root(cache, &cached_root), SSZ_SUCCESS);
        ASSERT_ERR(ssz_hash_tree_root_list_fixed(
                       bytes, count, ELEMENT_LIMIT, element_size, NULL, &expected_root),
                   SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(cached_root, expected_root);
    }

    ssz_merkle_cache_destroy(cache);
#undef ELEMENT_LIMIT
    return true;
}

static bool test_zero_range_and_logical_length_changes(void)
{
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = 0u,
        .leaf_limit = SSZ_NO_LIMIT,
        .logical_length = 0u,
        .mix_in_length = true,
        .hash_fn = NULL,
    };
    ssz_merkle_cache_t *cache = NULL;
    ssz_chunk_t leaves[2] = {make_chunk(0xABu), make_chunk(0xCDu)};
    ssz_chunk_t data_root;
    ssz_chunk_t root_len2;
    ssz_chunk_t root_len9;
    ssz_chunk_t expected;

    ASSERT_ERR(ssz_merkle_cache_create(&cfg, &cache), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_update_root_range(cache, 0u, leaves, 2u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 2u), SSZ_SUCCESS);

    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &data_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(leaves, 2u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, expected);

    ASSERT_ERR(ssz_merkle_cache_root(cache, &root_len2), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length(&expected, 2u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len2, expected);

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 9u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &root_len9), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length(&data_root, 9u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len9, expected);
    ASSERT_CHUNK_NE(root_len2, root_len9);

    ASSERT_ERR(ssz_merkle_cache_zero_range(cache, 1u, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &data_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, leaves[0]);

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &root_len2), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length(&leaves[0], 1u, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_len2, expected);

    ASSERT_ERR(ssz_merkle_cache_zero_range(cache, 0u, 1u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_data_root(cache, &data_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(data_root, zero_chunk());

    ASSERT_ERR(ssz_merkle_cache_set_logical_length(cache, 0u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkle_cache_root(cache, &root_len2), SSZ_SUCCESS);
    {
        ssz_chunk_t zero = zero_chunk();
        ASSERT_ERR(ssz_mix_in_length(&zero, 0u, NULL, &expected), SSZ_SUCCESS);
    }
    ASSERT_CHUNK_EQ(root_len2, expected);

    ssz_merkle_cache_destroy(cache);
    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"create_destroy_lifecycle", test_create_destroy_lifecycle},
        {"single_leaf_update_and_root_query", test_single_leaf_update_and_root_query},
        {"multiple_scattered_leaf_updates_and_root_correctness", test_multiple_scattered_leaf_updates_and_root_correctness},
        {"unchanged_query_returns_o1_cached_result", test_unchanged_query_returns_o1_cached_result},
        {"sync_packed_bytes_correctness", test_sync_packed_bytes_correctness},
        {"bitvector_and_bitlist_tail_validation", test_bitvector_and_bitlist_tail_validation},
        {"composite_sync_safe_fallback", test_composite_sync_safe_fallback},
        {"no_limit_growth_preserves_left_subtree_work", test_no_limit_growth_preserves_left_subtree_work},
        {"cached_vs_stateless_equivalence", test_cached_vs_stateless_equivalence},
        {"zero_range_and_logical_length_changes", test_zero_range_and_logical_length_changes},
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
