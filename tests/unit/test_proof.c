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

static ssz_chunk_t make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_chunk_t *misaligned_chunk_ptr(void *storage)
{
    uintptr_t base = (uintptr_t)storage;
    uintptr_t aligned = (base + (uintptr_t)(SSZ_CHUNK_ALIGNMENT - 1u)) &
                        ~((uintptr_t)SSZ_CHUNK_ALIGNMENT - 1u);
    return (ssz_chunk_t *)(void *)(aligned + 1u);
}

static ssz_error_t build_merkle_tree_8(const ssz_chunk_t leaves[8], ssz_chunk_t tree[16])
{
    ssz_error_t err = SSZ_SUCCESS;

    for (size_t i = 0u; i < 8u; i++)
    {
        tree[8u + i] = leaves[i];
    }

    for (size_t i = 7u; (i > 0u) && (err == SSZ_SUCCESS); i--)
    {
        err = ssz_hash_2to1(NULL, &tree[i * 2u], &tree[(i * 2u) + 1u], &tree[i]);
    }

    return err;
}

typedef struct
{
    ssz_error_t hash_err;
    ssz_error_t hash_2to1_err;
} proof_hash_fail_ctx_t;

static ssz_error_t proof_mock_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    const proof_hash_fail_ctx_t *cfg = (const proof_hash_fail_ctx_t *)ctx;
    (void)data;
    (void)data_len;
    if (out != NULL)
    {
        memset(out, 0u, 32u);
    }
    if (cfg == NULL)
    {
        return SSZ_SUCCESS;
    }
    return cfg->hash_err;
}

static ssz_error_t proof_mock_hash_2to1(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    const proof_hash_fail_ctx_t *cfg = (const proof_hash_fail_ctx_t *)ctx;
    (void)left;
    (void)right;
    if (out != NULL)
    {
        memset(out->bytes, 0u, SSZ_BYTES_PER_CHUNK);
    }
    if (cfg == NULL)
    {
        return SSZ_SUCCESS;
    }
    return cfg->hash_2to1_err;
}

static bool test_next_pow_of_two_edges(void)
{
    ASSERT_TRUE(ssz_next_pow_of_two(0u) == 1u);
    ASSERT_TRUE(ssz_next_pow_of_two(1u) == 1u);
    ASSERT_TRUE(ssz_next_pow_of_two(2u) == 2u);
    ASSERT_TRUE(ssz_next_pow_of_two(3u) == 4u);
    ASSERT_TRUE(ssz_next_pow_of_two(4u) == 4u);
    ASSERT_TRUE(ssz_next_pow_of_two(256u) == 256u);
    ASSERT_TRUE(ssz_next_pow_of_two(UINT64_C(1) << 63u) == (UINT64_C(1) << 63u));
    ASSERT_TRUE(ssz_next_pow_of_two((UINT64_C(1) << 63u) + 1u) == 0u);
    return true;
}

static bool test_generalized_index_inline_helpers(void)
{
    const ssz_gindex_t idx = 13u;

    ASSERT_TRUE(ssz_generalized_index_length(idx) == 3u);
    ASSERT_TRUE(ssz_generalized_index_bit(idx, 0u));
    ASSERT_TRUE(!ssz_generalized_index_bit(idx, 1u));
    ASSERT_TRUE(ssz_generalized_index_bit(idx, 2u));
    ASSERT_TRUE(!ssz_generalized_index_bit(idx, 64u));

    ASSERT_TRUE(ssz_generalized_index_sibling(13u) == 12u);
    ASSERT_TRUE(ssz_generalized_index_child(6u, false) == 12u);
    ASSERT_TRUE(ssz_generalized_index_child(6u, true) == 13u);
    ASSERT_TRUE(ssz_generalized_index_child(UINT64_C(1) << 63u, false) == 0u);
    ASSERT_TRUE(ssz_generalized_index_parent(13u) == 6u);

    return true;
}

