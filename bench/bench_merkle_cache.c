#include "ubench.h"
#include "ssz.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define CACHE_LIST_U64_COUNT        1000000u
#define CACHE_LIST_U64_LIMIT        UINT64_C(1000000)
#define CACHE_LIST_U64_ELEMENT_SIZE 8u
#define CACHE_LIST_U64S_PER_CHUNK   (SSZ_BYTES_PER_CHUNK / CACHE_LIST_U64_ELEMENT_SIZE)
#define CACHE_LIST_CHUNK_COUNT      (CACHE_LIST_U64_COUNT / CACHE_LIST_U64S_PER_CHUNK)

#define CACHE_INCREMENTAL_UPDATE_1    1u
#define CACHE_INCREMENTAL_UPDATE_10   10u
#define CACHE_INCREMENTAL_UPDATE_100  100u
#define CACHE_INCREMENTAL_UPDATE_1000 1000u

#define CACHE_COMPOSITE_FIELD_COUNT 17u
#define CACHE_UNCHANGED_QUERY_BATCH  1024u

#define CACHE_EPH_COUNT                1000u
#define CACHE_EPH_LIMIT                UINT64_C(1000)
#define CACHE_EPH_FIELD_COUNT          17u
#define CACHE_EPH_EXTRA_DATA_FIELD_ID  10u
#define CACHE_EPH_EXTRA_DATA_LEN       16u
#define CACHE_EPH_EXTRA_DATA_MAX_LEN   32u
#define CACHE_EPH_INCREMENTAL_BATCH_10 10u

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

typedef struct
{
    ssz_chunk_t *roots;
    uint64_t *tokens;
    uint64_t count;
} bench_composite_ctx_t;

typedef struct
{
    uint8_t parent_hash[32];
    uint8_t fee_recipient[20];
    uint8_t state_root[32];
    uint8_t receipts_root[32];
    uint8_t logs_bloom[256];
    uint8_t prev_randao[32];
    uint8_t block_number[8];
    uint8_t gas_limit[8];
    uint8_t gas_used[8];
    uint8_t timestamp[8];
    uint8_t extra_data[CACHE_EPH_EXTRA_DATA_LEN];
    uint8_t base_fee_per_gas[32];
    uint8_t block_hash[32];
    uint8_t transactions_root[32];
    uint8_t withdrawals_root[32];
    uint8_t blob_gas_used[8];
    uint8_t excess_blob_gas[8];
} bench_cached_eph_t;

typedef struct
{
    const bench_cached_eph_t *headers;
    uint64_t count;
} bench_cached_eph_list_ctx_t;

static int g_init_state = 0;

static uint64_t g_list_values_a[CACHE_LIST_U64_COUNT];
static uint64_t g_list_values_b[CACHE_LIST_U64_COUNT];
static uint64_t g_list_values_incremental[CACHE_LIST_U64_COUNT];

static uint64_t g_incremental_indices[CACHE_INCREMENTAL_UPDATE_1000];
static uint64_t g_incremental_update_tick = 1u;
static uint8_t g_full_build_toggle = 0u;

static ssz_merkle_cache_t *g_cache_full_build = NULL;
static ssz_merkle_cache_t *g_cache_incremental = NULL;
static ssz_merkle_cache_t *g_cache_unchanged = NULL;

static ssz_chunk_t g_composite_roots[CACHE_COMPOSITE_FIELD_COUNT];
static uint64_t g_composite_tokens[CACHE_COMPOSITE_FIELD_COUNT];
static uint64_t g_composite_update_tick = 1u;
static bench_composite_ctx_t g_composite_ctx;
static ssz_member_codec_t g_composite_codec;
static ssz_merkle_cache_sync_composite_opts_t g_composite_opts;
static ssz_merkle_cache_t *g_cache_composite = NULL;

static bench_cached_eph_t g_cached_eph_headers_baseline[CACHE_EPH_COUNT];
static bench_cached_eph_t g_cached_eph_headers_incremental_1[CACHE_EPH_COUNT];
static bench_cached_eph_t g_cached_eph_headers_incremental_10[CACHE_EPH_COUNT];

