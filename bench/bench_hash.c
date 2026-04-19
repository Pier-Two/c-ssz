#include <stdint.h>
#include <string.h>

#include "ssz.h"
#include "ubench.h"

#define HASH_ELEMENT_SIZE 32u

#define HASH_VECTOR_SMALL_COUNT 128u
#define HASH_VECTOR_LARGE_COUNT 1024u
#define HASH_VECTOR_SMALL_BYTES (HASH_VECTOR_SMALL_COUNT * HASH_ELEMENT_SIZE)
#define HASH_VECTOR_LARGE_BYTES (HASH_VECTOR_LARGE_COUNT * HASH_ELEMENT_SIZE)

#define HASH_LIST_SMALL_COUNT 128u
#define HASH_LIST_SMALL_LIMIT 256u
#define HASH_LIST_LARGE_COUNT 1024u
#define HASH_LIST_LARGE_LIMIT 2048u
#define HASH_LIST_SMALL_BYTES (HASH_LIST_SMALL_COUNT * HASH_ELEMENT_SIZE)
#define HASH_LIST_LARGE_BYTES (HASH_LIST_LARGE_COUNT * HASH_ELEMENT_SIZE)

#define HASH_COMPOSITE_VECTOR_COUNT 256u
#define HASH_COMPOSITE_LIST_COUNT   256u
#define HASH_COMPOSITE_LIST_LIMIT   512u

#define HASH_CONTAINER_FIELD_COUNT 8u

#define HASH_BITVECTOR_BIT_COUNT 8192u
#define HASH_BITVECTOR_BYTES     (HASH_BITVECTOR_BIT_COUNT / 8u)

#define HASH_BITLIST_BIT_LEN 8190u
#define HASH_BITLIST_BIT_MAX 16384u
#define HASH_BITLIST_BYTES   ((HASH_BITLIST_BIT_LEN + 7u) / 8u)

#define HASH_SMALL_BATCH 64u
#define HASH_FAST_BATCH  256u

#define HASH_SHA256_INPUT_BYTES 64u
#define HASH_BATCH_PAIR_COUNT   16u

typedef struct
{
    uint8_t seed;
} bench_root_ctx_t;

static ssz_error_t bench_generated_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const bench_root_ctx_t *root_ctx = (const bench_root_ctx_t *)ctx;
    if ((root_ctx == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    uint8_t member_mix = (uint8_t)(member_id & 0xFFu);
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        out_root->bytes[i] = (uint8_t)(root_ctx->seed + member_mix + (uint8_t)(i * 9u));
    }

    return SSZ_SUCCESS;
}

#define BENCH_EXPECT_OK(expr)                        \
    do                                               \
    {                                                \
        ssz_error_t bench_err__ = (expr);            \
        if (bench_err__ != SSZ_SUCCESS)              \
        {                                            \
            ubench_do_nothing((void *)&bench_err__); \
            return;                                  \
        }                                            \
    } while (0)

static int g_init_state = 0;

static uint8_t g_uint8_input = 0x5Au;
static uint16_t g_uint16_input = UINT16_C(0xBEEF);
static uint32_t g_uint32_input = UINT32_C(0x78563412);
static uint64_t g_uint64_input = UINT64_C(0x0102030405060708);
static uint8_t g_uint128_input[16];
static uint8_t g_uint256_input[32];

static uint8_t g_vector_fixed_data[HASH_VECTOR_LARGE_BYTES];
static uint8_t g_list_fixed_data[HASH_LIST_LARGE_BYTES];

static uint8_t g_bitvector_bits[HASH_BITVECTOR_BYTES];
static uint8_t g_bitlist_bits[HASH_BITLIST_BYTES];
static uint8_t g_sha256_input[HASH_SHA256_INPUT_BYTES];

static ssz_chunk_t g_vector_roots[HASH_COMPOSITE_VECTOR_COUNT];
static ssz_chunk_t g_list_roots[HASH_COMPOSITE_LIST_COUNT];

