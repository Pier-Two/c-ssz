#include <stdint.h>
#include <string.h>

#include "ssz.h"
#include "ubench.h"

#define SER_VECTOR_FIXED_COUNT        1024u
#define SER_VECTOR_FIXED_ELEMENT_SIZE 8u
#define SER_VECTOR_FIXED_BYTES        (SER_VECTOR_FIXED_COUNT * SER_VECTOR_FIXED_ELEMENT_SIZE)

#define SER_LIST_FIXED_COUNT        1024u
#define SER_LIST_FIXED_LIMIT        2048u
#define SER_LIST_FIXED_ELEMENT_SIZE 32u
#define SER_LIST_FIXED_BYTES        (SER_LIST_FIXED_COUNT * SER_LIST_FIXED_ELEMENT_SIZE)

#define SER_VECTOR_VARIABLE_COUNT   256u
#define SER_VECTOR_VARIABLE_MIN_LEN 24u
#define SER_VECTOR_VARIABLE_SPAN    16u
#define SER_VECTOR_VARIABLE_MAX_LEN (SER_VECTOR_VARIABLE_MIN_LEN + SER_VECTOR_VARIABLE_SPAN - 1u)
#define SER_VECTOR_VARIABLE_CAP \
    (SER_VECTOR_VARIABLE_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + SER_VECTOR_VARIABLE_MAX_LEN))

#define SER_LIST_VARIABLE_COUNT   192u
#define SER_LIST_VARIABLE_LIMIT   512u
#define SER_LIST_VARIABLE_MIN_LEN 16u
#define SER_LIST_VARIABLE_SPAN    24u
#define SER_LIST_VARIABLE_MAX_LEN (SER_LIST_VARIABLE_MIN_LEN + SER_LIST_VARIABLE_SPAN - 1u)
#define SER_LIST_VARIABLE_CAP \
    (SER_LIST_VARIABLE_COUNT * (SSZ_BYTES_PER_LENGTH_OFFSET + SER_LIST_VARIABLE_MAX_LEN))

#define SER_BITVECTOR_BIT_COUNT 8192u
#define SER_BITVECTOR_BYTES     (SER_BITVECTOR_BIT_COUNT / 8u)

#define SER_BITLIST_BIT_LEN  8190u
#define SER_BITLIST_BIT_MAX  16384u
#define SER_BITLIST_IN_BYTES ((SER_BITLIST_BIT_LEN + 7u) / 8u)

#define SER_SMALL_BATCH 64u

#define SER_CONTAINER_FIELD_COUNT  7u
#define SER_CONTAINER_VAR0_LEN     96u
#define SER_CONTAINER_VAR1_LEN     128u
#define SER_CONTAINER_OUTPUT_CAP   512u
#define SER_CONTAINER_FIXED_REGION (48u + 32u + 8u + 1u + 8u + 4u + 4u)
#define SER_CONTAINER_EXPECTED_SIZE \
    (SER_CONTAINER_FIXED_REGION + SER_CONTAINER_VAR0_LEN + SER_CONTAINER_VAR1_LEN)

#define SER_UNION_OPTION_COUNT      3u
#define SER_UNION_SELECTOR_NONE     0u
#define SER_UNION_SELECTOR_PAYLOAD  2u
#define SER_UNION_COMPAT_SELECTOR_A 2u
#define SER_UNION_COMPAT_SELECTOR_B 7u

typedef struct
{
    uint64_t id;
    const uint8_t *data;
    size_t len;
} bench_payload_entry_t;

typedef struct
{
    const bench_payload_entry_t *entries;
    size_t entry_count;
} bench_payload_map_t;

typedef struct
{
    size_t min_len;
    size_t span;
    uint8_t seed;
} bench_var_payload_ctx_t;

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

