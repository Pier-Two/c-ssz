#include "ubench.h"
#include "ssz.h"

#include <stdint.h>
#include <string.h>

#define DESER_VECTOR_FIXED_COUNT        1024u
#define DESER_VECTOR_FIXED_ELEMENT_SIZE 8u
#define DESER_VECTOR_FIXED_BYTES        (DESER_VECTOR_FIXED_COUNT * DESER_VECTOR_FIXED_ELEMENT_SIZE)

#define DESER_VECTOR_VARIABLE_COUNT   256u
#define DESER_VECTOR_VARIABLE_MIN_LEN 24u
#define DESER_VECTOR_VARIABLE_SPAN    16u
#define DESER_VECTOR_VARIABLE_MAX_LEN (DESER_VECTOR_VARIABLE_MIN_LEN + DESER_VECTOR_VARIABLE_SPAN - 1u)
#define DESER_VECTOR_VARIABLE_CAP                                                                      \
    (DESER_VECTOR_VARIABLE_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + DESER_VECTOR_VARIABLE_MAX_LEN))

#define DESER_LIST_FIXED_COUNT        1024u
#define DESER_LIST_FIXED_LIMIT        2048u
#define DESER_LIST_FIXED_ELEMENT_SIZE 32u
#define DESER_LIST_FIXED_BYTES        (DESER_LIST_FIXED_COUNT * DESER_LIST_FIXED_ELEMENT_SIZE)

#define DESER_LIST_VARIABLE_COUNT   192u
#define DESER_LIST_VARIABLE_LIMIT   512u
#define DESER_LIST_VARIABLE_MIN_LEN 16u
#define DESER_LIST_VARIABLE_SPAN    24u
#define DESER_LIST_VARIABLE_MAX_LEN (DESER_LIST_VARIABLE_MIN_LEN + DESER_LIST_VARIABLE_SPAN - 1u)
#define DESER_LIST_VARIABLE_CAP                                                                      \
    (DESER_LIST_VARIABLE_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + DESER_LIST_VARIABLE_MAX_LEN))

#define DESER_BITVECTOR_BIT_COUNT 8192u
#define DESER_BITVECTOR_BYTES     (DESER_BITVECTOR_BIT_COUNT / 8u)

#define DESER_BITLIST_BIT_LEN  8190u
#define DESER_BITLIST_BIT_MAX  16384u
#define DESER_BITLIST_IN_BYTES ((DESER_BITLIST_BIT_LEN + 7u) / 8u)

#define DESER_SMALL_BATCH 64u

#define DESER_UNION_OPTION_COUNT      3u
#define DESER_UNION_SELECTOR_PAYLOAD  2u
#define DESER_UNION_COMPAT_SELECTOR_A 2u
#define DESER_UNION_COMPAT_SELECTOR_B 7u

#define DESER_CONTAINER_FIELD_COUNT   7u
#define DESER_CONTAINER_VAR0_LEN      96u
#define DESER_CONTAINER_VAR1_LEN      128u
#define DESER_CONTAINER_OUTPUT_CAP    512u
#define DESER_CONTAINER_FIXED_REGION  (48u + 32u + 8u + 1u + 8u + 4u + 4u)
#define DESER_CONTAINER_EXPECTED_SIZE                                                               \
    (DESER_CONTAINER_FIXED_REGION + DESER_CONTAINER_VAR0_LEN + DESER_CONTAINER_VAR1_LEN)

typedef struct {
    size_t min_len;
    size_t span;
    uint8_t seed;
} bench_var_payload_ctx_t;

typedef struct {
    uint64_t id;
    const uint8_t *data;
    size_t len;
} bench_payload_entry_t;

typedef struct {
    const bench_payload_entry_t *entries;
    size_t entry_count;
} bench_payload_map_t;

typedef struct {
    uint64_t callback_count;
    uint64_t byte_acc;
} bench_read_sink_t;

static size_t bench_payload_len_for_member(const bench_var_payload_ctx_t *ctx, uint64_t member_id)
{
    size_t span = (ctx->span == 0u) ? 1u : ctx->span;
    return ctx->min_len + (size_t)(member_id % span);
}