static bool test_get_generalized_index_paths(void)
{
    static const ssz_gindex_type_t leaf = {
        .kind = SSZ_GINDEX_LEAF,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };

    static const ssz_gindex_type_t vector32 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 4u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };

    static const ssz_gindex_type_t list32 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 4u,
        .item_length = 32u,
        .has_mix_in_length = true,
        .elem_type = &leaf,
        .field_types = NULL,
    };

    static const ssz_gindex_type_t packed8 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 2u,
        .item_length = 8u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 8u,
    };

    static const ssz_gindex_type_t *const field_types[3] = {
        &leaf,
        &vector32,
        &list32,
    };

    static const ssz_gindex_type_t container = {
        .kind = SSZ_GINDEX_CONTAINER,
        .chunk_count = 3u,
        .item_length = 0u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = field_types,
    };

    ssz_gindex_t out = 0u;

    ASSERT_ERR(ssz_get_generalized_index(&container, (const ssz_path_step_t[]){1u, 2u}, 2u, &out),
               SSZ_SUCCESS);
    ASSERT_TRUE(out == 22u);

    ASSERT_ERR(ssz_get_generalized_index(
                   &container, (const ssz_path_step_t[]){2u, SSZ_PATH_STEP_LENGTH}, 2u, &out),
               SSZ_SUCCESS);
    ASSERT_TRUE(out == 13u);

    ASSERT_ERR(ssz_get_generalized_index(&packed8, (const ssz_path_step_t[]){5u}, 1u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 3u);

    ASSERT_ERR(ssz_get_generalized_index(&leaf, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    ASSERT_ERR(ssz_get_generalized_index(&vector32,
                                         (const ssz_path_step_t[]){SSZ_PATH_STEP_LENGTH},
                                         1u,
                                         &out),
               SSZ_ERR_GINDEX_INVALID);

    return true;
}

static bool test_get_branch_and_path_indices(void)
{
    ssz_gindex_t branch[4] = {0u};
    ssz_gindex_t path[4] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_get_branch_indices(13u, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);

    ASSERT_ERR(ssz_get_branch_indices(13u, branch, 4u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);
    ASSERT_TRUE(branch[0] == 12u);
    ASSERT_TRUE(branch[1] == 7u);
    ASSERT_TRUE(branch[2] == 2u);

    ASSERT_ERR(ssz_get_path_indices(13u, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);

    ASSERT_ERR(ssz_get_path_indices(13u, path, 4u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);
    ASSERT_TRUE(path[0] == 13u);
    ASSERT_TRUE(path[1] == 6u);
    ASSERT_TRUE(path[2] == 3u);

    return true;
}

static bool test_get_helper_indices_multi_index(void)
{
    const ssz_gindex_t indices[2] = {10u, 11u};
    ssz_gindex_t scratch[16] = {0u};
    ssz_gindex_t helpers[4] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_get_helper_indices(indices, 2u, NULL, 0u, &out_len, scratch, 16u), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);

    ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 4u, &out_len, scratch, 16u), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);
    ASSERT_TRUE(helpers[0] == 4u);
    ASSERT_TRUE(helpers[1] == 3u);

    return true;
}

static bool test_single_proof_calculate_and_verify(void)
{
    const ssz_chunk_t l0 = make_chunk(0x00u);
    const ssz_chunk_t l1 = make_chunk(0x20u);
    const ssz_chunk_t l2 = make_chunk(0x40u);
    const ssz_chunk_t l3 = make_chunk(0x60u);

    ssz_chunk_t h01;
    ssz_chunk_t h23;
    ssz_chunk_t root;
    ASSERT_ERR(ssz_hash_2to1(NULL, &l0, &l1, &h01), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &l2, &l3, &h23), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &h01, &h23, &root), SSZ_SUCCESS);

    const ssz_chunk_t proof[2] = {l3, h01};
    ssz_chunk_t computed;

    ASSERT_ERR(ssz_calculate_merkle_root(&l2, proof, 2u, 6u, NULL, &computed), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(computed, root);

    ASSERT_ERR(ssz_verify_merkle_proof(&l2, proof, 2u, 6u, &root, NULL), SSZ_SUCCESS);

    ssz_chunk_t wrong_root = root;
    wrong_root.bytes[0] ^= 0xFFu;
    ASSERT_ERR(ssz_verify_merkle_proof(&l2, proof, 2u, 6u, &wrong_root, NULL), SSZ_ERR_PROOF_INVALID);

    ASSERT_ERR(ssz_verify_merkle_proof(&l2, proof, 1u, 6u, &root, NULL), SSZ_ERR_PROOF_INVALID);

    return true;
}

static bool test_multi_proof_calculate_and_verify(void)
{
    const ssz_chunk_t l0 = make_chunk(0x01u);
    const ssz_chunk_t l1 = make_chunk(0x21u);
    const ssz_chunk_t l2 = make_chunk(0x41u);
    const ssz_chunk_t l3 = make_chunk(0x61u);

    ssz_chunk_t h01;
    ssz_chunk_t h23;
    ssz_chunk_t root;
    ASSERT_ERR(ssz_hash_2to1(NULL, &l0, &l1, &h01), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &l2, &l3, &h23), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &h01, &h23, &root), SSZ_SUCCESS);

    const ssz_chunk_t leaves[2] = {l1, l2};
    const ssz_gindex_t indices[2] = {5u, 6u};
    const ssz_chunk_t proof[2] = {l3, l0};

    ssz_gindex_t scratch_indices[16] = {0u};
    ssz_chunk_t scratch_nodes[16];
    memset(scratch_nodes, 0, sizeof(scratch_nodes));

    ssz_chunk_t computed;
    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                               indices,
                                               2u,
                                               proof,
                                               2u,
                                               scratch_indices,
                                               scratch_nodes,
                                               16u,
                                               NULL,
                                               &computed),
               SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(computed, root);

    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            2u,
                                            proof,
                                            2u,
                                            &root,
                                            scratch_indices,
                                            scratch_nodes,
                                            16u,
                                            NULL),
               SSZ_SUCCESS);

    ssz_chunk_t wrong_root = root;
    wrong_root.bytes[0] ^= 0x01u;
    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            2u,
                                            proof,
                                            2u,
                                            &wrong_root,
                                            scratch_indices,
                                            scratch_nodes,
                                            16u,
                                            NULL),
               SSZ_ERR_PROOF_INVALID);

    return true;
}

static bool test_multi_proof_non_overlapping_indices_still_verify(void)
{
    const ssz_chunk_t l0 = make_chunk(0x10u);
    const ssz_chunk_t l1 = make_chunk(0x30u);
    const ssz_chunk_t l2 = make_chunk(0x50u);
    const ssz_chunk_t l3 = make_chunk(0x70u);

    ssz_chunk_t h01;
    ssz_chunk_t h23;
    ssz_chunk_t root;
    ASSERT_ERR(ssz_hash_2to1(NULL, &l0, &l1, &h01), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &l2, &l3, &h23), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &h01, &h23, &root), SSZ_SUCCESS);

    const ssz_chunk_t leaves[2] = {l0, l3};
    const ssz_gindex_t indices[2] = {4u, 7u};
    const ssz_chunk_t proof[2] = {l2, l1};

    ssz_gindex_t helper_scratch[16] = {0u};
    ssz_gindex_t helpers[4] = {0u};
    size_t helper_len = 0u;
    ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
               SSZ_SUCCESS);
    ASSERT_TRUE(helper_len == 2u);
    ASSERT_TRUE(helpers[0] == 6u);
    ASSERT_TRUE(helpers[1] == 5u);

    ssz_gindex_t scratch_indices[16] = {0u};
    ssz_chunk_t scratch_nodes[16];
    memset(scratch_nodes, 0, sizeof(scratch_nodes));

    ssz_chunk_t computed;
    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                               indices,
                                               2u,
                                               proof,
                                               2u,
                                               scratch_indices,
                                               scratch_nodes,
                                               16u,
                                               NULL,
                                               &computed),
               SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(computed, root);

    memset(scratch_nodes, 0, sizeof(scratch_nodes));
    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            2u,
                                            proof,
                                            2u,
                                            &root,
                                            scratch_indices,
                                            scratch_nodes,
                                            16u,
                                            NULL),
               SSZ_SUCCESS);

    return true;
}

