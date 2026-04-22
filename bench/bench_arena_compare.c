#include "ubench.h"
#include "ssz.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ARENA_LIST_U64_COUNT        1000000u
#define ARENA_LIST_U64_ELEMENT_SIZE 8u
#define ARENA_LIST_U64_BYTES        (ARENA_LIST_U64_COUNT * ARENA_LIST_U64_ELEMENT_SIZE)
#define ARENA_LIST_U64_LIMIT        UINT64_C(1099511627776)

#define ARENA_EXEC_HEADER_COUNT 1000u
#define ARENA_EXEC_HEADER_LIMIT UINT64_C(1099511627776)

#define ARENA_EXEC_HEADER_FIELD_COUNT          17u
#define ARENA_EXEC_HEADER_EXTRA_DATA_FIELD_ID  10u
#define ARENA_EXEC_HEADER_EXTRA_DATA_LEN       16u
#define ARENA_EXEC_HEADER_EXTRA_DATA_MAX_LEN   32u
#define ARENA_EXEC_HEADER_MIN_ENCODED_LEN      584u
#define ARENA_EXEC_HEADER_EXPECTED_ENCODED_LEN 600u

#define ARENA_EXEC_HEADER_LIST_ENCODED_CAP                                                            \
    (ARENA_EXEC_HEADER_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + ARENA_EXEC_HEADER_EXPECTED_ENCODED_LEN))

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
    uint8_t extra_data[ARENA_EXEC_HEADER_EXTRA_DATA_LEN];
    uint8_t base_fee_per_gas[32];
    uint8_t block_hash[32];
    uint8_t transactions_root[32];
    uint8_t withdrawals_root[32];
    uint8_t blob_gas_used[8];
    uint8_t excess_blob_gas[8];
} bench_execution_payload_header_t;

typedef struct
{
    const bench_execution_payload_header_t *headers;
    uint64_t count;
} bench_exec_header_list_write_ctx_t;

typedef struct
{
    uint64_t callback_count;
    uint64_t byte_acc;
} bench_read_sink_t;

typedef struct
{
    bench_read_sink_t *sink;
    ssz_member_codec_t container_read_codec;
} bench_exec_header_list_read_ctx_t;

static int g_init_state = 0;

static uint64_t g_list_u64_values[ARENA_LIST_U64_COUNT];
static uint8_t g_list_u64_encoded[ARENA_LIST_U64_BYTES];
static uint8_t g_list_u64_decoded[ARENA_LIST_U64_BYTES];
static size_t g_list_u64_encoded_len = 0u;

static const size_t g_exec_header_field_fixed_sizes[ARENA_EXEC_HEADER_FIELD_COUNT] = {
    32u,
    20u,
    32u,
    32u,
    256u,
    32u,
    8u,
    8u,
    8u,
    8u,
    0u,
    32u,
    32u,
    32u,
    32u,
    8u,
    8u,
};
static const ssz_container_schema_t g_exec_header_schema =
    SSZ_CONTAINER_SCHEMA_FROM_ARRAY(g_exec_header_field_fixed_sizes);

static bench_execution_payload_header_t g_exec_headers[ARENA_EXEC_HEADER_COUNT];
static bench_exec_header_list_write_ctx_t g_exec_header_list_write_ctx;
static ssz_member_codec_t g_exec_header_list_write_codec;

static uint8_t g_exec_header_list_encoded[ARENA_EXEC_HEADER_LIST_ENCODED_CAP];
static size_t g_exec_header_list_encoded_len = 0u;

static bench_read_sink_t g_exec_header_read_sink;
static bench_exec_header_list_read_ctx_t g_exec_header_list_read_ctx;
static ssz_member_codec_t g_exec_header_list_read_codec;
static ssz_member_codec_t g_exec_header_list_hash_codec;

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