static bench_root_ctx_t g_vector_composite_ctx;
static bench_root_ctx_t g_list_composite_ctx;
static bench_root_ctx_t g_container_ctx;

static ssz_member_codec_t g_vector_composite_codec;
static ssz_member_codec_t g_list_composite_codec;
static ssz_member_codec_t g_container_codec;
static ssz_chunk_t g_bench_hash_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_bench_hash_scratch = {
    .chunks = g_bench_hash_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

static void bench_init_hash_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    for (size_t i = 0u; i < sizeof(g_uint256_input); i++)
    {
        g_uint256_input[i] = (uint8_t)(0x80u + (uint8_t)i);
    }

    for (size_t i = 0u; i < sizeof(g_uint128_input); i++)
    {
        g_uint128_input[i] = (uint8_t)(0x50u + (uint8_t)i);
    }

    for (size_t i = 0u; i < sizeof(g_vector_fixed_data); i++)
    {
        g_vector_fixed_data[i] = (uint8_t)(i * 3u + 17u);
    }

    for (size_t i = 0u; i < sizeof(g_list_fixed_data); i++)
    {
        g_list_fixed_data[i] = (uint8_t)(i * 5u + 29u);
    }

    for (size_t i = 0u; i < sizeof(g_bitvector_bits); i++)
    {
        g_bitvector_bits[i] = (uint8_t)(0x5Au ^ (uint8_t)(i * 7u));
    }

    for (size_t i = 0u; i < sizeof(g_bitlist_bits); i++)
    {
        g_bitlist_bits[i] = (uint8_t)(0xA5u ^ (uint8_t)(i * 11u));
    }
    g_bitlist_bits[sizeof(g_bitlist_bits) - 1u] &= 0x3Fu;

    for (size_t i = 0u; i < sizeof(g_sha256_input); i++)
    {
        g_sha256_input[i] = (uint8_t)(0x30u + (uint8_t)i);
    }

    g_vector_composite_ctx = (bench_root_ctx_t){.seed = 0x10u};
    g_list_composite_ctx = (bench_root_ctx_t){.seed = 0x40u};
    g_container_ctx = (bench_root_ctx_t){.seed = 0x70u};

    g_vector_composite_codec = (ssz_member_codec_t){
        .ctx = &g_vector_composite_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_generated_root,
    };
    g_list_composite_codec = (ssz_member_codec_t){
        .ctx = &g_list_composite_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_generated_root,
    };
    g_container_codec = (ssz_member_codec_t){
        .ctx = &g_container_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_generated_root,
    };

    for (uint64_t i = 0u; i < HASH_COMPOSITE_VECTOR_COUNT; i++)
    {
        if (bench_generated_root(&g_vector_composite_ctx, i, &g_vector_roots[i]) != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    for (uint64_t i = 0u; i < HASH_COMPOSITE_LIST_COUNT; i++)
    {
        if (bench_generated_root(&g_list_composite_ctx, i, &g_list_roots[i]) != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    ssz_chunk_t sanity_root;
    ssz_error_t err = ssz_hash_tree_root_bitlist(
        g_bitlist_bits,
        sizeof(g_bitlist_bits),
        HASH_BITLIST_BIT_LEN,
        HASH_BITLIST_BIT_MAX,
        &g_bench_hash_scratch,
        NULL,
        &sanity_root);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    err = ssz_hash_tree_root_vector_composite(
        HASH_CONTAINER_FIELD_COUNT,
        &g_container_codec,
        &g_bench_hash_scratch,
        NULL,
        &sanity_root);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    g_init_state = 1;
}

UBENCH(hash, tree_root_uint8)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint8((uint8_t)(g_uint8_input + (uint8_t)i), &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_uint16)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint16((uint16_t)(g_uint16_input + (uint16_t)i), &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_uint32)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint32(g_uint32_input + (uint32_t)i, &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_uint128)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t in[16];
    ssz_chunk_t root;
    memcpy(in, g_uint128_input, sizeof(in));

    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        in[0] = (uint8_t)(g_uint128_input[0] + (uint8_t)i);
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint128(in, sizeof(in), &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_boolean)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_hash_tree_root_boolean((uint8_t)(i & 1u), &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_uint64)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint64(g_uint64_input + i, &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_uint256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t in[32];
    ssz_chunk_t root;
    memcpy(in, g_uint256_input, sizeof(in));

    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        in[0] = (uint8_t)(g_uint256_input[0] + (uint8_t)i);
        BENCH_EXPECT_OK(ssz_hash_tree_root_uint256(in, sizeof(in), &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_vector_fixed_128x32)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_fixed(
        g_vector_fixed_data,
        HASH_VECTOR_SMALL_COUNT,
        HASH_ELEMENT_SIZE,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_vector_fixed_1024x32)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_fixed(
        g_vector_fixed_data,
        HASH_VECTOR_LARGE_COUNT,
        HASH_ELEMENT_SIZE,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_list_fixed_128x32)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_fixed(
        g_list_fixed_data,
        HASH_LIST_SMALL_COUNT,
        HASH_LIST_SMALL_LIMIT,
        HASH_ELEMENT_SIZE,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_list_fixed_1024x32)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_fixed(
        g_list_fixed_data,
        HASH_LIST_LARGE_COUNT,
        HASH_LIST_LARGE_LIMIT,
        HASH_ELEMENT_SIZE,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_vector_composite_256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_composite(
        HASH_COMPOSITE_VECTOR_COUNT,
        &g_vector_composite_codec,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_list_composite_256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_composite(
        HASH_COMPOSITE_LIST_COUNT,
        HASH_COMPOSITE_LIST_LIMIT,
        &g_list_composite_codec,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_vector_roots_256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_roots(
        g_vector_roots,
        HASH_COMPOSITE_VECTOR_COUNT,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_list_roots_256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_roots(
        g_list_roots,
        HASH_COMPOSITE_LIST_COUNT,
        HASH_COMPOSITE_LIST_LIMIT,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_union)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        uint8_t selector = (i & 1u) ? 0u : 2u;
        BENCH_EXPECT_OK(ssz_hash_tree_root_union(selector, true, &g_container_codec, NULL, &root));
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_container_8_fields)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_composite(
        HASH_CONTAINER_FIELD_COUNT,
        &g_container_codec,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_bitvector_8192)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_bitvector(
        g_bitvector_bits,
        sizeof(g_bitvector_bits),
        HASH_BITVECTOR_BIT_COUNT,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, tree_root_bitlist_8190)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_bitlist(
        g_bitlist_bits,
        sizeof(g_bitlist_bits),
        HASH_BITLIST_BIT_LEN,
        HASH_BITLIST_BIT_MAX,
        &g_bench_hash_scratch,
        NULL,
        &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(hash, sha256)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t in[HASH_SHA256_INPUT_BYTES];
    uint8_t out[32];
    memcpy(in, g_sha256_input, sizeof(in));

    for (size_t i = 0u; i < HASH_FAST_BATCH; i++)
    {
        in[0] = (uint8_t)(g_sha256_input[0] + (uint8_t)i);
        BENCH_EXPECT_OK(ssz_hash_sha256(in, sizeof(in), out));
    }
    ubench_do_nothing(out);
}

UBENCH(hash, hash_2to1_batch_16)
{
    bench_init_hash_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t pairs[HASH_BATCH_PAIR_COUNT * 2u];
    ssz_chunk_t out[HASH_BATCH_PAIR_COUNT];
    for (size_t i = 0u; i < HASH_BATCH_PAIR_COUNT * 2u; i++)
    {
        pairs[i] = g_vector_roots[i];
    }

    for (size_t i = 0u; i < HASH_SMALL_BATCH; i++)
    {
        pairs[0].bytes[0] = (uint8_t)(pairs[0].bytes[0] ^ (uint8_t)i);
        BENCH_EXPECT_OK(ssz_hash_2to1_batch(NULL, pairs, HASH_BATCH_PAIR_COUNT, out));
    }
    ubench_do_nothing(out);
}

UBENCH_MAIN();