static ssz_merkle_cache_t *g_cached_eph_element_caches_incremental_1[CACHE_EPH_COUNT];
static ssz_merkle_cache_t *g_cached_eph_element_caches_incremental_10[CACHE_EPH_COUNT];
static ssz_merkle_cache_t *g_cached_eph_list_cache_incremental_1 = NULL;
static ssz_merkle_cache_t *g_cached_eph_list_cache_incremental_10 = NULL;

static bench_cached_eph_list_ctx_t g_cached_eph_list_ctx;
static ssz_member_codec_t g_cached_eph_list_codec;

static uint64_t g_cached_eph_incremental_1_tick = 1u;
static uint64_t g_cached_eph_incremental_10_tick = 1u;
static ssz_chunk_t g_bench_merkle_cache_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_bench_merkle_cache_scratch = {
    .chunks = g_bench_merkle_cache_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

static uint64_t bench_make_u64_value(uint64_t index, uint64_t salt)
{
    return UINT64_C(0x9E3779B97F4A7C15) ^ (index * UINT64_C(0x100000001B3)) ^ salt;
}

static void bench_fill_pattern(uint8_t *dst, size_t len, uint8_t seed, uint64_t item_index)
{
    uint8_t item_mix = (uint8_t)((item_index * 29u) & 0xFFu);
    for (size_t i = 0u; i < len; i++)
    {
        dst[i] = (uint8_t)(seed ^ item_mix ^ (uint8_t)(i * 11u));
    }
}

static ssz_error_t bench_create_list_cache(ssz_merkle_cache_t **out_cache)
{
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = CACHE_LIST_CHUNK_COUNT,
        .leaf_limit = CACHE_LIST_CHUNK_COUNT,
        .logical_length = CACHE_LIST_U64_COUNT,
        .mix_in_length = true,
        .hash_fn = NULL,
    };

    return ssz_merkle_cache_create(&cfg, out_cache);
}

static ssz_chunk_t bench_u64_chunk_from_values(const uint64_t *values, uint64_t chunk_index)
{
    ssz_chunk_t chunk;
    const uint8_t *bytes = (const uint8_t *)values;
    size_t offset = (size_t)chunk_index * SSZ_BYTES_PER_CHUNK;
    memcpy(chunk.bytes, bytes + offset, SSZ_BYTES_PER_CHUNK);
    return chunk;
}

static ssz_error_t bench_apply_incremental_updates(size_t update_count)
{
    for (size_t i = 0u; i < update_count; i++)
    {
        uint64_t element_index = g_incremental_indices[i];
        uint64_t mix = UINT64_C(0xD6E8FEB86659FD93) ^ (g_incremental_update_tick + (uint64_t)i);
        uint64_t chunk_index = element_index / CACHE_LIST_U64S_PER_CHUNK;
        ssz_chunk_t leaf;

        g_list_values_incremental[element_index] ^= mix;
        leaf = bench_u64_chunk_from_values(g_list_values_incremental, chunk_index);

        {
            ssz_error_t err = ssz_merkle_cache_update_root_range(g_cache_incremental, chunk_index, &leaf, 1u);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
        }
    }

    g_incremental_update_tick++;
    return SSZ_SUCCESS;
}