static bool test_multi_proof_cascading_parent_reduction(void)
{
    const ssz_chunk_t n8 = make_chunk(0x12u);
    const ssz_chunk_t n5 = make_chunk(0x32u);
    const ssz_chunk_t p9 = make_chunk(0x52u);
    const ssz_chunk_t p3 = make_chunk(0x72u);
    const ssz_chunk_t leaves[2] = {n8, n5};
    const ssz_gindex_t indices[2] = {8u, 5u};
    const ssz_chunk_t proof[2] = {p9, p3};
    ssz_gindex_t helper_scratch[16] = {0u};
    ssz_gindex_t helpers[4] = {0u};
    size_t helper_len = 0u;
    ssz_chunk_t node4;
    ssz_chunk_t node2;
    ssz_chunk_t root;
    ssz_chunk_t computed;
    ssz_gindex_t scratch_indices[16] = {0u};
    ssz_chunk_t scratch_nodes[16];

    ASSERT_ERR(ssz_hash_2to1(NULL, &n8, &p9, &node4), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &node4, &n5, &node2), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &node2, &p3, &root), SSZ_SUCCESS);

    ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
               SSZ_SUCCESS);
    ASSERT_TRUE(helper_len == 2u);
    ASSERT_TRUE(helpers[0] == 9u);
    ASSERT_TRUE(helpers[1] == 3u);

    memset(scratch_nodes, 0, sizeof(scratch_nodes));
    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                               indices,
                                               2u,
                                               proof,
                                               2u,
                                               scratch_indices,
                                               scratch_nodes,
                                               16u,
                                               NULL,
                                               &computed),
               SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(computed, root);

    memset(scratch_nodes, 0, sizeof(scratch_nodes));
    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            2u,
                                            proof,
                                            2u,
                                            &root,
                                            scratch_indices,
                                            scratch_nodes,
                                            16u,
                                            NULL),
               SSZ_SUCCESS);

    return true;
}

static bool test_multi_proof_spec_valid_reducer_regressions(void)
{
    const ssz_chunk_t leaves8[8] = {
        make_chunk(0x05u),
        make_chunk(0x25u),
        make_chunk(0x45u),
        make_chunk(0x65u),
        make_chunk(0x85u),
        make_chunk(0xA5u),
        make_chunk(0xC5u),
        make_chunk(0xE5u),
    };
    ssz_chunk_t tree[16];

    memset(tree, 0, sizeof(tree));
    ASSERT_ERR(build_merkle_tree_8(leaves8, tree), SSZ_SUCCESS);

    {
        const ssz_chunk_t leaves[2] = {tree[8], tree[12]};
        const ssz_gindex_t indices[2] = {8u, 12u};
        const ssz_chunk_t proof[4] = {tree[13], tree[9], tree[7], tree[5]};
        ssz_gindex_t helper_scratch[32] = {0u};
        ssz_gindex_t helpers[8] = {0u};
        size_t helper_len = 0u;
        ssz_gindex_t scratch_indices[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        ssz_chunk_t computed;

        ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 8u, &helper_len, helper_scratch, 32u),
                   SSZ_SUCCESS);
        ASSERT_TRUE(helper_len == 4u);
        ASSERT_TRUE(helpers[0] == 13u);
        ASSERT_TRUE(helpers[1] == 9u);
        ASSERT_TRUE(helpers[2] == 7u);
        ASSERT_TRUE(helpers[3] == 5u);

        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                                   indices,
                                                   2u,
                                                   proof,
                                                   helper_len,
                                                   scratch_indices,
                                                   scratch_nodes,
                                                   16u,
                                                   NULL,
                                                   &computed),
                   SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(computed, tree[1]);

        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                                indices,
                                                2u,
                                                proof,
                                                helper_len,
                                                &tree[1],
                                                scratch_indices,
                                                scratch_nodes,
                                                16u,
                                                NULL),
                   SSZ_SUCCESS);
    }

    {
        const ssz_chunk_t leaves[2] = {tree[6], tree[8]};
        const ssz_gindex_t indices[2] = {6u, 8u};
        const ssz_chunk_t proof[3] = {tree[9], tree[7], tree[5]};
        ssz_gindex_t helper_scratch[32] = {0u};
        ssz_gindex_t helpers[8] = {0u};
        size_t helper_len = 0u;
        ssz_gindex_t scratch_indices[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        ssz_chunk_t computed;

        ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 8u, &helper_len, helper_scratch, 32u),
                   SSZ_SUCCESS);
        ASSERT_TRUE(helper_len == 3u);
        ASSERT_TRUE(helpers[0] == 9u);
        ASSERT_TRUE(helpers[1] == 7u);
        ASSERT_TRUE(helpers[2] == 5u);

        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                                   indices,
                                                   2u,
                                                   proof,
                                                   helper_len,
                                                   scratch_indices,
                                                   scratch_nodes,
                                                   16u,
                                                   NULL,
                                                   &computed),
                   SSZ_SUCCESS);
        ASSERT_CHUNK_EQ(computed, tree[1]);

        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                                indices,
                                                2u,
                                                proof,
                                                helper_len,
                                                &tree[1],
                                                scratch_indices,
                                                scratch_nodes,
                                                16u,
                                                NULL),
                   SSZ_SUCCESS);
    }

    return true;
}