static ssz_error_t bench_variable_payload_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const bench_var_payload_ctx_t *cfg = (const bench_var_payload_ctx_t *)ctx;
    if ((cfg == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    size_t len = bench_payload_len_for_member(cfg, member_id);
    *out_written = len;

    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    uint8_t member_mix = (uint8_t)(member_id & 0xFFu);
    for (size_t i = 0u; i < len; i++)
    {
        out[i] = (uint8_t)(cfg->seed ^ member_mix ^ (uint8_t)(i * 11u));
    }

    return SSZ_SUCCESS;
}

static ssz_error_t bench_payload_map_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const bench_payload_map_t *map = (const bench_payload_map_t *)ctx;

    if ((map == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < map->entry_count; i++)
    {
        if (map->entries[i].id != member_id)
        {
            continue;
        }

        *out_written = map->entries[i].len;
        if (out == NULL)
        {
            return SSZ_SUCCESS;
        }

        if (out_cap < map->entries[i].len)
        {
            return SSZ_ERR_BUFFER_TOO_SMALL;
        }

        if ((map->entries[i].len != 0u) && (map->entries[i].data != NULL))
        {
            memcpy(out, map->entries[i].data, map->entries[i].len);
        }

        return SSZ_SUCCESS;
    }

    return SSZ_ERR_INVALID_ARGUMENT;
}

static ssz_error_t bench_read_sink_callback(
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

    sink->callback_count++;
    sink->byte_acc += (uint64_t)data_len + member_id;
    if ((data_len > 0u) && (data != NULL))
    {
        sink->byte_acc += data[0];
    }

    return SSZ_SUCCESS;
}

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

static uint8_t g_uint8_encoded[1];
static uint8_t g_uint16_encoded[2];
static uint8_t g_uint32_encoded[4];
static uint8_t g_uint64_encoded[8];
static uint8_t g_uint128_encoded[16];
static uint8_t g_uint256_encoded[32];
static uint8_t g_boolean_encoded[1];

static uint8_t g_vector_fixed_encoded[DESER_VECTOR_FIXED_BYTES];
static uint8_t g_vector_fixed_decoded[DESER_VECTOR_FIXED_BYTES];

static uint8_t g_vector_variable_encoded[DESER_VECTOR_VARIABLE_CAP];
static size_t g_vector_variable_encoded_len = 0u;

static uint8_t g_list_fixed_encoded[DESER_LIST_FIXED_BYTES];
static uint8_t g_list_fixed_decoded[DESER_LIST_FIXED_BYTES];

static uint8_t g_list_variable_encoded[DESER_LIST_VARIABLE_CAP];
static size_t g_list_variable_encoded_len = 0u;

static uint8_t g_bitvector_encoded[DESER_BITVECTOR_BYTES];
static uint8_t g_bitvector_decoded[DESER_BITVECTOR_BYTES];

static uint8_t g_bitlist_raw[DESER_BITLIST_IN_BYTES];
static uint8_t g_bitlist_encoded[DESER_BITLIST_IN_BYTES + 1u];
static size_t g_bitlist_encoded_len = 0u;
static uint8_t g_bitlist_decoded[DESER_BITLIST_IN_BYTES];

static const size_t g_container_field_fixed_sizes[DESER_CONTAINER_FIELD_COUNT] = {
    48u,
    32u,
    8u,
    1u,
    8u,
    0u,
    0u,
};

static uint8_t g_container_field0[48];
static uint8_t g_container_field1[32];
static uint8_t g_container_field2[8];
static uint8_t g_container_field3[1];
static uint8_t g_container_field4[8];
static uint8_t g_container_field5[DESER_CONTAINER_VAR0_LEN];
static uint8_t g_container_field6[DESER_CONTAINER_VAR1_LEN];

static bench_payload_entry_t g_container_entries[DESER_CONTAINER_FIELD_COUNT];
static bench_payload_map_t g_container_payload_map;
static ssz_member_codec_t g_container_write_codec;

static uint8_t g_container_encoded[DESER_CONTAINER_OUTPUT_CAP];
static size_t g_container_encoded_len = 0u;

static bench_var_payload_ctx_t g_vector_variable_payload_cfg;
static ssz_member_codec_t g_vector_variable_write_codec;

static bench_var_payload_ctx_t g_list_variable_payload_cfg;
static ssz_member_codec_t g_list_variable_write_codec;

static bench_read_sink_t g_vector_variable_read_sink;
static bench_read_sink_t g_list_variable_read_sink;
static bench_read_sink_t g_container_read_sink;
static bench_read_sink_t g_union_read_sink;
static bench_read_sink_t g_compatible_union_read_sink;

static ssz_member_codec_t g_vector_variable_read_codec;
static ssz_member_codec_t g_list_variable_read_codec;
static ssz_member_codec_t g_container_read_codec;
static ssz_member_codec_t g_union_read_codec;
static ssz_member_codec_t g_compatible_union_read_codec;

static const uint8_t g_union_payload_selector2[2] = {0xDEu, 0xADu};
static const uint8_t g_union_payload_selector7[2] = {0x77u, 0x88u};
static const uint8_t g_compatible_union_allowed_selectors[3] = {
    1u,
    DESER_UNION_COMPAT_SELECTOR_A,
    DESER_UNION_COMPAT_SELECTOR_B,
};
static bench_payload_entry_t g_union_entries[2];
static bench_payload_map_t g_union_payload_map;
static ssz_member_codec_t g_union_write_codec;
static uint8_t g_union_encoded[16];
static size_t g_union_encoded_len = 0u;
static uint8_t g_compatible_union_encoded[16];
static size_t g_compatible_union_encoded_len = 0u;

static void bench_init_deserialize_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    ssz_error_t err = ssz_serialize_uint8(0xABu, g_uint8_encoded);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_uint16(UINT16_C(0xBEEF), g_uint16_encoded);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_uint32(UINT32_C(0x78563412), g_uint32_encoded);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_uint64(UINT64_C(0x1122334455667788), g_uint64_encoded);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    for (size_t i = 0u; i < sizeof(g_uint256_encoded); i++)
    {
        g_uint256_encoded[i] = (uint8_t)(0x90u + (uint8_t)i);
    }

    for (size_t i = 0u; i < sizeof(g_uint128_encoded); i++)
    {
        g_uint128_encoded[i] = (uint8_t)(0x50u + (uint8_t)i);
    }

    err = ssz_serialize_boolean(1u, g_boolean_encoded);
    if (err != SSZ_SUCCESS)
    {
        g_init_state = -1;
        return;
    }

    for (size_t i = 0u; i < sizeof(g_vector_fixed_encoded); i++)
    {
        g_vector_fixed_encoded[i] = (uint8_t)(i * 13u + 5u);
    }

    for (size_t i = 0u; i < sizeof(g_list_fixed_encoded); i++)
    {
        g_list_fixed_encoded[i] = (uint8_t)(i * 19u + 7u);
    }

    for (size_t i = 0u; i < sizeof(g_bitvector_encoded); i++)
    {
        g_bitvector_encoded[i] = (uint8_t)(0x3Cu ^ (uint8_t)(i * 15u));
    }

    for (size_t i = 0u; i < sizeof(g_bitlist_raw); i++)
    {
        g_bitlist_raw[i] = (uint8_t)(0xA5u ^ (uint8_t)(i * 7u));
    }
    g_bitlist_raw[sizeof(g_bitlist_raw) - 1u] &= 0x3Fu;

    err = ssz_serialize_bitlist(g_bitlist_raw,
                                sizeof(g_bitlist_raw),
                                DESER_BITLIST_BIT_LEN,
                                DESER_BITLIST_BIT_MAX,
                                g_bitlist_encoded,
                                sizeof(g_bitlist_encoded),
                                &g_bitlist_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_bitlist_encoded_len > sizeof(g_bitlist_encoded)))
    {
        g_init_state = -1;
        return;
    }

    g_vector_variable_payload_cfg = (bench_var_payload_ctx_t){
        .min_len = DESER_VECTOR_VARIABLE_MIN_LEN,
        .span = DESER_VECTOR_VARIABLE_SPAN,
        .seed = 0x41u,
    };
    g_vector_variable_write_codec = (ssz_member_codec_t){
        .ctx = &g_vector_variable_payload_cfg,
        .write = bench_variable_payload_write,
        .read = NULL,
        .root = NULL,
    };

    err = ssz_serialize_vector_variable(DESER_VECTOR_VARIABLE_COUNT,
                                        &g_vector_variable_write_codec,
                                        g_vector_variable_encoded,
                                        sizeof(g_vector_variable_encoded),
                                        &g_vector_variable_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_vector_variable_encoded_len > sizeof(g_vector_variable_encoded)))
    {
        g_init_state = -1;
        return;
    }

    g_list_variable_payload_cfg = (bench_var_payload_ctx_t){
        .min_len = DESER_LIST_VARIABLE_MIN_LEN,
        .span = DESER_LIST_VARIABLE_SPAN,
        .seed = 0x72u,
    };
    g_list_variable_write_codec = (ssz_member_codec_t){
        .ctx = &g_list_variable_payload_cfg,
        .write = bench_variable_payload_write,
        .read = NULL,
        .root = NULL,
    };

    err = ssz_serialize_list_variable(DESER_LIST_VARIABLE_COUNT,
                                      DESER_LIST_VARIABLE_LIMIT,
                                      &g_list_variable_write_codec,
                                      g_list_variable_encoded,
                                      sizeof(g_list_variable_encoded),
                                      &g_list_variable_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_list_variable_encoded_len > sizeof(g_list_variable_encoded)))
    {
        g_init_state = -1;
        return;
    }

    for (size_t i = 0u; i < sizeof(g_container_field0); i++)
    {
        g_container_field0[i] = (uint8_t)(0x10u + (uint8_t)i);
    }
    for (size_t i = 0u; i < sizeof(g_container_field1); i++)
    {
        g_container_field1[i] = (uint8_t)(0x40u + (uint8_t)i);
    }
    for (size_t i = 0u; i < sizeof(g_container_field2); i++)
    {
        g_container_field2[i] = (uint8_t)(0x60u + (uint8_t)i);
    }
    g_container_field3[0] = 1u;
    for (size_t i = 0u; i < sizeof(g_container_field4); i++)
    {
        g_container_field4[i] = (uint8_t)(0x70u + (uint8_t)i);
    }
    for (size_t i = 0u; i < sizeof(g_container_field5); i++)
    {
        g_container_field5[i] = (uint8_t)(0x80u + (uint8_t)i);
    }
    for (size_t i = 0u; i < sizeof(g_container_field6); i++)
    {
        g_container_field6[i] = (uint8_t)(0xC0u + (uint8_t)i);
    }

    g_container_entries[0] = (bench_payload_entry_t){0u, g_container_field0, sizeof(g_container_field0)};
    g_container_entries[1] = (bench_payload_entry_t){1u, g_container_field1, sizeof(g_container_field1)};
    g_container_entries[2] = (bench_payload_entry_t){2u, g_container_field2, sizeof(g_container_field2)};
    g_container_entries[3] = (bench_payload_entry_t){3u, g_container_field3, sizeof(g_container_field3)};
    g_container_entries[4] = (bench_payload_entry_t){4u, g_container_field4, sizeof(g_container_field4)};
    g_container_entries[5] = (bench_payload_entry_t){5u, g_container_field5, sizeof(g_container_field5)};
    g_container_entries[6] = (bench_payload_entry_t){6u, g_container_field6, sizeof(g_container_field6)};

    g_container_payload_map = (bench_payload_map_t){
        .entries = g_container_entries,
        .entry_count = DESER_CONTAINER_FIELD_COUNT,
    };
    g_container_write_codec = (ssz_member_codec_t){
        .ctx = &g_container_payload_map,
        .write = bench_payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    err = ssz_serialize_container(g_container_field_fixed_sizes,
                                  DESER_CONTAINER_FIELD_COUNT,
                                  &g_container_write_codec,
                                  g_container_encoded,
                                  sizeof(g_container_encoded),
                                  &g_container_encoded_len);
    if ((err != SSZ_SUCCESS) ||
        (g_container_encoded_len != DESER_CONTAINER_EXPECTED_SIZE) ||
        (g_container_encoded_len > sizeof(g_container_encoded)))
    {
        g_init_state = -1;
        return;
    }

    g_union_entries[0] = (bench_payload_entry_t){
        DESER_UNION_SELECTOR_PAYLOAD,
        g_union_payload_selector2,
        sizeof(g_union_payload_selector2),
    };
    g_union_entries[1] = (bench_payload_entry_t){
        DESER_UNION_COMPAT_SELECTOR_B,
        g_union_payload_selector7,
        sizeof(g_union_payload_selector7),
    };
    g_union_payload_map = (bench_payload_map_t){
        .entries = g_union_entries,
        .entry_count = sizeof(g_union_entries) / sizeof(g_union_entries[0]),
    };
    g_union_write_codec = (ssz_member_codec_t){
        .ctx = &g_union_payload_map,
        .write = bench_payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    err = ssz_serialize_union(DESER_UNION_SELECTOR_PAYLOAD,
                              DESER_UNION_OPTION_COUNT,
                              true,
                              &g_union_write_codec,
                              g_union_encoded,
                              sizeof(g_union_encoded),
                              &g_union_encoded_len);
    if ((err != SSZ_SUCCESS) || (g_union_encoded_len != 1u + sizeof(g_union_payload_selector2)))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_compatible_union(DESER_UNION_COMPAT_SELECTOR_B,
                                         g_compatible_union_allowed_selectors,
                                         sizeof(g_compatible_union_allowed_selectors),
                                         &g_union_write_codec,
                                         g_compatible_union_encoded,
                                         sizeof(g_compatible_union_encoded),
                                         &g_compatible_union_encoded_len);
    if ((err != SSZ_SUCCESS) ||
        (g_compatible_union_encoded_len != 1u + sizeof(g_union_payload_selector7)))
    {
        g_init_state = -1;
        return;
    }

    g_vector_variable_read_codec = (ssz_member_codec_t){
        .ctx = &g_vector_variable_read_sink,
        .write = NULL,
        .read = bench_read_sink_callback,
        .root = NULL,
    };
    g_list_variable_read_codec = (ssz_member_codec_t){
        .ctx = &g_list_variable_read_sink,
        .write = NULL,
        .read = bench_read_sink_callback,
        .root = NULL,
    };
    g_container_read_codec = (ssz_member_codec_t){
        .ctx = &g_container_read_sink,
        .write = NULL,
        .read = bench_read_sink_callback,
        .root = NULL,
    };
    g_union_read_codec = (ssz_member_codec_t){
        .ctx = &g_union_read_sink,
        .write = NULL,
        .read = bench_read_sink_callback,
        .root = NULL,
    };
    g_compatible_union_read_codec = (ssz_member_codec_t){
        .ctx = &g_compatible_union_read_sink,
        .write = NULL,
        .read = bench_read_sink_callback,
        .root = NULL,
    };

    g_init_state = 1;
}

UBENCH(deserialize, uint8)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out_value = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint8(g_uint8_encoded, &out_value));
    }
    ubench_do_nothing((void *)&out_value);
}