static ssz_error_t bench_composite_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const bench_composite_ctx_t *composite = (const bench_composite_ctx_t *)ctx;

    if ((composite == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((member_id >= composite->count) || (composite->roots == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_root = composite->roots[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t bench_composite_token(const void *ctx, uint64_t member_id, uint64_t *out_token)
{
    const bench_composite_ctx_t *composite = (const bench_composite_ctx_t *)ctx;

    if ((composite == NULL) || (out_token == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((member_id >= composite->count) || (composite->tokens == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_token = composite->tokens[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t bench_composite_root_batch(
    const void *ctx,
    uint64_t start_index,
    uint64_t count,
    ssz_chunk_t *out_roots)
{
    const bench_composite_ctx_t *composite = (const bench_composite_ctx_t *)ctx;

    if ((composite == NULL) || (out_roots == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((start_index > composite->count) || (count > (composite->count - start_index)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (count == 0u)
    {
        return SSZ_SUCCESS;
    }

    memcpy(out_roots, &composite->roots[start_index], (size_t)count * sizeof(*out_roots));
    return SSZ_SUCCESS;
}

static void bench_mutate_composite_field(void)
{
    uint64_t field_index = g_composite_update_tick % CACHE_COMPOSITE_FIELD_COUNT;
    uint8_t mix = (uint8_t)(g_composite_update_tick & 0xFFu);

    g_composite_roots[field_index].bytes[0] ^= (uint8_t)(0xA5u ^ mix);
    g_composite_roots[field_index].bytes[15] ^= (uint8_t)(0x3Cu + mix);
    g_composite_roots[field_index].bytes[31] ^= (uint8_t)(0x5Fu - (mix & 0x1Fu));
    g_composite_tokens[field_index] ^= (UINT64_C(0x9E3779B97F4A7C15) + g_composite_update_tick);

    g_composite_update_tick++;
}

static ssz_error_t bench_create_cached_eph_element_cache(ssz_merkle_cache_t **out_cache)
{
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = CACHE_EPH_FIELD_COUNT,
        .leaf_limit = CACHE_EPH_FIELD_COUNT,
        .logical_length = CACHE_EPH_FIELD_COUNT,
        .mix_in_length = false,
        .hash_fn = NULL,
    };

    return ssz_merkle_cache_create(&cfg, out_cache);
}

static ssz_error_t bench_create_cached_eph_list_cache(ssz_merkle_cache_t **out_cache)
{
    const ssz_merkle_cache_config_t cfg = {
        .initial_leaf_count = CACHE_EPH_COUNT,
        .leaf_limit = CACHE_EPH_LIMIT,
        .logical_length = CACHE_EPH_COUNT,
        .mix_in_length = true,
        .hash_fn = NULL,
    };

    return ssz_merkle_cache_create(&cfg, out_cache);
}

static void bench_init_cached_eph(bench_cached_eph_t *header, uint64_t item_index)
{
    bench_fill_pattern(header->parent_hash, sizeof(header->parent_hash), 0x10u, item_index);
    bench_fill_pattern(header->fee_recipient, sizeof(header->fee_recipient), 0x21u, item_index);
    bench_fill_pattern(header->state_root, sizeof(header->state_root), 0x32u, item_index);
    bench_fill_pattern(header->receipts_root, sizeof(header->receipts_root), 0x43u, item_index);
    bench_fill_pattern(header->logs_bloom, sizeof(header->logs_bloom), 0x54u, item_index);
    bench_fill_pattern(header->prev_randao, sizeof(header->prev_randao), 0x65u, item_index);
    bench_fill_pattern(header->block_number, sizeof(header->block_number), 0x76u, item_index);
    bench_fill_pattern(header->gas_limit, sizeof(header->gas_limit), 0x87u, item_index);
    bench_fill_pattern(header->gas_used, sizeof(header->gas_used), 0x98u, item_index);
    bench_fill_pattern(header->timestamp, sizeof(header->timestamp), 0xA9u, item_index);
    bench_fill_pattern(header->extra_data, sizeof(header->extra_data), 0xBAu, item_index);
    bench_fill_pattern(header->base_fee_per_gas, sizeof(header->base_fee_per_gas), 0xCBu, item_index);
    bench_fill_pattern(header->block_hash, sizeof(header->block_hash), 0xDCu, item_index);
    bench_fill_pattern(header->transactions_root, sizeof(header->transactions_root), 0xEDu, item_index);
    bench_fill_pattern(header->withdrawals_root, sizeof(header->withdrawals_root), 0xFEu, item_index);
    bench_fill_pattern(header->blob_gas_used, sizeof(header->blob_gas_used), 0x1Fu, item_index);
    bench_fill_pattern(header->excess_blob_gas, sizeof(header->excess_blob_gas), 0x2Eu, item_index);
}

static ssz_error_t bench_cached_eph_hash_tree_root_fixed_bytes(
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    return ssz_hash_tree_root_vector_fixed(
        bytes, (uint64_t)byte_len, 1u, &g_bench_merkle_cache_scratch, NULL, out_root);
}

static ssz_error_t bench_cached_eph_field_root(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    const bench_cached_eph_t *header = (const bench_cached_eph_t *)ctx;
    if ((header == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    switch (member_id)
    {
    case 0u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->parent_hash, sizeof(header->parent_hash), out_root);
    case 1u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->fee_recipient, sizeof(header->fee_recipient), out_root);
    case 2u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->state_root, sizeof(header->state_root), out_root);
    case 3u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(
            header->receipts_root, sizeof(header->receipts_root), out_root);
    case 4u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->logs_bloom, sizeof(header->logs_bloom), out_root);
    case 5u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->prev_randao, sizeof(header->prev_randao), out_root);
    case 6u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->block_number, sizeof(header->block_number), out_root);
    case 7u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->gas_limit, sizeof(header->gas_limit), out_root);
    case 8u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->gas_used, sizeof(header->gas_used), out_root);
    case 9u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->timestamp, sizeof(header->timestamp), out_root);
    case CACHE_EPH_EXTRA_DATA_FIELD_ID:
        return ssz_hash_tree_root_list_fixed(header->extra_data,
                                             (uint64_t)sizeof(header->extra_data),
                                             CACHE_EPH_EXTRA_DATA_MAX_LEN,
                                             1u,
                                             &g_bench_merkle_cache_scratch,
                                             NULL,
                                             out_root);
    case 11u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(
            header->base_fee_per_gas, sizeof(header->base_fee_per_gas), out_root);
    case 12u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->block_hash, sizeof(header->block_hash), out_root);
    case 13u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(
            header->transactions_root, sizeof(header->transactions_root), out_root);
    case 14u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(
            header->withdrawals_root, sizeof(header->withdrawals_root), out_root);
    case 15u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(header->blob_gas_used, sizeof(header->blob_gas_used), out_root);
    case 16u:
        return bench_cached_eph_hash_tree_root_fixed_bytes(
            header->excess_blob_gas, sizeof(header->excess_blob_gas), out_root);
    default:
        return SSZ_ERR_INVALID_ARGUMENT;
    }
}

static ssz_error_t bench_cached_eph_list_member_root(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    const bench_cached_eph_list_ctx_t *list_ctx = (const bench_cached_eph_list_ctx_t *)ctx;

    if ((list_ctx == NULL) || (out_root == NULL) || (list_ctx->headers == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id >= list_ctx->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    const bench_cached_eph_t *header = &list_ctx->headers[member_id];
    const ssz_member_codec_t field_codec = (ssz_member_codec_t){
        .ctx = (void *)header,
        .write = NULL,
        .read = NULL,
        .root = bench_cached_eph_field_root,
    };

    return ssz_hash_tree_root_vector_composite(
        CACHE_EPH_FIELD_COUNT, &field_codec, &g_bench_merkle_cache_scratch, NULL, out_root);
}

static ssz_error_t bench_cached_eph_get_field_bytes(
    bench_cached_eph_t *header,
    uint64_t field_id,
    uint8_t **out_bytes,
    size_t *out_len)
{
    if ((header == NULL) || (out_bytes == NULL) || (out_len == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    switch (field_id)
    {
    case 0u:
        *out_bytes = header->parent_hash;
        *out_len = sizeof(header->parent_hash);
        return SSZ_SUCCESS;
    case 1u:
        *out_bytes = header->fee_recipient;
        *out_len = sizeof(header->fee_recipient);
        return SSZ_SUCCESS;
    case 2u:
        *out_bytes = header->state_root;
        *out_len = sizeof(header->state_root);
        return SSZ_SUCCESS;
    case 3u:
        *out_bytes = header->receipts_root;
        *out_len = sizeof(header->receipts_root);
        return SSZ_SUCCESS;
    case 4u:
        *out_bytes = header->logs_bloom;
        *out_len = sizeof(header->logs_bloom);
        return SSZ_SUCCESS;
    case 5u:
        *out_bytes = header->prev_randao;
        *out_len = sizeof(header->prev_randao);
        return SSZ_SUCCESS;
    case 6u:
        *out_bytes = header->block_number;
        *out_len = sizeof(header->block_number);
        return SSZ_SUCCESS;
    case 7u:
        *out_bytes = header->gas_limit;
        *out_len = sizeof(header->gas_limit);
        return SSZ_SUCCESS;
    case 8u:
        *out_bytes = header->gas_used;
        *out_len = sizeof(header->gas_used);
        return SSZ_SUCCESS;
    case 9u:
        *out_bytes = header->timestamp;
        *out_len = sizeof(header->timestamp);
        return SSZ_SUCCESS;
    case CACHE_EPH_EXTRA_DATA_FIELD_ID:
        *out_bytes = header->extra_data;
        *out_len = sizeof(header->extra_data);
        return SSZ_SUCCESS;
    case 11u:
        *out_bytes = header->base_fee_per_gas;
        *out_len = sizeof(header->base_fee_per_gas);
        return SSZ_SUCCESS;
    case 12u:
        *out_bytes = header->block_hash;
        *out_len = sizeof(header->block_hash);
        return SSZ_SUCCESS;
    case 13u:
        *out_bytes = header->transactions_root;
        *out_len = sizeof(header->transactions_root);
        return SSZ_SUCCESS;
    case 14u:
        *out_bytes = header->withdrawals_root;
        *out_len = sizeof(header->withdrawals_root);
        return SSZ_SUCCESS;
    case 15u:
        *out_bytes = header->blob_gas_used;
        *out_len = sizeof(header->blob_gas_used);
        return SSZ_SUCCESS;
    case 16u:
        *out_bytes = header->excess_blob_gas;
        *out_len = sizeof(header->excess_blob_gas);
        return SSZ_SUCCESS;
    default:
        return SSZ_ERR_INVALID_ARGUMENT;
    }
}

static ssz_error_t bench_cached_eph_mutate_field(
    bench_cached_eph_t *header,
    uint64_t field_id,
    uint64_t tick)
{
    uint8_t *bytes = NULL;
    size_t byte_len = 0u;
    uint8_t mix = (uint8_t)(tick & 0xFFu);
    ssz_error_t err = bench_cached_eph_get_field_bytes(header, field_id, &bytes, &byte_len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    if ((bytes == NULL) || (byte_len == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    bytes[0] ^= (uint8_t)(0xA5u ^ mix);
    bytes[byte_len / 2u] ^= (uint8_t)(0x3Cu + (mix & 0x3Fu));
    bytes[byte_len - 1u] ^= (uint8_t)(0x5Fu - (mix & 0x1Fu));
    return SSZ_SUCCESS;
}

static ssz_error_t bench_cached_eph_refresh_element_root(
    bench_cached_eph_t *headers,
    ssz_merkle_cache_t **element_caches,
    ssz_merkle_cache_t *list_cache,
    uint64_t element_index,
    uint64_t field_id)
{
    ssz_chunk_t field_root;
    ssz_chunk_t element_root;
    ssz_error_t err = SSZ_SUCCESS;

    if ((headers == NULL) || (element_caches == NULL) || (list_cache == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((element_index >= CACHE_EPH_COUNT) || (field_id >= CACHE_EPH_FIELD_COUNT))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_caches[element_index] == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = bench_cached_eph_field_root(&headers[element_index], field_id, &field_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkle_cache_update_root_range(element_caches[element_index], field_id, &field_root, 1u);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkle_cache_data_root(element_caches[element_index], &element_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_merkle_cache_update_root_range(list_cache, element_index, &element_root, 1u);
}

static ssz_error_t bench_cached_eph_apply_update(
    bench_cached_eph_t *headers,
    ssz_merkle_cache_t **element_caches,
    ssz_merkle_cache_t *list_cache,
    uint64_t element_index,
    uint64_t field_id,
    uint64_t tick)
{
    ssz_error_t err = bench_cached_eph_mutate_field(&headers[element_index], field_id, tick);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return bench_cached_eph_refresh_element_root(
        headers, element_caches, list_cache, element_index, field_id);
}

static ssz_error_t bench_cached_eph_full_build(
    bench_cached_eph_t *headers,
    ssz_merkle_cache_t **element_caches,
    ssz_merkle_cache_t **out_list_cache)
{
    ssz_chunk_t field_roots[CACHE_EPH_FIELD_COUNT];
    ssz_chunk_t element_roots[CACHE_EPH_COUNT];
    ssz_error_t err = SSZ_SUCCESS;

    if ((headers == NULL) || (element_caches == NULL) || (out_list_cache == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint64_t i = 0u; i < CACHE_EPH_COUNT; i++)
    {
        err = bench_create_cached_eph_element_cache(&element_caches[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }

        for (uint64_t field_id = 0u; field_id < CACHE_EPH_FIELD_COUNT; field_id++)
        {
            err = bench_cached_eph_field_root(&headers[i], field_id, &field_roots[field_id]);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
        }

        err = ssz_merkle_cache_update_root_range(
            element_caches[i], 0u, field_roots, CACHE_EPH_FIELD_COUNT);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }

        err = ssz_merkle_cache_data_root(element_caches[i], &element_roots[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    err = bench_create_cached_eph_list_cache(out_list_cache);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkle_cache_update_root_range(*out_list_cache, 0u, element_roots, CACHE_EPH_COUNT);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkle_cache_set_logical_length(*out_list_cache, CACHE_EPH_COUNT);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    {
        ssz_chunk_t warm_root;
        err = ssz_merkle_cache_root(*out_list_cache, &warm_root);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t bench_cached_eph_verify_root_equivalence(
    const bench_cached_eph_t *headers,
    ssz_merkle_cache_t *list_cache)
{
    bench_cached_eph_list_ctx_t list_ctx;
    ssz_member_codec_t list_codec;
    ssz_chunk_t stateless_root;
    ssz_chunk_t cached_root;
    ssz_error_t err = SSZ_SUCCESS;

    if ((headers == NULL) || (list_cache == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    list_ctx = (bench_cached_eph_list_ctx_t){
        .headers = headers,
        .count = CACHE_EPH_COUNT,
    };
    list_codec = (ssz_member_codec_t){
        .ctx = &list_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_cached_eph_list_member_root,
    };

    err = ssz_hash_tree_root_list_composite(
        CACHE_EPH_COUNT, CACHE_EPH_LIMIT, &list_codec, &g_bench_merkle_cache_scratch, NULL, &stateless_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_merkle_cache_root(list_cache, &cached_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (memcmp(cached_root.bytes, stateless_root.bytes, SSZ_BYTES_PER_CHUNK) != 0)
    {
        return SSZ_ERR_HASH_FAILURE;
    }

    return SSZ_SUCCESS;
}

static void bench_init_merkle_cache_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    if ((CACHE_LIST_U64_COUNT % CACHE_LIST_U64S_PER_CHUNK) != 0u)
    {
        g_init_state = -1;
        return;
    }

    for (uint64_t i = 0u; i < CACHE_LIST_U64_COUNT; i++)
    {
        g_list_values_a[i] = bench_make_u64_value(i, UINT64_C(0xA5A5A5A5A5A5A5A5));
        g_list_values_b[i] = bench_make_u64_value(i, UINT64_C(0x5A5A5A5A5A5A5A5A));
        g_list_values_incremental[i] = g_list_values_a[i];
    }

    for (size_t i = 0u; i < CACHE_INCREMENTAL_UPDATE_1000; i++)
    {
        uint64_t chunk_index = ((uint64_t)i * UINT64_C(9973) + UINT64_C(17)) % CACHE_LIST_CHUNK_COUNT;
        g_incremental_indices[i] = chunk_index * CACHE_LIST_U64S_PER_CHUNK;
    }

    {
        ssz_error_t err = bench_create_list_cache(&g_cache_full_build);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    {
        ssz_error_t err = bench_create_list_cache(&g_cache_incremental);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        err = ssz_merkle_cache_sync_packed_list_fixed(g_cache_incremental,
                                                      (const uint8_t *)g_list_values_incremental,
                                                      CACHE_LIST_U64_COUNT,
                                                      CACHE_LIST_U64_LIMIT,
                                                      CACHE_LIST_U64_ELEMENT_SIZE);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        {
            ssz_chunk_t root;
            err = ssz_merkle_cache_data_root(g_cache_incremental, &root);
            if (err != SSZ_SUCCESS)
            {
                g_init_state = -1;
                return;
            }
        }
    }

    {
        ssz_error_t err = bench_create_list_cache(&g_cache_unchanged);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        err = ssz_merkle_cache_sync_packed_list_fixed(g_cache_unchanged,
                                                      (const uint8_t *)g_list_values_a,
                                                      CACHE_LIST_U64_COUNT,
                                                      CACHE_LIST_U64_LIMIT,
                                                      CACHE_LIST_U64_ELEMENT_SIZE);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        {
            ssz_chunk_t root;
            err = ssz_merkle_cache_data_root(g_cache_unchanged, &root);
            if (err != SSZ_SUCCESS)
            {
                g_init_state = -1;
                return;
            }
        }
    }

    for (uint64_t i = 0u; i < CACHE_COMPOSITE_FIELD_COUNT; i++)
    {
        g_composite_tokens[i] = UINT64_C(0x1000000000000000) + i;
        for (size_t j = 0u; j < SSZ_BYTES_PER_CHUNK; j++)
        {
            g_composite_roots[i].bytes[j] = (uint8_t)(0x11u + (uint8_t)(i * 7u) + (uint8_t)(j * 3u));
        }
    }

    g_composite_ctx = (bench_composite_ctx_t){
        .roots = g_composite_roots,
        .tokens = g_composite_tokens,
        .count = CACHE_COMPOSITE_FIELD_COUNT,
    };
    g_composite_codec = (ssz_member_codec_t){
        .ctx = &g_composite_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_composite_root,
    };
    g_composite_opts = (ssz_merkle_cache_sync_composite_opts_t){
        .ctx = &g_composite_ctx,
        .token = bench_composite_token,
        .root_batch = bench_composite_root_batch,
    };

    {
        const ssz_merkle_cache_config_t composite_cfg = {
            .initial_leaf_count = CACHE_COMPOSITE_FIELD_COUNT,
            .leaf_limit = CACHE_COMPOSITE_FIELD_COUNT,
            .logical_length = CACHE_COMPOSITE_FIELD_COUNT,
            .mix_in_length = false,
            .hash_fn = NULL,
        };
        ssz_error_t err = ssz_merkle_cache_create(&composite_cfg, &g_cache_composite);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        err = ssz_merkle_cache_sync_composite(g_cache_composite,
                                              CACHE_COMPOSITE_FIELD_COUNT,
                                              CACHE_COMPOSITE_FIELD_COUNT,
                                              &g_composite_codec,
                                              &g_composite_opts);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }

        {
            ssz_chunk_t root;
            err = ssz_merkle_cache_data_root(g_cache_composite, &root);
            if (err != SSZ_SUCCESS)
            {
                g_init_state = -1;
                return;
            }
        }
    }

    for (uint64_t i = 0u; i < CACHE_EPH_COUNT; i++)
    {
        bench_init_cached_eph(&g_cached_eph_headers_baseline[i], i);
    }

    memcpy(g_cached_eph_headers_incremental_1,
           g_cached_eph_headers_baseline,
           sizeof(g_cached_eph_headers_incremental_1));
    memcpy(g_cached_eph_headers_incremental_10,
           g_cached_eph_headers_baseline,
           sizeof(g_cached_eph_headers_incremental_10));

    g_cached_eph_list_ctx = (bench_cached_eph_list_ctx_t){
        .headers = g_cached_eph_headers_baseline,
        .count = CACHE_EPH_COUNT,
    };
    g_cached_eph_list_codec = (ssz_member_codec_t){
        .ctx = &g_cached_eph_list_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_cached_eph_list_member_root,
    };

    {
        ssz_error_t err = bench_cached_eph_full_build(
            g_cached_eph_headers_incremental_1,
            g_cached_eph_element_caches_incremental_1,
            &g_cached_eph_list_cache_incremental_1);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    {
        ssz_error_t err = bench_cached_eph_full_build(
            g_cached_eph_headers_incremental_10,
            g_cached_eph_element_caches_incremental_10,
            &g_cached_eph_list_cache_incremental_10);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    {
        ssz_error_t err = bench_cached_eph_verify_root_equivalence(
            g_cached_eph_headers_incremental_1, g_cached_eph_list_cache_incremental_1);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    {
        ssz_error_t err = bench_cached_eph_verify_root_equivalence(
            g_cached_eph_headers_incremental_10, g_cached_eph_list_cache_incremental_10);
        if (err != SSZ_SUCCESS)
        {
            g_init_state = -1;
            return;
        }
    }

    g_init_state = 1;
}

UBENCH(merkle_cache, stateless_list_u64_1m_hash_tree_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_fixed((const uint8_t *)g_list_values_a,
                                                  CACHE_LIST_U64_COUNT,
                                                  CACHE_LIST_U64_LIMIT,
                                                  CACHE_LIST_U64_ELEMENT_SIZE,
                                                  &g_bench_merkle_cache_scratch,
                                                  NULL,
                                                  &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_full_build_all_dirty)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    const uint64_t *source = (g_full_build_toggle == 0u) ? g_list_values_a : g_list_values_b;
    ssz_chunk_t root;

    g_full_build_toggle ^= 1u;

    BENCH_EXPECT_OK(ssz_merkle_cache_sync_packed_list_fixed(g_cache_full_build,
                                                            (const uint8_t *)source,
                                                            CACHE_LIST_U64_COUNT,
                                                            CACHE_LIST_U64_LIMIT,
                                                            CACHE_LIST_U64_ELEMENT_SIZE));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_full_build, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_incremental_1_leaf_then_data_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(bench_apply_incremental_updates(CACHE_INCREMENTAL_UPDATE_1));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_incremental, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_incremental_10_leaves_then_data_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(bench_apply_incremental_updates(CACHE_INCREMENTAL_UPDATE_10));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_incremental, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_incremental_100_leaves_then_data_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(bench_apply_incremental_updates(CACHE_INCREMENTAL_UPDATE_100));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_incremental, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_incremental_1000_leaves_then_data_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(bench_apply_incremental_updates(CACHE_INCREMENTAL_UPDATE_1000));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_incremental, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_list_u64_1m_unchanged_data_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    for (size_t i = 0u; i < CACHE_UNCHANGED_QUERY_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_unchanged, &root));
        ubench_do_nothing((void *)&root);
    }
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, stateless_container_17_hash_tree_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_vector_composite(
        CACHE_COMPOSITE_FIELD_COUNT, &g_composite_codec, &g_bench_merkle_cache_scratch, NULL, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_container_17_sync_composite_incremental)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    bench_mutate_composite_field();

    BENCH_EXPECT_OK(ssz_merkle_cache_sync_composite(g_cache_composite,
                                                    CACHE_COMPOSITE_FIELD_COUNT,
                                                    CACHE_COMPOSITE_FIELD_COUNT,
                                                    &g_composite_codec,
                                                    &g_composite_opts));
    BENCH_EXPECT_OK(ssz_merkle_cache_data_root(g_cache_composite, &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, stateless_execution_payload_header_list_1000_hash_tree_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_composite(CACHE_EPH_COUNT,
                                                      CACHE_EPH_LIMIT,
                                                      &g_cached_eph_list_codec,
                                                      &g_bench_merkle_cache_scratch,
                                                      NULL,
                                                      &root));
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_execution_payload_header_list_1000_incremental_1_field_then_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t element_index = (g_cached_eph_incremental_1_tick * UINT64_C(131) + UINT64_C(17)) % CACHE_EPH_COUNT;
    uint64_t field_id = (g_cached_eph_incremental_1_tick * UINT64_C(7) + UINT64_C(3)) % CACHE_EPH_FIELD_COUNT;
    ssz_chunk_t root;

    BENCH_EXPECT_OK(bench_cached_eph_apply_update(g_cached_eph_headers_incremental_1,
                                                  g_cached_eph_element_caches_incremental_1,
                                                  g_cached_eph_list_cache_incremental_1,
                                                  element_index,
                                                  field_id,
                                                  g_cached_eph_incremental_1_tick));
    BENCH_EXPECT_OK(ssz_merkle_cache_root(g_cached_eph_list_cache_incremental_1, &root));

    g_cached_eph_incremental_1_tick++;
    ubench_do_nothing((void *)&root);
}

UBENCH(merkle_cache, cached_execution_payload_header_list_1000_incremental_10_fields_then_root)
{
    bench_init_merkle_cache_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t base_index = (g_cached_eph_incremental_10_tick * UINT64_C(131) + UINT64_C(29)) % CACHE_EPH_COUNT;
    ssz_chunk_t root;

    for (uint64_t i = 0u; i < CACHE_EPH_INCREMENTAL_BATCH_10; i++)
    {
        uint64_t element_index = (base_index + (i * UINT64_C(97))) % CACHE_EPH_COUNT;
        uint64_t field_id = (g_cached_eph_incremental_10_tick + (i * UINT64_C(5))) % CACHE_EPH_FIELD_COUNT;
        BENCH_EXPECT_OK(bench_cached_eph_apply_update(g_cached_eph_headers_incremental_10,
                                                      g_cached_eph_element_caches_incremental_10,
                                                      g_cached_eph_list_cache_incremental_10,
                                                      element_index,
                                                      field_id,
                                                      g_cached_eph_incremental_10_tick + i));
    }

    BENCH_EXPECT_OK(ssz_merkle_cache_root(g_cached_eph_list_cache_incremental_10, &root));

    g_cached_eph_incremental_10_tick += CACHE_EPH_INCREMENTAL_BATCH_10;
    ubench_do_nothing((void *)&root);
}

UBENCH_MAIN();