static bool test_multi_proof_empty_helper_allows_null_proof(void)
{
    const ssz_chunk_t root = make_chunk(0xA5u);
    const ssz_chunk_t leaves[1] = {root};
    const ssz_gindex_t indices[1] = {1u};
    ssz_gindex_t helpers[1] = {0u};
    size_t helper_len = 0u;

    ssz_gindex_t scratch_indices[1] = {0u};
    ssz_chunk_t scratch_nodes[1];
    memset(scratch_nodes, 0, sizeof(scratch_nodes));

    ASSERT_ERR(ssz_get_helper_indices(indices, 1u, helpers, 1u, &helper_len, scratch_indices, 1u),
               SSZ_SUCCESS);
    ASSERT_TRUE(helper_len == 0u);

    ssz_chunk_t computed;
    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                               indices,
                                               1u,
                                               NULL,
                                               0u,
                                               scratch_indices,
                                               scratch_nodes,
                                               1u,
                                               NULL,
                                               &computed),
               SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(computed, root);

    memset(scratch_nodes, 0, sizeof(scratch_nodes));
    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            1u,
                                            NULL,
                                            0u,
                                            &root,
                                            scratch_indices,
                                            scratch_nodes,
                                            1u,
                                            NULL),
               SSZ_SUCCESS);

    return true;
}