UBENCH(deserialize, uint16)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint16_t out_value = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint16(g_uint16_encoded, &out_value));
    }
    ubench_do_nothing((void *)&out_value);
}

UBENCH(deserialize, uint32)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint32_t out_value = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint32(g_uint32_encoded, &out_value));
    }
    ubench_do_nothing((void *)&out_value);
}

UBENCH(deserialize, uint128)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out_value[16];
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint128(g_uint128_encoded, out_value));
    }
    ubench_do_nothing(out_value);
}

UBENCH(deserialize, boolean)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out_value = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_boolean(g_boolean_encoded, &out_value));
    }
    ubench_do_nothing((void *)&out_value);
}

UBENCH(deserialize, uint64)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t out_value = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint64(g_uint64_encoded, &out_value));
    }
    ubench_do_nothing((void *)&out_value);
}

UBENCH(deserialize, uint256)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out_value[32];
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_uint256(g_uint256_encoded, out_value));
    }
    ubench_do_nothing(out_value);
}

UBENCH(deserialize, vector_fixed_1024x8)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    BENCH_EXPECT_OK(ssz_deserialize_vector_fixed(g_vector_fixed_encoded,
                                                 sizeof(g_vector_fixed_encoded),
                                                 DESER_VECTOR_FIXED_COUNT,
                                                 DESER_VECTOR_FIXED_ELEMENT_SIZE,
                                                 g_vector_fixed_decoded,
                                                 sizeof(g_vector_fixed_decoded)));
    ubench_do_nothing(g_vector_fixed_decoded);
}