static ssz_chunk_t g_bench_arena_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_bench_arena_scratch = {
    .chunks = g_bench_arena_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

static void bench_fill_pattern(uint8_t *dst, size_t len, uint8_t seed, uint64_t item_index)
{
    uint8_t item_mix = (uint8_t)((item_index * 29u) & 0xFFu);
    for (size_t i = 0u; i < len; i++)
    {
        dst[i] = (uint8_t)(seed ^ item_mix ^ (uint8_t)(i * 11u));
    }
}

static void bench_init_execution_payload_header(
    bench_execution_payload_header_t *header,
    uint64_t item_index)
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

static ssz_error_t bench_exec_header_list_member_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const bench_exec_header_list_write_ctx_t *write_ctx = (const bench_exec_header_list_write_ctx_t *)ctx;
    if ((write_ctx == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id >= write_ctx->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    const bench_execution_payload_header_t *header = &write_ctx->headers[member_id];
    const size_t fixed_prefix_len = offsetof(bench_execution_payload_header_t, extra_data);
    const size_t fixed_suffix_src = offsetof(bench_execution_payload_header_t, base_fee_per_gas);
    const size_t fixed_suffix_len = sizeof(header->base_fee_per_gas) + sizeof(header->block_hash) +
                                    sizeof(header->transactions_root) + sizeof(header->withdrawals_root) +
                                    sizeof(header->blob_gas_used) + sizeof(header->excess_blob_gas);
    const size_t total_len = ARENA_EXEC_HEADER_MIN_ENCODED_LEN + sizeof(header->extra_data);

    *out_written = total_len;
    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < total_len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(out, (const uint8_t *)header, fixed_prefix_len);
    ssz_error_t err = ssz_serialize_uint32((uint32_t)ARENA_EXEC_HEADER_MIN_ENCODED_LEN, out + fixed_prefix_len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    memcpy(out + fixed_prefix_len + SSZ_BYTES_PER_LENGTH_OFFSET, (const uint8_t *)header + fixed_suffix_src, fixed_suffix_len);
    memcpy(out + ARENA_EXEC_HEADER_MIN_ENCODED_LEN, header->extra_data, sizeof(header->extra_data));

    return SSZ_SUCCESS;
}

static ssz_error_t bench_exec_header_field_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    bench_read_sink_t *sink = (bench_read_sink_t *)ctx;
    if (sink == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id >= ARENA_EXEC_HEADER_FIELD_COUNT)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((member_id == ARENA_EXEC_HEADER_EXTRA_DATA_FIELD_ID) &&
        (data_len > ARENA_EXEC_HEADER_EXTRA_DATA_MAX_LEN))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    sink->callback_count++;
    sink->byte_acc += (uint64_t)data_len + member_id;
    if ((data_len != 0u) && (data != NULL))
    {
        sink->byte_acc += data[0];
    }

    return SSZ_SUCCESS;
}

static ssz_error_t bench_exec_header_list_member_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    bench_exec_header_list_read_ctx_t *read_ctx = (bench_exec_header_list_read_ctx_t *)ctx;
    if ((read_ctx == NULL) || (read_ctx->sink == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id >= ARENA_EXEC_HEADER_COUNT)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    read_ctx->sink->byte_acc += member_id;

    return ssz_deserialize_container(
        data, data_len, &g_exec_header_schema, &read_ctx->container_read_codec);
}

static ssz_error_t bench_hash_tree_root_fixed_bytes(
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    return ssz_hash_tree_root_vector_fixed(
        bytes, (uint64_t)byte_len, 1u, &g_bench_arena_scratch, NULL, out_root);
}

static ssz_error_t bench_exec_header_field_root(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    const bench_execution_payload_header_t *header = (const bench_execution_payload_header_t *)ctx;
    if ((header == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    switch (member_id)
    {
    case 0u:
        return bench_hash_tree_root_fixed_bytes(header->parent_hash, sizeof(header->parent_hash), out_root);
    case 1u:
        return bench_hash_tree_root_fixed_bytes(header->fee_recipient, sizeof(header->fee_recipient), out_root);
    case 2u:
        return bench_hash_tree_root_fixed_bytes(header->state_root, sizeof(header->state_root), out_root);
    case 3u:
        return bench_hash_tree_root_fixed_bytes(header->receipts_root, sizeof(header->receipts_root), out_root);
    case 4u:
        return bench_hash_tree_root_fixed_bytes(header->logs_bloom, sizeof(header->logs_bloom), out_root);
    case 5u:
        return bench_hash_tree_root_fixed_bytes(header->prev_randao, sizeof(header->prev_randao), out_root);
    case 6u:
        return bench_hash_tree_root_fixed_bytes(header->block_number, sizeof(header->block_number), out_root);
    case 7u:
        return bench_hash_tree_root_fixed_bytes(header->gas_limit, sizeof(header->gas_limit), out_root);
    case 8u:
        return bench_hash_tree_root_fixed_bytes(header->gas_used, sizeof(header->gas_used), out_root);
    case 9u:
        return bench_hash_tree_root_fixed_bytes(header->timestamp, sizeof(header->timestamp), out_root);
    case ARENA_EXEC_HEADER_EXTRA_DATA_FIELD_ID:
        return ssz_hash_tree_root_list_fixed(header->extra_data,
                                             (uint64_t)sizeof(header->extra_data),
                                             ARENA_EXEC_HEADER_EXTRA_DATA_MAX_LEN,
                                             1u,
                                             &g_bench_arena_scratch,
                                             NULL,
                                             out_root);
    case 11u:
        return bench_hash_tree_root_fixed_bytes(header->base_fee_per_gas, sizeof(header->base_fee_per_gas), out_root);
    case 12u:
        return bench_hash_tree_root_fixed_bytes(header->block_hash, sizeof(header->block_hash), out_root);
    case 13u:
        return bench_hash_tree_root_fixed_bytes(header->transactions_root, sizeof(header->transactions_root), out_root);
    case 14u:
        return bench_hash_tree_root_fixed_bytes(header->withdrawals_root, sizeof(header->withdrawals_root), out_root);
    case 15u:
        return bench_hash_tree_root_fixed_bytes(header->blob_gas_used, sizeof(header->blob_gas_used), out_root);
    case 16u:
        return bench_hash_tree_root_fixed_bytes(header->excess_blob_gas, sizeof(header->excess_blob_gas), out_root);
    default:
        return SSZ_ERR_INVALID_ARGUMENT;
    }
}

static ssz_error_t bench_exec_header_list_member_root(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    const bench_exec_header_list_write_ctx_t *hash_ctx = (const bench_exec_header_list_write_ctx_t *)ctx;
    if ((hash_ctx == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id >= hash_ctx->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    const bench_execution_payload_header_t *header = &hash_ctx->headers[member_id];
    const ssz_member_codec_t field_codec = (ssz_member_codec_t){
        .ctx = (void *)header,
        .write = NULL,
        .read = NULL,
        .root = bench_exec_header_field_root,
    };

    return ssz_hash_tree_root_vector_composite(
        ARENA_EXEC_HEADER_FIELD_COUNT, &field_codec, &g_bench_arena_scratch, NULL, out_root);
}

static void bench_init_arena_compare_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    for (uint64_t i = 0u; i < ARENA_LIST_U64_COUNT; i++)
    {
        g_list_u64_values[i] = UINT64_C(0x9E3779B97F4A7C15) ^ (i * UINT64_C(0x100000001B3));
    }

    ssz_error_t err = ssz_serialize_list_fixed((const uint8_t *)g_list_u64_values,
                                               ARENA_LIST_U64_COUNT,
                                               ARENA_LIST_U64_LIMIT,
                                               ARENA_LIST_U64_ELEMENT_SIZE,
                                               g_list_u64_encoded,
                                               sizeof(g_list_u64_encoded),
                                               &g_list_u64_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_list_u64_encoded_len != sizeof(g_list_u64_encoded)))
    {
        g_init_state = -1;
        return;
    }

    for (uint64_t i = 0u; i < ARENA_EXEC_HEADER_COUNT; i++)
    {
        bench_init_execution_payload_header(&g_exec_headers[i], i);
    }

    g_exec_header_list_write_ctx = (bench_exec_header_list_write_ctx_t){
        .headers = g_exec_headers,
        .count = ARENA_EXEC_HEADER_COUNT,
    };
    g_exec_header_list_write_codec = (ssz_member_codec_t){
        .ctx = &g_exec_header_list_write_ctx,
        .write = bench_exec_header_list_member_write,
        .read = NULL,
        .root = NULL,
    };
    g_exec_header_list_hash_codec = (ssz_member_codec_t){
        .ctx = &g_exec_header_list_write_ctx,
        .write = NULL,
        .read = NULL,
        .root = bench_exec_header_list_member_root,
    };

    err = ssz_serialize_list_variable(ARENA_EXEC_HEADER_COUNT,
                                      ARENA_EXEC_HEADER_LIMIT,
                                      &g_exec_header_list_write_codec,
                                      g_exec_header_list_encoded,
                                      sizeof(g_exec_header_list_encoded),
                                      &g_exec_header_list_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_exec_header_list_encoded_len > sizeof(g_exec_header_list_encoded)) ||
        (g_exec_header_list_encoded_len !=
         ARENA_EXEC_HEADER_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + ARENA_EXEC_HEADER_EXPECTED_ENCODED_LEN)))
    {
        g_init_state = -1;
        return;
    }

    g_exec_header_read_sink = (bench_read_sink_t){0u, 0u};
    g_exec_header_list_read_ctx = (bench_exec_header_list_read_ctx_t){
        .sink = &g_exec_header_read_sink,
        .container_read_codec =
            {
                .ctx = &g_exec_header_read_sink,
                .write = NULL,
                .read = bench_exec_header_field_read,
                .root = NULL,
            },
    };
    g_exec_header_list_read_codec = (ssz_member_codec_t){
        .ctx = &g_exec_header_list_read_ctx,
        .write = NULL,
        .read = bench_exec_header_list_member_read,
        .root = NULL,
    };

    uint64_t out_count = 0u;
    err = ssz_deserialize_list_variable(g_exec_header_list_encoded,
                                        g_exec_header_list_encoded_len,
                                        ARENA_EXEC_HEADER_LIMIT,
                                        ARENA_EXEC_HEADER_MIN_ENCODED_LEN,
                                        &g_exec_header_list_read_codec,
                                        &out_count);
    if ((err != SSZ_SUCCESS) || (out_count != ARENA_EXEC_HEADER_COUNT))
    {
        g_init_state = -1;
        return;
    }

    g_exec_header_read_sink.callback_count = 0u;
    g_exec_header_read_sink.byte_acc = 0u;
    g_init_state = 1;
}

UBENCH(arena_compare, list_u64_1m_encode)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_list_fixed((const uint8_t *)g_list_u64_values,
                                             ARENA_LIST_U64_COUNT,
                                             ARENA_LIST_U64_LIMIT,
                                             ARENA_LIST_U64_ELEMENT_SIZE,
                                             g_list_u64_encoded,
                                             sizeof(g_list_u64_encoded),
                                             &out_len));

    ubench_do_nothing(g_list_u64_encoded);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(arena_compare, list_u64_1m_decode)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t out_count = 0u;
    BENCH_EXPECT_OK(ssz_deserialize_list_fixed(g_list_u64_encoded,
                                               g_list_u64_encoded_len,
                                               ARENA_LIST_U64_LIMIT,
                                               ARENA_LIST_U64_ELEMENT_SIZE,
                                               g_list_u64_decoded,
                                               sizeof(g_list_u64_decoded),
                                               &out_count));

    ubench_do_nothing(g_list_u64_decoded);
    ubench_do_nothing((void *)&out_count);
}