static bool test_multi_proof_rejects_overlapping_indices(void)
{
    const ssz_chunk_t leaves[2] = {make_chunk(0x11u), make_chunk(0x22u)};
    const ssz_chunk_t expected_root = make_chunk(0x33u);
    ssz_chunk_t out_root;

    {
        const ssz_gindex_t indices[2] = {2u, 4u};
        ssz_gindex_t helpers[4] = {0u};
        ssz_gindex_t helper_scratch[16] = {0u};
        size_t helper_len = 0u;
        ssz_gindex_t scratch_indices[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        memset(scratch_nodes, 0, sizeof(scratch_nodes));

        ASSERT_ERR(ssz_get_helper_indices(indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                                   indices,
                                                   2u,
                                                   NULL,
                                                   0u,
                                                   scratch_indices,
                                                   scratch_nodes,
                                                   16u,
                                                   NULL,
                                                   &out_root),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                                indices,
                                                2u,
                                                NULL,
                                                0u,
                                                &expected_root,
                                                scratch_indices,
                                                scratch_nodes,
                                                16u,
                                                NULL),
                   SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const ssz_gindex_t reversed_indices[2] = {4u, 2u};
        ssz_gindex_t helpers[4] = {0u};
        ssz_gindex_t helper_scratch[16] = {0u};
        size_t helper_len = 0u;

        ASSERT_ERR(
            ssz_get_helper_indices(reversed_indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    return true;
}

static bool test_multi_proof_rejects_duplicate_and_root_overlaps(void)
{
    const ssz_chunk_t leaves[2] = {make_chunk(0x44u), make_chunk(0x55u)};
    const ssz_chunk_t expected_root = make_chunk(0x66u);
    ssz_chunk_t out_root;

    {
        const ssz_gindex_t duplicate_indices[2] = {5u, 5u};
        ssz_gindex_t helpers[4] = {0u};
        ssz_gindex_t helper_scratch[16] = {0u};
        size_t helper_len = 0u;
        ssz_gindex_t scratch_indices[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        memset(scratch_nodes, 0, sizeof(scratch_nodes));

        ASSERT_ERR(ssz_get_helper_indices(
                       duplicate_indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                                   duplicate_indices,
                                                   2u,
                                                   NULL,
                                                   0u,
                                                   scratch_indices,
                                                   scratch_nodes,
                                                   16u,
                                                   NULL,
                                                   &out_root),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                                duplicate_indices,
                                                2u,
                                                NULL,
                                                0u,
                                                &expected_root,
                                                scratch_indices,
                                                scratch_nodes,
                                                16u,
                                                NULL),
                   SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const ssz_gindex_t root_overlap_indices[2] = {1u, 2u};
        ssz_gindex_t helpers[4] = {0u};
        ssz_gindex_t helper_scratch[16] = {0u};
        size_t helper_len = 0u;
        ssz_gindex_t scratch_indices[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        memset(scratch_nodes, 0, sizeof(scratch_nodes));

        ASSERT_ERR(ssz_get_helper_indices(
                       root_overlap_indices, 2u, helpers, 4u, &helper_len, helper_scratch, 16u),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                                   root_overlap_indices,
                                                   2u,
                                                   NULL,
                                                   0u,
                                                   scratch_indices,
                                                   scratch_nodes,
                                                   16u,
                                                   NULL,
                                                   &out_root),
                   SSZ_ERR_INVALID_ARGUMENT);
        ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                                root_overlap_indices,
                                                2u,
                                                NULL,
                                                0u,
                                                &expected_root,
                                                scratch_indices,
                                                scratch_nodes,
                                                16u,
                                                NULL),
                   SSZ_ERR_INVALID_ARGUMENT);
    }

    return true;
}

static bool test_proof_error_cases(void)
{
    const ssz_chunk_t leaf = make_chunk(0xAAu);
    const ssz_chunk_t proof[1] = {make_chunk(0xBBu)};
    ssz_chunk_t out_root;

    ASSERT_ERR(ssz_calculate_merkle_root(&leaf, proof, 1u, 0u, NULL, &out_root), SSZ_ERR_GINDEX_INVALID);
    ASSERT_ERR(ssz_calculate_merkle_root(NULL, proof, 1u, 2u, NULL, &out_root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_calculate_merkle_root(&leaf, proof, 1u, 2u, NULL, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_calculate_merkle_root(&leaf, proof, 0u, 2u, NULL, &out_root), SSZ_ERR_PROOF_INVALID);

    const ssz_chunk_t leaves[2] = {make_chunk(0x01u), make_chunk(0x02u)};
    const ssz_gindex_t indices[2] = {5u, 6u};
    const ssz_chunk_t multiproof[2] = {make_chunk(0x03u), make_chunk(0x04u)};

    ssz_gindex_t scratch_indices_small[2] = {0u};
    ssz_chunk_t scratch_nodes_small[2];
    memset(scratch_nodes_small, 0, sizeof(scratch_nodes_small));

    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves,
                                               indices,
                                               2u,
                                               multiproof,
                                               2u,
                                               scratch_indices_small,
                                               scratch_nodes_small,
                                               2u,
                                               NULL,
                                               &out_root),
               SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(ssz_verify_merkle_multiproof(leaves,
                                            indices,
                                            2u,
                                            multiproof,
                                            1u,
                                            &leaf,
                                            scratch_indices_small,
                                            scratch_nodes_small,
                                            2u,
                                            NULL),
               SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(ssz_get_branch_indices(0u, NULL, 0u, NULL), SSZ_ERR_GINDEX_INVALID);
    ASSERT_ERR(ssz_get_path_indices(0u, NULL, 0u, NULL), SSZ_ERR_GINDEX_INVALID);

    const ssz_gindex_t bad_indices[1] = {0u};
    ssz_gindex_t scratch[4] = {0u};
    size_t out_len = 0u;
    ASSERT_ERR(ssz_get_helper_indices(bad_indices, 1u, NULL, 0u, &out_len, scratch, 4u),
               SSZ_ERR_GINDEX_INVALID);

    return true;
}

static bool test_get_generalized_index_error_paths(void)
{
    static const ssz_gindex_type_t leaf = {
        .kind = SSZ_GINDEX_LEAF,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };

    ssz_gindex_t out = 0u;

    ASSERT_ERR(ssz_get_generalized_index(NULL, NULL, 0u, &out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_generalized_index(&leaf, NULL, 1u, &out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_generalized_index(&leaf, (const ssz_path_step_t[]){0u}, 1u, NULL),
               SSZ_ERR_INVALID_ARGUMENT);

    static const ssz_gindex_type_t zero_chunk_elements = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 0u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&zero_chunk_elements, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_SCHEMA_INVALID);

    static const ssz_gindex_type_t container_missing_fields = {
        .kind = SSZ_GINDEX_CONTAINER,
        .chunk_count = 1u,
        .item_length = 0u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&container_missing_fields,
                                         (const ssz_path_step_t[]){0u},
                                         1u,
                                         &out),
               SSZ_ERR_GINDEX_INVALID);

    static const ssz_gindex_type_t *const null_field_types[1] = {NULL};
    static const ssz_gindex_type_t container_null_field = {
        .kind = SSZ_GINDEX_CONTAINER,
        .chunk_count = 1u,
        .item_length = 0u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = null_field_types,
    };
    ASSERT_ERR(ssz_get_generalized_index(&container_null_field, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_TYPE_MISMATCH);

    static const ssz_gindex_type_t no_elem_type = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&no_elem_type, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_SCHEMA_INVALID);

    static const ssz_gindex_type_t bad_item_len_zero = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 0u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&bad_item_len_zero, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_SCHEMA_INVALID);

    static const ssz_gindex_type_t bad_item_len_large = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 33u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&bad_item_len_large, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_SCHEMA_INVALID);

    static const ssz_gindex_type_t one_chunk_32 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&one_chunk_32, (const ssz_path_step_t[]){1u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    static const ssz_gindex_type_t packed_overflow = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 2u,
        .item_length = 8u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = UINT64_MAX,
    };
    ASSERT_ERR(ssz_get_generalized_index(&packed_overflow,
                                         (const ssz_path_step_t[]){(UINT64_MAX / 8u) + 1u},
                                         1u,
                                         &out),
               SSZ_ERR_OVERFLOW);

    static const ssz_gindex_type_t packed_oob = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 8u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 8u,
    };
    ASSERT_ERR(ssz_get_generalized_index(&packed_oob, (const ssz_path_step_t[]){4u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    static const ssz_gindex_type_t width_overflow = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = (UINT64_C(1) << 63u) + 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&width_overflow, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_OVERFLOW);

    static const ssz_gindex_type_t base_width_overflow = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = (UINT64_C(1) << 63u),
        .item_length = 32u,
        .has_mix_in_length = true,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&base_width_overflow, (const ssz_path_step_t[]){0u}, 1u, &out),
               SSZ_ERR_OVERFLOW);

    static const ssz_gindex_type_t inner_two = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 2u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    static const ssz_gindex_type_t root_overflow = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = (UINT64_C(1) << 63u),
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &inner_two,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&root_overflow, (const ssz_path_step_t[]){0u, 1u}, 2u, &out),
               SSZ_ERR_OVERFLOW);

    static const ssz_gindex_type_t length_target = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 4u,
        .item_length = 32u,
        .has_mix_in_length = true,
        .elem_type = &leaf,
        .field_types = NULL,
    };
    static const ssz_gindex_type_t length_root_overflow = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = (UINT64_C(1) << 63u),
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = &length_target,
        .field_types = NULL,
    };
    ASSERT_ERR(ssz_get_generalized_index(&length_root_overflow,
                                         (const ssz_path_step_t[]){0u, SSZ_PATH_STEP_LENGTH},
                                         2u,
                                         &out),
               SSZ_ERR_OVERFLOW);

    return true;
}