static ssz_error_t bench_payload_write(
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

static int g_init_state = 0;

static uint64_t g_uint64_input = UINT64_C(0x1122334455667788);
static uint8_t g_uint256_input[32];

static uint8_t g_vector_fixed_input[SER_VECTOR_FIXED_BYTES];
static uint8_t g_vector_fixed_output[SER_VECTOR_FIXED_BYTES];

static uint8_t g_list_fixed_input[SER_LIST_FIXED_BYTES];
static uint8_t g_list_fixed_output[SER_LIST_FIXED_BYTES];

static uint8_t g_bitvector_input[SER_BITVECTOR_BYTES];
static uint8_t g_bitvector_output[SER_BITVECTOR_BYTES];

static uint8_t g_bitlist_input[SER_BITLIST_IN_BYTES];
static uint8_t g_bitlist_output[SER_BITLIST_IN_BYTES + 1u];

static const size_t g_container_field_fixed_sizes[SER_CONTAINER_FIELD_COUNT] = {
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
static uint8_t g_container_field5[SER_CONTAINER_VAR0_LEN];
static uint8_t g_container_field6[SER_CONTAINER_VAR1_LEN];

static bench_payload_entry_t g_container_entries[SER_CONTAINER_FIELD_COUNT];
static bench_payload_map_t g_container_map;
static ssz_member_codec_t g_container_codec;
static uint8_t g_container_output[SER_CONTAINER_OUTPUT_CAP];

static bench_var_payload_ctx_t g_vector_variable_payload_cfg;
static ssz_member_codec_t g_vector_variable_codec;

static bench_var_payload_ctx_t g_list_variable_payload_cfg;
static ssz_member_codec_t g_list_variable_codec;

static const uint8_t g_union_payload_selector2[2] = {0xDEu, 0xADu};
static const uint8_t g_union_payload_selector7[2] = {0x77u, 0x88u};
static const uint8_t g_union_allowed_selectors[3] = {
    1u,
    SER_UNION_COMPAT_SELECTOR_A,
    SER_UNION_COMPAT_SELECTOR_B,
};

static bench_payload_entry_t g_union_entries[2];
static bench_payload_map_t g_union_map;
static ssz_member_codec_t g_union_codec;

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

static void bench_init_serialize_data(void)
{
    if (g_init_state != 0)
    {
        return;
    }

    for (size_t i = 0u; i < sizeof(g_uint256_input); i++)
    {
        g_uint256_input[i] = (uint8_t)(0xA0u + (uint8_t)i);
    }

    for (size_t i = 0u; i < sizeof(g_vector_fixed_input); i++)
    {
        g_vector_fixed_input[i] = (uint8_t)(i * 17u + 3u);
    }

    for (size_t i = 0u; i < sizeof(g_list_fixed_input); i++)
    {
        g_list_fixed_input[i] = (uint8_t)(i * 29u + 11u);
    }

    for (size_t i = 0u; i < sizeof(g_bitvector_input); i++)
    {
        g_bitvector_input[i] = (uint8_t)(0x5Au ^ (uint8_t)(i * 9u));
    }

    for (size_t i = 0u; i < sizeof(g_bitlist_input); i++)
    {
        g_bitlist_input[i] = (uint8_t)(0xA5u ^ (uint8_t)(i * 7u));
    }
    g_bitlist_input[sizeof(g_bitlist_input) - 1u] &= 0x3Fu;

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

    g_container_entries[0] =
        (bench_payload_entry_t){0u, g_container_field0, sizeof(g_container_field0)};
    g_container_entries[1] =
        (bench_payload_entry_t){1u, g_container_field1, sizeof(g_container_field1)};
    g_container_entries[2] =
        (bench_payload_entry_t){2u, g_container_field2, sizeof(g_container_field2)};
    g_container_entries[3] =
        (bench_payload_entry_t){3u, g_container_field3, sizeof(g_container_field3)};
    g_container_entries[4] =
        (bench_payload_entry_t){4u, g_container_field4, sizeof(g_container_field4)};
    g_container_entries[5] =
        (bench_payload_entry_t){5u, g_container_field5, sizeof(g_container_field5)};
    g_container_entries[6] =
        (bench_payload_entry_t){6u, g_container_field6, sizeof(g_container_field6)};

    g_container_map.entries = g_container_entries;
    g_container_map.entry_count = SER_CONTAINER_FIELD_COUNT;
    g_container_codec.ctx = &g_container_map;
    g_container_codec.write = bench_payload_write;
    g_container_codec.read = NULL;
    g_container_codec.root = NULL;

    g_vector_variable_payload_cfg = (bench_var_payload_ctx_t){
        .min_len = SER_VECTOR_VARIABLE_MIN_LEN,
        .span = SER_VECTOR_VARIABLE_SPAN,
        .seed = 0x41u,
    };
    g_vector_variable_codec = (ssz_member_codec_t){
        .ctx = &g_vector_variable_payload_cfg,
        .write = bench_variable_payload_write,
        .read = NULL,
        .root = NULL,
    };

    g_list_variable_payload_cfg = (bench_var_payload_ctx_t){
        .min_len = SER_LIST_VARIABLE_MIN_LEN,
        .span = SER_LIST_VARIABLE_SPAN,
        .seed = 0x72u,
    };
    g_list_variable_codec = (ssz_member_codec_t){
        .ctx = &g_list_variable_payload_cfg,
        .write = bench_variable_payload_write,
        .read = NULL,
        .root = NULL,
    };

    g_union_entries[0] = (bench_payload_entry_t){
        SER_UNION_SELECTOR_PAYLOAD,
        g_union_payload_selector2,
        sizeof(g_union_payload_selector2),
    };
    g_union_entries[1] = (bench_payload_entry_t){
        SER_UNION_COMPAT_SELECTOR_B,
        g_union_payload_selector7,
        sizeof(g_union_payload_selector7),
    };
    g_union_map = (bench_payload_map_t){
        .entries = g_union_entries,
        .entry_count = sizeof(g_union_entries) / sizeof(g_union_entries[0]),
    };
    g_union_codec = (ssz_member_codec_t){
        .ctx = &g_union_map,
        .write = bench_payload_write,
        .read = NULL,
        .root = NULL,
    };

    size_t out_len = 0u;
    ssz_error_t err = ssz_serialize_container(
        g_container_field_fixed_sizes,
        SER_CONTAINER_FIELD_COUNT,
        &g_container_codec,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len != SER_CONTAINER_EXPECTED_SIZE))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_vector_variable(
        SER_VECTOR_VARIABLE_COUNT,
        &g_vector_variable_codec,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len > SER_VECTOR_VARIABLE_CAP))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_list_variable(
        SER_LIST_VARIABLE_COUNT,
        SER_LIST_VARIABLE_LIMIT,
        &g_list_variable_codec,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len > SER_LIST_VARIABLE_CAP))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_union(
        SER_UNION_SELECTOR_PAYLOAD,
        SER_UNION_OPTION_COUNT,
        true,
        &g_union_codec,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len != 1u + sizeof(g_union_payload_selector2)))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_compatible_union(
        SER_UNION_COMPAT_SELECTOR_B,
        g_union_allowed_selectors,
        sizeof(g_union_allowed_selectors),
        &g_union_codec,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len != 1u + sizeof(g_union_payload_selector7)))
    {
        g_init_state = -1;
        return;
    }

    err = ssz_serialize_bitlist(
        g_bitlist_input,
        sizeof(g_bitlist_input),
        SER_BITLIST_BIT_LEN,
        SER_BITLIST_BIT_MAX,
        NULL,
        0u,
        &out_len);
    if ((err != SSZ_SUCCESS) || (out_len > sizeof(g_bitlist_output)))
    {
        g_init_state = -1;
        return;
    }

    g_init_state = 1;
}

