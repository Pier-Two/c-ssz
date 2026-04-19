#include "ubench.h"
#include "ssz.h"

#include <stdint.h>
#include <string.h>

#define MERKLE_LEAF_COUNT      1024u
#define MERKLE_TREE_NODE_CAP   (MERKLE_LEAF_COUNT * 2u)
#define MERKLE_SINGLE_LEAF_POS 777u
#define MERKLE_MAX_DEPTH       64u

#define MERKLE_MULTI_LEAF_COUNT 4u
#define MERKLE_HELPER_CAP       1024u
#define MERKLE_SCRATCH_CAP      4096u

#define MERKLE_SMALL_BATCH 64u

#define BENCH_EXPECT_OK(expr)                                                                        \
    do                                                                                               \
    {                                                                                                \
        ssz_error_t bench_err__ = (expr);                                                            \
        if (bench_err__ != SSZ_SUCCESS)                                                              \
        {                                                                                            \
            ubench_do_nothing((void *)&bench_err__);                                                \
            return;                                                                                  \
        }                                                                                            \
    } while (0)

static int g_init_state = 0;

static ssz_chunk_t g_tree[MERKLE_TREE_NODE_CAP];
static ssz_chunk_t g_root;
static ssz_chunk_t g_bench_merkle_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_bench_merkle_scratch = {
    .chunks = g_bench_merkle_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

static ssz_gindex_t g_single_index = 0u;
static ssz_chunk_t g_single_leaf;
static ssz_chunk_t g_single_proof[MERKLE_MAX_DEPTH];
static size_t g_single_proof_len = 0u;

static ssz_gindex_t g_multi_indices[MERKLE_MULTI_LEAF_COUNT];
static ssz_chunk_t g_multi_leaves[MERKLE_MULTI_LEAF_COUNT];
static ssz_gindex_t g_multi_helper_indices[MERKLE_HELPER_CAP];
static ssz_chunk_t g_multi_proof[MERKLE_HELPER_CAP];
static size_t g_multi_proof_count = 0u;

static ssz_gindex_t g_branch_indices_buf[MERKLE_MAX_DEPTH];
static ssz_chunk_t g_generated_single_proof[MERKLE_MAX_DEPTH];

static ssz_gindex_t g_generated_helper_indices[MERKLE_HELPER_CAP];
static ssz_chunk_t g_generated_multi_proof[MERKLE_HELPER_CAP];

static ssz_gindex_t g_scratch_indices[MERKLE_SCRATCH_CAP];
static ssz_chunk_t g_scratch_nodes[MERKLE_SCRATCH_CAP];
static const uint8_t g_active_fields[2] = {0x0Fu, 0x01u};

static const ssz_gindex_type_t g_gindex_leaf = {
    .kind = SSZ_GINDEX_LEAF,
    .chunk_count = 1u,
    .item_length = 32u,
    .has_mix_in_length = false,
    .elem_type = NULL,
    .field_types = NULL,
};

static const ssz_gindex_type_t g_gindex_vector32 = {
    .kind = SSZ_GINDEX_ELEMENTS,
    .chunk_count = 4u,
    .item_length = 32u,
    .has_mix_in_length = false,
    .elem_type = &g_gindex_leaf,
    .field_types = NULL,
};

static const ssz_gindex_type_t g_gindex_list32 = {
    .kind = SSZ_GINDEX_ELEMENTS,
    .chunk_count = 4u,
    .item_length = 32u,
    .has_mix_in_length = true,
    .elem_type = &g_gindex_leaf,
    .field_types = NULL,
};

static const ssz_gindex_type_t *const g_gindex_container_fields[3] = {
    &g_gindex_leaf,
    &g_gindex_vector32,
    &g_gindex_list32,
};

static const ssz_gindex_type_t g_gindex_container = {
    .kind = SSZ_GINDEX_CONTAINER,
    .chunk_count = 3u,
    .item_length = 0u,
    .has_mix_in_length = false,
    .elem_type = NULL,
    .field_types = g_gindex_container_fields,
};

static ssz_chunk_t bench_make_leaf(uint64_t idx)
{
    ssz_chunk_t leaf;
    uint8_t mix0 = (uint8_t)(idx & 0xFFu);
    uint8_t mix1 = (uint8_t)((idx >> 8u) & 0xFFu);

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        leaf.bytes[i] = (uint8_t)(mix0 ^ (uint8_t)(i * 13u) ^ mix1);
    }

    return leaf;
}