UBENCH(deserialize, vector_variable_256)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_vector_variable_read_sink.callback_count = 0u;
    g_vector_variable_read_sink.byte_acc = 0u;

    BENCH_EXPECT_OK(ssz_deserialize_vector_variable(g_vector_variable_encoded,
                                                    g_vector_variable_encoded_len,
                                                    DESER_VECTOR_VARIABLE_COUNT,
                                                    DESER_VECTOR_VARIABLE_MIN_LEN,
                                                    &g_vector_variable_read_codec));

    ubench_do_nothing((void *)&g_vector_variable_read_sink);
}

UBENCH(deserialize, list_fixed_1024x32)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t out_count = 0u;
    BENCH_EXPECT_OK(ssz_deserialize_list_fixed(g_list_fixed_encoded,
                                               sizeof(g_list_fixed_encoded),
                                               DESER_LIST_FIXED_LIMIT,
                                               DESER_LIST_FIXED_ELEMENT_SIZE,
                                               g_list_fixed_decoded,
                                               sizeof(g_list_fixed_decoded),
                                               &out_count));
    ubench_do_nothing(g_list_fixed_decoded);
    ubench_do_nothing((void *)&out_count);
}

UBENCH(deserialize, list_variable_192)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_list_variable_read_sink.callback_count = 0u;
    g_list_variable_read_sink.byte_acc = 0u;

    uint64_t out_count = 0u;
    BENCH_EXPECT_OK(ssz_deserialize_list_variable(g_list_variable_encoded,
                                                  g_list_variable_encoded_len,
                                                  DESER_LIST_VARIABLE_LIMIT,
                                                  DESER_LIST_VARIABLE_MIN_LEN,
                                                  &g_list_variable_read_codec,
                                                  &out_count));

    ubench_do_nothing((void *)&g_list_variable_read_sink);
    ubench_do_nothing((void *)&out_count);
}