UBENCH(arena_compare, list_u64_1m_hash_tree_root)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_fixed((const uint8_t *)g_list_u64_values,
                                                  ARENA_LIST_U64_COUNT,
                                                  ARENA_LIST_U64_LIMIT,
                                                  ARENA_LIST_U64_ELEMENT_SIZE,
                                                  &g_bench_arena_scratch,
                                                  NULL,
                                                  &root));

    ubench_do_nothing((void *)&root);
}

UBENCH(arena_compare, execution_payload_header_list_1000_encode)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_list_variable(ARENA_EXEC_HEADER_COUNT,
                                                ARENA_EXEC_HEADER_LIMIT,
                                                &g_exec_header_list_write_codec,
                                                g_exec_header_list_encoded,
                                                sizeof(g_exec_header_list_encoded),
                                                &out_len));

    ubench_do_nothing(g_exec_header_list_encoded);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(arena_compare, execution_payload_header_list_1000_decode)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_exec_header_read_sink.callback_count = 0u;
    g_exec_header_read_sink.byte_acc = 0u;

    uint64_t out_count = 0u;
    BENCH_EXPECT_OK(ssz_deserialize_list_variable(g_exec_header_list_encoded,
                                                  g_exec_header_list_encoded_len,
                                                  ARENA_EXEC_HEADER_LIMIT,
                                                  ARENA_EXEC_HEADER_MIN_ENCODED_LEN,
                                                  &g_exec_header_list_read_codec,
                                                  &out_count));

    ubench_do_nothing((void *)&g_exec_header_read_sink);
    ubench_do_nothing((void *)&out_count);
}

UBENCH(arena_compare, execution_payload_header_list_1000_hash_tree_root)
{
    bench_init_arena_compare_data();
    if (g_init_state != 1)
    {
        return;
    }

    ssz_chunk_t root;
    BENCH_EXPECT_OK(ssz_hash_tree_root_list_composite(ARENA_EXEC_HEADER_COUNT,
                                                      ARENA_EXEC_HEADER_LIMIT,
                                                      &g_exec_header_list_hash_codec,
                                                      &g_bench_arena_scratch,
                                                      NULL,
                                                      &root));

    ubench_do_nothing((void *)&root);
}

UBENCH_MAIN();