static bool test_get_generalized_index_packed_oob_rejects_padding(void)
{
    static const ssz_gindex_type_t leaf = {
        .kind = SSZ_GINDEX_LEAF,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };

    static const ssz_gindex_type_t vector_uint8_31 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 1u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 31u,
    };

    static const ssz_gindex_type_t list_uint8_31 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 1u,
        .has_mix_in_length = true,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 31u,
    };

    static const ssz_gindex_type_t vector_uint16_15 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 2u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 15u,
    };

    static const ssz_gindex_type_t vector_uint32_7 = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 4u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
        .element_count_or_limit = 7u,
    };

    static const ssz_gindex_type_t packed_missing_bound = {
        .kind = SSZ_GINDEX_ELEMENTS,
        .chunk_count = 1u,
        .item_length = 1u,
        .has_mix_in_length = false,
        .elem_type = &leaf,
        .field_types = NULL,
    };

    ssz_gindex_t out = 0u;

    ASSERT_ERR(ssz_get_generalized_index(&vector_uint8_31, (const ssz_path_step_t[]){30u}, 1u, &out),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_get_generalized_index(&vector_uint8_31, (const ssz_path_step_t[]){31u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    ASSERT_ERR(ssz_get_generalized_index(&list_uint8_31, (const ssz_path_step_t[]){30u}, 1u, &out),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_get_generalized_index(&list_uint8_31, (const ssz_path_step_t[]){31u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    ASSERT_ERR(ssz_get_generalized_index(&vector_uint16_15, (const ssz_path_step_t[]){15u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);
    ASSERT_ERR(ssz_get_generalized_index(&vector_uint32_7, (const ssz_path_step_t[]){7u}, 1u, &out),
               SSZ_ERR_GINDEX_INVALID);

    ASSERT_ERR(ssz_get_generalized_index(&packed_missing_bound,
                                         (const ssz_path_step_t[]){0u},
                                         1u,
                                         &out),
               SSZ_ERR_SCHEMA_INVALID);

    return true;
}

static bool test_indices_and_helper_error_paths(void)
{
    ssz_gindex_t out_buf[4] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_get_branch_indices(5u, NULL, 0u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_branch_indices(5u, out_buf, 0u, &out_len), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_get_path_indices(5u, NULL, 0u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_path_indices(5u, out_buf, 0u, &out_len), SSZ_ERR_BUFFER_TOO_SMALL);

    ssz_gindex_t scratch[16] = {0u};
    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){5u}, 1u, out_buf, 4u, &out_len, NULL, 1u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_helper_indices(NULL, 1u, out_buf, 4u, &out_len, scratch, 16u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){5u}, 1u, NULL, 0u, NULL, scratch, 16u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){5u}, 1u, out_buf, 0u, &out_len, scratch, 16u),
               SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){1u}, 1u, out_buf, 4u, &out_len, scratch, 16u),
               SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 0u);

    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){2u, 4u},
                                      2u,
                                      out_buf,
                                      4u,
                                      &out_len,
                                      scratch,
                                      16u),
               SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){7u, 6u, 5u},
                                      3u,
                                      NULL,
                                      0u,
                                      &out_len,
                                      scratch,
                                      16u),
               SSZ_SUCCESS);

    return true;
}