static void bench_init_merkle_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    for (size_t i = 0u; i < MERKLE_LEAF_COUNT; i++)
    {
        g_tree[MERKLE_LEAF_COUNT + i] = bench_make_leaf(i);
    }

    for (size_t i = MERKLE_LEAF_COUNT; i-- > 1u;)
    {
        ssz_error_t err = ssz_hash_2to1(
            NULL,
            &g_tree[i << 1u],
            &g_tree[(i << 1u) | 1u],
            &g_tree[i]);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    g_root = g_tree[1u];

    g_single_index = (ssz_gindex_t)(MERKLE_LEAF_COUNT + MERKLE_SINGLE_LEAF_POS);
    g_single_leaf = g_tree[g_single_index];

    g_single_proof_len = 0u;
    ssz_gindex_t cursor = g_single_index;
    while (cursor > 1u)
    {
        if (g_single_proof_len >= MERKLE_MAX_DEPTH)
        {
            g_init_state = -1;
            return;
        }

        ssz_gindex_t sibling = cursor ^ 1u;
        g_single_proof[g_single_proof_len++] = g_tree[sibling];
        cursor >>= 1u;
    }

    static const size_t k_multi_leaf_positions[MERKLE_MULTI_LEAF_COUNT] = {5u, 77u, 333u, 900u};
    for (size_t i = 0u; i < MERKLE_MULTI_LEAF_COUNT; i++)
    {
        g_multi_indices[i] = (ssz_gindex_t)(MERKLE_LEAF_COUNT + k_multi_leaf_positions[i]);
        g_multi_leaves[i] = g_tree[g_multi_indices[i]];
    }

    size_t helper_len = 0u;
    ssz_error_t err = ssz_get_helper_indices(g_multi_indices,
                                             MERKLE_MULTI_LEAF_COUNT,
                                             NULL,
                                             0u,
                                             &helper_len,
                                             g_scratch_indices,
                                             MERKLE_SCRATCH_CAP);
    if ((err != SSZ_SUCCESS) || (helper_len > MERKLE_HELPER_CAP))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_get_helper_indices(g_multi_indices,
                                 MERKLE_MULTI_LEAF_COUNT,
                                 g_multi_helper_indices,
                                 MERKLE_HELPER_CAP,
                                 &helper_len,
                                 g_scratch_indices,
                                 MERKLE_SCRATCH_CAP);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    g_multi_proof_count = helper_len;
    for (size_t i = 0u; i < g_multi_proof_count; i++)
    {
        if (g_multi_helper_indices[i] >= MERKLE_TREE_NODE_CAP)
        {
            g_init_state = -1;
            return;
        }
        g_multi_proof[i] = g_tree[g_multi_helper_indices[i]];
    }

    err = ssz_verify_merkle_proof(&g_single_leaf,
                                  g_single_proof,
                                  g_single_proof_len,
                                  g_single_index,
                                  &g_root,
                                  NULL);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    memset(g_scratch_indices, 0, sizeof(g_scratch_indices));
    memset(g_scratch_nodes, 0, sizeof(g_scratch_nodes));

    err = ssz_verify_merkle_multiproof(g_multi_leaves,
                                       g_multi_indices,
                                       MERKLE_MULTI_LEAF_COUNT,
                                       g_multi_proof,
                                       g_multi_proof_count,
                                       &g_root,
                                       g_scratch_indices,
                                       g_scratch_nodes,
                                       MERKLE_SCRATCH_CAP,
                                       NULL);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    g_init_state = 1;
}

UBENCH(merkle, merkleize)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_merkleize(
        &g_tree[MERKLE_LEAF_COUNT], MERKLE_LEAF_COUNT, SSZ_NO_LIMIT, &g_bench_merkle_scratch, NULL, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle, mix_in_length)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t mixed;
    for (size_t i = 0u; i < MERKLE_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_mix_in_length_u64(&g_root, (uint64_t)i, NULL, &mixed));
    }
    ubench_do_nothing((void *)&mixed);
}

UBENCH(merkle, mix_in_selector)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t mixed;
    for (size_t i = 0u; i < MERKLE_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_mix_in_selector(&g_root, (uint8_t)i, NULL, &mixed));
    }
    ubench_do_nothing((void *)&mixed);
}

UBENCH(merkle, mix_in_active_fields)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t mixed;
    for (size_t i = 0u; i < MERKLE_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_mix_in_active_fields(&g_root,
                                                 g_active_fields,
                                                 sizeof(g_active_fields),
                                                 NULL,
                                                 &mixed));
    }
    ubench_do_nothing((void *)&mixed);
}