UBENCH(deserialize, bitvector_8192)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_bitvector(g_bitvector_encoded,
                                                  sizeof(g_bitvector_encoded),
                                                  DESER_BITVECTOR_BIT_COUNT,
                                                  g_bitvector_decoded,
                                                  sizeof(g_bitvector_decoded)));
    }
    ubench_do_nothing(g_bitvector_decoded);
}

UBENCH(deserialize, bitlist_8190)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint64_t out_bit_len = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_bitlist(g_bitlist_encoded,
                                                g_bitlist_encoded_len,
                                                DESER_BITLIST_BIT_MAX,
                                                g_bitlist_decoded,
                                                sizeof(g_bitlist_decoded),
                                                &out_bit_len));
    }

    ubench_do_nothing(g_bitlist_decoded);
    ubench_do_nothing((void *)&out_bit_len);
}

UBENCH(deserialize, container_validator_like_mixed)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_container_read_sink.callback_count = 0u;
    g_container_read_sink.byte_acc = 0u;

    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_container(g_container_encoded,
                                                  g_container_encoded_len,
                                                  g_container_field_fixed_sizes,
                                                  DESER_CONTAINER_FIELD_COUNT,
                                                  &g_container_read_codec));
    }

    ubench_do_nothing((void *)&g_container_read_sink);
}

UBENCH(deserialize, union)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_union_read_sink.callback_count = 0u;
    g_union_read_sink.byte_acc = 0u;

    uint8_t out_selector = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_union(g_union_encoded,
                                              g_union_encoded_len,
                                              DESER_UNION_OPTION_COUNT,
                                              true,
                                              &g_union_read_codec,
                                              &out_selector));
    }

    ubench_do_nothing((void *)&g_union_read_sink);
    ubench_do_nothing((void *)&out_selector);
}

UBENCH(deserialize, compatible_union)
{
    bench_init_deserialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    g_compatible_union_read_sink.callback_count = 0u;
    g_compatible_union_read_sink.byte_acc = 0u;

    uint8_t out_selector = 0u;
    for (size_t i = 0u; i < DESER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_deserialize_compatible_union(g_compatible_union_encoded,
                                                         g_compatible_union_encoded_len,
                                                         g_compatible_union_allowed_selectors,
                                                         sizeof(g_compatible_union_allowed_selectors),
                                                         &g_compatible_union_read_codec,
                                                         &out_selector));
    }

    ubench_do_nothing((void *)&g_compatible_union_read_sink);
    ubench_do_nothing((void *)&out_selector);
}

UBENCH_MAIN();