UBENCH(serialize, uint8)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[1];
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_uint8((uint8_t)(0x80u + (uint8_t)i), out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, uint16)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[2];
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_uint16((uint16_t)(0xB000u + (uint16_t)i), out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, uint32)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[4];
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_uint32(UINT32_C(0x78000000) + (uint32_t)i, out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, uint128)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t in[16];
    uint8_t out[16];
    for (size_t i = 0u; i < sizeof(in); i++)
    {
        in[i] = (uint8_t)(0xA0u + (uint8_t)i);
    }

    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        in[0] = (uint8_t)(0xA0u + (uint8_t)i);
        BENCH_EXPECT_OK(ssz_serialize_uint128(in, sizeof(in), out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, boolean)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[1];
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_boolean((uint8_t)(i & 1u), out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, uint64)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[8];
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_uint64(g_uint64_input + i, out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, uint256)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t in[32];
    uint8_t out[32];
    memcpy(in, g_uint256_input, sizeof(in));

    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        in[0] = (uint8_t)(g_uint256_input[0] + (uint8_t)i);
        BENCH_EXPECT_OK(ssz_serialize_uint256(in, sizeof(in), out));
    }
    ubench_do_nothing(out);
}

UBENCH(serialize, vector_fixed_1024x8)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_vector_fixed(
        g_vector_fixed_input,
        SER_VECTOR_FIXED_COUNT,
        SER_VECTOR_FIXED_ELEMENT_SIZE,
        g_vector_fixed_output,
        sizeof(g_vector_fixed_output),
        &out_len));
    ubench_do_nothing(g_vector_fixed_output);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, list_fixed_1024x32)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_list_fixed(
        g_list_fixed_input,
        SER_LIST_FIXED_COUNT,
        SER_LIST_FIXED_LIMIT,
        SER_LIST_FIXED_ELEMENT_SIZE,
        g_list_fixed_output,
        sizeof(g_list_fixed_output),
        &out_len));
    ubench_do_nothing(g_list_fixed_output);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, vector_variable_256)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[SER_VECTOR_VARIABLE_CAP];
    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_vector_variable(
        SER_VECTOR_VARIABLE_COUNT,
        &g_vector_variable_codec,
        out,
        sizeof(out),
        &out_len));
    ubench_do_nothing(out);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, list_variable_192)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[SER_LIST_VARIABLE_CAP];
    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_list_variable(
        SER_LIST_VARIABLE_COUNT,
        SER_LIST_VARIABLE_LIMIT,
        &g_list_variable_codec,
        out,
        sizeof(out),
        &out_len));
    ubench_do_nothing(out);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, bitvector_8192)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_bitvector(
            g_bitvector_input,
            sizeof(g_bitvector_input),
            SER_BITVECTOR_BIT_COUNT,
            g_bitvector_output,
            sizeof(g_bitvector_output),
            &out_len));
    }
    ubench_do_nothing(g_bitvector_output);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, bitlist_8190)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        BENCH_EXPECT_OK(ssz_serialize_bitlist(
            g_bitlist_input,
            sizeof(g_bitlist_input),
            SER_BITLIST_BIT_LEN,
            SER_BITLIST_BIT_MAX,
            g_bitlist_output,
            sizeof(g_bitlist_output),
            &out_len));
    }
    ubench_do_nothing(g_bitlist_output);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, container_validator_like_mixed)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    size_t out_len = 0u;
    BENCH_EXPECT_OK(ssz_serialize_container(
        g_container_field_fixed_sizes,
        SER_CONTAINER_FIELD_COUNT,
        &g_container_codec,
        g_container_output,
        sizeof(g_container_output),
        &out_len));
    ubench_do_nothing(g_container_output);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, union)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[16];
    size_t out_len = 0u;
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        uint8_t selector = (i & 1u) ? SER_UNION_SELECTOR_PAYLOAD : SER_UNION_SELECTOR_NONE;
        BENCH_EXPECT_OK(ssz_serialize_union(
            selector,
            SER_UNION_OPTION_COUNT,
            true,
            &g_union_codec,
            out,
            sizeof(out),
            &out_len));
    }
    ubench_do_nothing(out);
    ubench_do_nothing((void *)&out_len);
}

UBENCH(serialize, compatible_union)
{
    bench_init_serialize_data();
    if (g_init_state != 1)
    {
        return;
    }

    uint8_t out[16];
    size_t out_len = 0u;
    for (size_t i = 0u; i < SER_SMALL_BATCH; i++)
    {
        uint8_t selector = (i & 1u) ? SER_UNION_COMPAT_SELECTOR_B : SER_UNION_COMPAT_SELECTOR_A;
        BENCH_EXPECT_OK(ssz_serialize_compatible_union(
            selector,
            g_union_allowed_selectors,
            sizeof(g_union_allowed_selectors),
            &g_union_codec,
            out,
            sizeof(out),
            &out_len));
    }
    ubench_do_nothing(out);
    ubench_do_nothing((void *)&out_len);
}

UBENCH_MAIN();