static bool test_proof_hash_and_verify_error_paths(void)
{
    const ssz_chunk_t leaf = make_chunk(0x11u);
    const ssz_chunk_t proof[1] = {make_chunk(0x22u)};
    ssz_chunk_t out_root;

    proof_hash_fail_ctx_t fail_ctx = {
        .hash_err = SSZ_SUCCESS,
        .hash_2to1_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t fail_hash = {
        .hash = proof_mock_hash,
        .hash_2to1 = proof_mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &fail_ctx,
    };

    ASSERT_ERR(ssz_calculate_merkle_root(&leaf, proof, 1u, 2u, &fail_hash, &out_root), SSZ_ERR_HASH_FAILURE);
    ASSERT_ERR(ssz_verify_merkle_proof(NULL, proof, 1u, 2u, &leaf, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_verify_merkle_proof(&leaf, proof, 1u, 2u, NULL, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_verify_merkle_multiproof((const ssz_chunk_t[]){leaf},
                                            (const ssz_gindex_t[]){2u},
                                            1u,
                                            proof,
                                            1u,
                                            NULL,
                                            (ssz_gindex_t[4]){0u},
                                            (ssz_chunk_t[4]){{0}},
                                            4u,
                                            NULL),
               SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_multiproof_additional_error_paths(void)
{
    const ssz_chunk_t l0 = make_chunk(0x01u);
    const ssz_chunk_t l1 = make_chunk(0x21u);
    const ssz_chunk_t l2 = make_chunk(0x41u);
    const ssz_chunk_t l3 = make_chunk(0x61u);

    const ssz_chunk_t leaves_ok[2] = {l1, l2};
    const ssz_gindex_t indices_ok[2] = {5u, 6u};
    const ssz_chunk_t proof_ok[2] = {l3, l0};

    ssz_gindex_t helper_scratch[16] = {0u};
    size_t helper_len = 0u;
    ASSERT_ERR(ssz_get_helper_indices(indices_ok, 2u, NULL, 0u, &helper_len, helper_scratch, 16u),
               SSZ_SUCCESS);

    ASSERT_ERR(ssz_calculate_multi_merkle_root(NULL,
                                               indices_ok,
                                               2u,
                                               proof_ok,
                                               helper_len,
                                               helper_scratch,
                                               (ssz_chunk_t[16]){{0}},
                                               16u,
                                               NULL,
                                               &(ssz_chunk_t){0}),
               SSZ_ERR_INVALID_ARGUMENT);

    ssz_gindex_t calc_scratch_idx[16] = {0u};
    ssz_chunk_t calc_scratch_nodes[16];
    memset(calc_scratch_nodes, 0, sizeof(calc_scratch_nodes));

    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves_ok,
                                               indices_ok,
                                               2u,
                                               NULL,
                                               helper_len,
                                               calc_scratch_idx,
                                               calc_scratch_nodes,
                                               16u,
                                               NULL,
                                               &(ssz_chunk_t){0}),
               SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves_ok,
                                               indices_ok,
                                               2u,
                                               proof_ok,
                                               helper_len - 1u,
                                               calc_scratch_idx,
                                               calc_scratch_nodes,
                                               16u,
                                               NULL,
                                               &(ssz_chunk_t){0}),
               SSZ_ERR_PROOF_INVALID);

    ASSERT_ERR(ssz_get_helper_indices((const ssz_gindex_t[]){2u, 2u},
                                      2u,
                                      NULL,
                                      0u,
                                      &helper_len,
                                      helper_scratch,
                                      16u),
               SSZ_ERR_INVALID_ARGUMENT);

    ssz_gindex_t zero_cap_idx[1] = {0u};
    ssz_chunk_t zero_cap_nodes[1];
    memset(zero_cap_nodes, 0, sizeof(zero_cap_nodes));
    ASSERT_ERR(ssz_calculate_multi_merkle_root((const ssz_chunk_t[]){l0},
                                               (const ssz_gindex_t[]){1u},
                                               1u,
                                               proof_ok,
                                               0u,
                                               zero_cap_idx,
                                               zero_cap_nodes,
                                               0u,
                                               NULL,
                                               &(ssz_chunk_t){0}),
               SSZ_ERR_BUFFER_TOO_SMALL);

    {
        const ssz_chunk_t single_leaf[1] = {l1};
        const ssz_gindex_t single_index[1] = {2u};
        size_t single_helper_len = 0u;
        ssz_gindex_t single_helper_scratch[8] = {0u};
        ASSERT_ERR(ssz_get_helper_indices(single_index,
                                          1u,
                                          NULL,
                                          0u,
                                          &single_helper_len,
                                          single_helper_scratch,
                                          8u),
                   SSZ_SUCCESS);

        ssz_chunk_t single_proof[2] = {l0, l3};

        {
            size_t scratch_cap = single_helper_len + 1u;
            ssz_gindex_t scratch_idx[8] = {0u};
            ssz_chunk_t scratch_nodes[8];
            memset(scratch_nodes, 0, sizeof(scratch_nodes));
            ASSERT_ERR(ssz_calculate_multi_merkle_root(single_leaf,
                                                       single_index,
                                                       1u,
                                                       single_proof,
                                                       single_helper_len,
                                                       scratch_idx,
                                                       scratch_nodes,
                                                       scratch_cap,
                                                       NULL,
                                                       &(ssz_chunk_t){0}),
                       SSZ_ERR_BUFFER_TOO_SMALL);
        }

        {
            size_t scratch_cap = single_helper_len + 2u;
            ssz_gindex_t scratch_idx[8] = {0u};
            ssz_chunk_t scratch_nodes[8];
            memset(scratch_nodes, 0, sizeof(scratch_nodes));
            ASSERT_ERR(ssz_calculate_multi_merkle_root(single_leaf,
                                                       single_index,
                                                       1u,
                                                       single_proof,
                                                       single_helper_len,
                                                       scratch_idx,
                                                       scratch_nodes,
                                                       scratch_cap,
                                                       NULL,
                                                       &(ssz_chunk_t){0}),
                       SSZ_SUCCESS);
        }
    }

    ASSERT_ERR(ssz_calculate_multi_merkle_root((const ssz_chunk_t[]){l1, l2},
                                               (const ssz_gindex_t[]){5u, 5u},
                                               2u,
                                               NULL,
                                               0u,
                                               calc_scratch_idx,
                                               calc_scratch_nodes,
                                               16u,
                                               NULL,
                                               &(ssz_chunk_t){0}),
               SSZ_ERR_INVALID_ARGUMENT);

    {
        size_t scratch_cap = helper_len + 2u;
        ssz_gindex_t small_cap_idx[8] = {0u};
        ssz_chunk_t small_cap_nodes[8];
        memset(small_cap_nodes, 0, sizeof(small_cap_nodes));
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves_ok,
                                                   indices_ok,
                                                   2u,
                                                   proof_ok,
                                                   helper_len,
                                                   small_cap_idx,
                                                   small_cap_nodes,
                                                   scratch_cap,
                                                   NULL,
                                                   &(ssz_chunk_t){0}),
                   SSZ_ERR_BUFFER_TOO_SMALL);
    }

    proof_hash_fail_ctx_t hash_fail_ctx = {
        .hash_err = SSZ_SUCCESS,
        .hash_2to1_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t fail_hash = {
        .hash = proof_mock_hash,
        .hash_2to1 = proof_mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &hash_fail_ctx,
    };
    {
        ssz_gindex_t scratch_idx[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves_ok,
                                                   indices_ok,
                                                   2u,
                                                   proof_ok,
                                                   helper_len,
                                                   scratch_idx,
                                                   scratch_nodes,
                                                   16u,
                                                   &fail_hash,
                                                   &(ssz_chunk_t){0}),
                   SSZ_ERR_HASH_FAILURE);
    }

    {
        size_t scratch_cap = helper_len + 4u;
        ssz_gindex_t scratch_idx[16] = {0u};
        ssz_chunk_t scratch_nodes[16];
        memset(scratch_nodes, 0, sizeof(scratch_nodes));
        ASSERT_ERR(ssz_calculate_multi_merkle_root(leaves_ok,
                                                   indices_ok,
                                                   2u,
                                                   proof_ok,
                                                   helper_len,
                                                   scratch_idx,
                                                   scratch_nodes,
                                                   scratch_cap,
                                                   NULL,
                                                   &(ssz_chunk_t){0}),
                   SSZ_ERR_BUFFER_TOO_SMALL);
    }

    return true;
}