UBENCH(merkle, get_generalized_index)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    const ssz_path_step_t path[2] = {1u, 2u};
    ssz_gindex_t index = 0u;
    for (size_t i = 0u; i < MERKLE_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_get_generalized_index(&g_gindex_container, path, 2u, &index));
    }
    ubench_do_nothing((void *)&index);
}

UBENCH(merkle, get_path_indices)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_gindex_t indices[MERKLE_MAX_DEPTH];
    size_t out_len = 0u;
    for (size_t i = 0u; i < MERKLE_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_get_path_indices(g_single_index, indices, MERKLE_MAX_DEPTH, &out_len));
    }

    ubench_do_nothing(indices);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(merkle, compute_single_proof)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t branch_len = 0u;
    for (size_t iter = 0u; iter < MERKLE_SMALL_BATCH; iter++)
    {
        BENCH_EXPECT_OK(ssz_get_branch_indices(
            g_single_index,
            g_branch_indices_buf,
            MERKLE_MAX_DEPTH,
            &branch_len));

        if (branch_len > MERKLE_MAX_DEPTH)
        {
            return;
        }

        for (size_t i = 0u; i < branch_len; i++)
        {
            if (g_branch_indices_buf[i] >= MERKLE_TREE_NODE_CAP)
            {
                return;
            }
            g_generated_single_proof[i] = g_tree[g_branch_indices_buf[i]];
        }
    }

    ubench_do_nothing((void *)&branch_len);
    ubench_do_nothing(g_generated_single_proof);
}

UBENCH(merkle, verify_single_proof)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    BENCH_EXPECT_OK(ssz_verify_merkle_proof(&g_single_leaf,
                                            g_single_proof,
                                            g_single_proof_len,
                                            g_single_index,
                                            &g_root,
                                            NULL));

    ubench_do_nothing((void *)&g_root);
}

UBENCH(merkle, calculate_single_merkle_root)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t computed_root;
    BENCH_EXPECT_OK(ssz_calculate_merkle_root(&g_single_leaf,
                                              g_single_proof,
                                              g_single_proof_len,
                                              g_single_index,
                                              NULL,
                                              &computed_root));

    ubench_do_nothing((void *)&computed_root);
}

UBENCH(merkle, compute_multi_proof)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t helper_len = 0u;
    BENCH_EXPECT_OK(ssz_get_helper_indices(g_multi_indices,
                                           MERKLE_MULTI_LEAF_COUNT,
                                           g_generated_helper_indices,
                                           MERKLE_HELPER_CAP,
                                           &helper_len,
                                           g_scratch_indices,
                                           MERKLE_SCRATCH_CAP));

    if (helper_len > MERKLE_HELPER_CAP)
    {
        return;
    }

    for (size_t i = 0u; i < helper_len; i++)
    {
        if (g_generated_helper_indices[i] >= MERKLE_TREE_NODE_CAP)
        {
            return;
        }
        g_generated_multi_proof[i] = g_tree[g_generated_helper_indices[i]];
    }

    ubench_do_nothing((void *)&helper_len);
    ubench_do_nothing(g_generated_multi_proof);
}

UBENCH(merkle, verify_multi_proof)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    memset(g_scratch_indices, 0, sizeof(g_scratch_indices));
    memset(g_scratch_nodes, 0, sizeof(g_scratch_nodes));

    BENCH_EXPECT_OK(ssz_verify_merkle_multiproof(g_multi_leaves,
                                                  g_multi_indices,
                                                  MERKLE_MULTI_LEAF_COUNT,
                                                  g_multi_proof,
                                                  g_multi_proof_count,
                                                  &g_root,
                                                  g_scratch_indices,
                                                  g_scratch_nodes,
                                                  MERKLE_SCRATCH_CAP,
                                                  NULL));

    ubench_do_nothing((void *)&g_root);
}

UBENCH(merkle, calculate_multi_merkle_root)
{
    bench_init_merkle_data();
    if (g_init_state != 1)
    {
        return;
    }

    memset(g_scratch_indices, 0, sizeof(g_scratch_indices));
    memset(g_scratch_nodes, 0, sizeof(g_scratch_nodes));

    ssz_chunk_t computed_root;
    BENCH_EXPECT_OK(ssz_calculate_multi_merkle_root(g_multi_leaves,
                                                     g_multi_indices,
                                                     MERKLE_MULTI_LEAF_COUNT,
                                                     g_multi_proof,
                                                     g_multi_proof_count,
                                                     g_scratch_indices,
                                                     g_scratch_nodes,
                                                     MERKLE_SCRATCH_CAP,
                                                     NULL,
                                                     &computed_root));

    ubench_do_nothing((void *)&computed_root);
}

UBENCH_MAIN();