static bool test_helper_indices_descending_sort(void)
{
    /* Indices {32, 47, 55} are depth-5 nodes in separate subtrees.  They
       produce a 10-element diff array {7,9,10,12,17,22,26,33,46,54} that is
       in ascending order, so the helper sorter has to fully reorder it into
       descending output. */
    const ssz_gindex_t indices[3] = {32u, 47u, 55u};
    ssz_gindex_t scratch[64] = {0u};
    ssz_gindex_t helpers[16] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_get_helper_indices(indices, 3u, helpers, 16u, &out_len, scratch, 64u), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 10u);
    /* Must come back in descending order. */
    ASSERT_TRUE(helpers[0] == 54u);
    ASSERT_TRUE(helpers[1] == 46u);
    ASSERT_TRUE(helpers[2] == 33u);
    ASSERT_TRUE(helpers[3] == 26u);
    ASSERT_TRUE(helpers[4] == 22u);
    ASSERT_TRUE(helpers[5] == 17u);
    ASSERT_TRUE(helpers[6] == 12u);
    ASSERT_TRUE(helpers[7] == 10u);
    ASSERT_TRUE(helpers[8] == 9u);
    ASSERT_TRUE(helpers[9] == 7u);

    return true;
}

static bool test_proof_alignment_error_paths(void)
{
    const ssz_chunk_t leaf = make_chunk(0x11u);
    const ssz_chunk_t proof[1] = {make_chunk(0x22u)};
    const ssz_chunk_t expected_root = make_chunk(0x33u);
    ssz_gindex_t scratch_indices[4] = {0u};
    ssz_chunk_t out_root = make_chunk(0x44u);
    uint8_t leaf_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t expected_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t scratch_raw[(sizeof(ssz_chunk_t) * 2u) + SSZ_CHUNK_ALIGNMENT] = {0u};

    ASSERT_TRUE(SSZ_CHUNK_ALIGNMENT > 1u);

    (void)memcpy((void *)misaligned_chunk_ptr(leaf_raw), &leaf, sizeof(leaf));
    (void)memcpy((void *)misaligned_chunk_ptr(expected_raw), &expected_root, sizeof(expected_root));

    ASSERT_ERR(ssz_calculate_merkle_root((const ssz_chunk_t *)(const void *)misaligned_chunk_ptr(leaf_raw),
                                         proof,
                                         1u,
                                         2u,
                                         NULL,
                                         &out_root),
               SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR(ssz_verify_merkle_proof(&leaf,
                                       proof,
                                       1u,
                                       2u,
                                       (const ssz_chunk_t *)(const void *)misaligned_chunk_ptr(expected_raw),
                                       NULL),
               SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR(ssz_calculate_multi_merkle_root(&leaf,
                                               (const ssz_gindex_t[]){1u},
                                               1u,
                                               NULL,
                                               0u,
                                               scratch_indices,
                                               misaligned_chunk_ptr(scratch_raw),
                                               2u,
                                               NULL,
                                               &out_root),
               SSZ_ERR_ALIGNMENT_INVALID);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"next_pow_of_two_edges", test_next_pow_of_two_edges},
        {"generalized_index_inline_helpers", test_generalized_index_inline_helpers},
        {"get_generalized_index_paths", test_get_generalized_index_paths},
        {"get_branch_and_path_indices", test_get_branch_and_path_indices},
        {"get_helper_indices_multi_index", test_get_helper_indices_multi_index},
        {"single_proof_calculate_and_verify", test_single_proof_calculate_and_verify},
        {"multi_proof_calculate_and_verify", test_multi_proof_calculate_and_verify},
        {"multi_proof_non_overlapping_indices_still_verify",
         test_multi_proof_non_overlapping_indices_still_verify},
        {"multi_proof_cascading_parent_reduction", test_multi_proof_cascading_parent_reduction},
        {"multi_proof_spec_valid_reducer_regressions",
         test_multi_proof_spec_valid_reducer_regressions},
        {"multi_proof_empty_helper_allows_null_proof", test_multi_proof_empty_helper_allows_null_proof},
        {"multi_proof_rejects_overlapping_indices", test_multi_proof_rejects_overlapping_indices},
        {"multi_proof_rejects_duplicate_and_root_overlaps",
         test_multi_proof_rejects_duplicate_and_root_overlaps},
        {"proof_error_cases", test_proof_error_cases},
        {"get_generalized_index_error_paths", test_get_generalized_index_error_paths},
        {"get_generalized_index_packed_oob_rejects_padding",
         test_get_generalized_index_packed_oob_rejects_padding},
        {"indices_and_helper_error_paths", test_indices_and_helper_error_paths},
        {"proof_hash_and_verify_error_paths", test_proof_hash_and_verify_error_paths},
        {"multiproof_additional_error_paths", test_multiproof_additional_error_paths},
        {"helper_indices_descending_sort", test_helper_indices_descending_sort},
        {"proof_alignment_error_paths", test_proof_alignment_error_paths},
    };

    size_t passed = 0u;
    const size_t total = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0u; i < total; i++)
    {
        if (!tests[i].fn())
        {
            fprintf(stderr, "[FAIL] %s\n", tests[i].name);
            return 1;
        }
        passed++;
    }

    printf("[OK] %zu/%zu proof tests passed\n", passed, total);
    return 0;
}
