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

#define ASSERT_MEM_EQ(actual, expected, len)                                        \
    do                                                                              \
    {                                                                               \
        if (memcmp((actual), (expected), (len)) != 0)                               \
        {                                                                           \
            fprintf(                                                                \
                stderr,                                                             \
                "Assertion failed at %s:%d: memory mismatch (%s vs %s, len=%zu)\n", \
                __FILE__,                                                           \
                __LINE__,                                                           \
                #actual,                                                            \
                #expected,                                                          \
                (size_t)(len));                                                     \
            return false;                                                           \
        }                                                                           \
    } while (0)

typedef struct
{
    uint64_t id;
    const uint8_t *data;
    size_t len;
} payload_entry_t;

typedef struct
{
    const payload_entry_t *entries;
    size_t entry_count;
} payload_map_ctx_t;

static ssz_error_t payload_map_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const payload_map_ctx_t *map = (const payload_map_ctx_t *)ctx;

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

static ssz_error_t fail_if_called_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    (void)ctx;
    (void)member_id;
    (void)out;
    (void)out_cap;
    (void)out_written;
    return SSZ_ERR_TYPE_MISMATCH;
}

typedef struct
{
    const size_t *lengths;
    const ssz_error_t *errors;
    size_t step_count;
    size_t step_index;
    uint8_t fill;
} scripted_write_ctx_t;

static ssz_error_t scripted_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    scripted_write_ctx_t *script = (scripted_write_ctx_t *)ctx;
    (void)member_id;

    if ((script == NULL) || (out_written == NULL) || (script->lengths == NULL) ||
        (script->errors == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (script->step_index >= script->step_count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    size_t idx = script->step_index++;
    *out_written = script->lengths[idx];
    if (script->errors[idx] != SSZ_SUCCESS)
    {
        return script->errors[idx];
    }

    if ((out != NULL) && (out_cap != 0u))
    {
        size_t fill_len = (script->lengths[idx] < out_cap) ? script->lengths[idx] : out_cap;
        memset(out, script->fill, fill_len);
    }

    return SSZ_SUCCESS;
}

static ssz_member_codec_t make_scripted_codec(scripted_write_ctx_t *ctx)
{
    ssz_member_codec_t codec = {
        .ctx = ctx,
        .write = scripted_write,
        .read = NULL,
        .root = NULL,
    };
    return codec;
}

static bool test_serialize_basic_types_and_aliases(void)
{
    uint8_t u8 = 0u;
    uint8_t u16[2] = {0u};
    uint8_t u32[4] = {0u};
    uint8_t u64[8] = {0u};
    uint8_t u128[16] = {0u};
    uint8_t u256[32] = {0u};
    uint8_t boolean_out = 0u;
    uint8_t byte_out = 0u;
    uint8_t bit_out = 0u;

    const uint8_t in128[16] = {
        0x00,
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77,
        0x88,
        0x99,
        0xAA,
        0xBB,
        0xCC,
        0xDD,
        0xEE,
        0xFF,
    };
    const uint8_t in256[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    };

    ASSERT_ERR(ssz_serialize_uint8(0xABu, &u8), SSZ_SUCCESS);
    ASSERT_TRUE(u8 == 0xABu);

    ASSERT_ERR(ssz_serialize_uint16(UINT16_C(0xBEEF), u16), SSZ_SUCCESS);
    ASSERT_TRUE(u16[0] == 0xEFu);
    ASSERT_TRUE(u16[1] == 0xBEu);

    ASSERT_ERR(ssz_serialize_uint32(UINT32_C(0x78563412), u32), SSZ_SUCCESS);
    ASSERT_MEM_EQ(u32, ((const uint8_t[4]){0x12, 0x34, 0x56, 0x78}), 4u);

    ASSERT_ERR(ssz_serialize_uint64(UINT64_C(0x0102030405060708), u64), SSZ_SUCCESS);
    ASSERT_MEM_EQ(u64, ((const uint8_t[8]){0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01}), 8u);

    ASSERT_ERR(ssz_serialize_uint128(in128, sizeof(in128), u128), SSZ_SUCCESS);
    ASSERT_MEM_EQ(u128, in128, sizeof(in128));

    ASSERT_ERR(ssz_serialize_uint256(in256, sizeof(in256), u256), SSZ_SUCCESS);
    ASSERT_MEM_EQ(u256, in256, sizeof(in256));

    ASSERT_ERR(ssz_serialize_boolean(1u, &boolean_out), SSZ_SUCCESS);
    ASSERT_TRUE(boolean_out == 1u);

    ASSERT_ERR(ssz_serialize_uint8(0xFEu, &byte_out), SSZ_SUCCESS);
    ASSERT_TRUE(byte_out == 0xFEu);

    ASSERT_ERR(ssz_serialize_boolean(1u, &bit_out), SSZ_SUCCESS);
    ASSERT_TRUE(bit_out == 1u);

    return true;
}

static bool test_serialize_boolean_canonical_rejection(void)
{
    uint8_t out = 0u;

    ASSERT_ERR(ssz_serialize_boolean(2u, &out), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_serialize_boolean(0xFFu, &out), SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_serialize_fixed_width_exact_and_invalid_lengths(void)
{
    const uint8_t exact128[16] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
        0xFFu,
    };
    const uint8_t in128[15] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
    };
    const uint8_t overlong128[17] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
        0xFFu,
        0x42u,
    };
    const uint8_t exact256[32] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu,
    };
    const uint8_t in256[31] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu,
    };
    const uint8_t overlong256[33] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu, 0x42u,
    };
    uint8_t out128[16];
    uint8_t out256[32];
    uint8_t expected128[16];
    uint8_t expected256[32];

    ASSERT_ERR(ssz_serialize_uint128(exact128, sizeof(exact128), out128), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out128, exact128, sizeof(out128));
    ASSERT_ERR(ssz_serialize_uint256(exact256, sizeof(exact256), out256), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out256, exact256, sizeof(out256));

    memset(out128, 0x3C, sizeof(out128));
    memset(out256, 0xC3, sizeof(out256));
    memset(expected128, 0x3C, sizeof(expected128));
    memset(expected256, 0xC3, sizeof(expected256));

    ASSERT_ERR(ssz_serialize_uint128(in128, sizeof(in128), out128), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_serialize_uint128(overlong128, sizeof(overlong128), out128),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_serialize_uint256(in256, sizeof(in256), out256), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_serialize_uint256(overlong256, sizeof(overlong256), out256),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_MEM_EQ(out128, expected128, sizeof(out128));
    ASSERT_MEM_EQ(out256, expected256, sizeof(out256));

    return true;
}

static bool test_serialize_bitvector_and_padding_validation(void)
{
    const uint8_t valid_bits[2] = {0x55u, 0x01u};
    const uint8_t invalid_bits[2] = {0x55u, 0xC1u};
    uint8_t out[2] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_bitvector(valid_bits, sizeof(valid_bits), 10u, NULL, 0u, &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);

    ASSERT_ERR(
        ssz_serialize_bitvector(valid_bits, sizeof(valid_bits), 10u, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);
    ASSERT_MEM_EQ(out, valid_bits, 2u);

    ASSERT_ERR(
        ssz_serialize_bitvector(
            invalid_bits,
            sizeof(invalid_bits),
            10u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_serialize_bitlist_delimiter_and_limit(void)
{
    const uint8_t bits[2] = {0xAAu, 0x02u};
    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_bitlist(bits, sizeof(bits), 10u, 10u, NULL, 0u, &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);

    ASSERT_ERR(
        ssz_serialize_bitlist(bits, sizeof(bits), 10u, 10u, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 2u);
    ASSERT_MEM_EQ(out, ((const uint8_t[2]){0xAAu, 0x06u}), 2u);

    ASSERT_ERR(
        ssz_serialize_bitlist(bits, sizeof(bits), 10u, 9u, out, sizeof(out), &out_len),
        SSZ_ERR_LIMIT_EXCEEDED);

    return true;
}

static bool test_serialize_vector_fixed_and_size_query(void)
{
    const uint8_t elements[6] = {0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u};
    uint8_t out[6] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_serialize_vector_fixed(elements, 3u, 2u, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == sizeof(elements));

    ASSERT_ERR(
        ssz_serialize_vector_fixed(elements, 3u, 2u, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == sizeof(elements));
    ASSERT_MEM_EQ(out, elements, sizeof(elements));

    return true;
}

static bool test_serialize_vector_variable_and_size_query(void)
{
    const uint8_t e0[] = {0xA0u};
    const uint8_t e1[] = {0xB0u, 0xB1u};
    const uint8_t e2[] = {0xC0u, 0xC1u, 0xC2u};
    const payload_entry_t entries[] = {
        {0u, e0, sizeof(e0)},
        {1u, e1, sizeof(e1)},
        {2u, e2, sizeof(e2)},
    };
    const payload_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    uint8_t out[32] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_serialize_vector_variable(3u, &codec, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 18u);

    ASSERT_ERR(ssz_serialize_vector_variable(3u, &codec, out, sizeof(out), &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 18u);
    ASSERT_MEM_EQ(
        out,
        ((const uint8_t[18]){
            0x0Cu,
            0x00u,
            0x00u,
            0x00u,
            0x0Du,
            0x00u,
            0x00u,
            0x00u,
            0x0Fu,
            0x00u,
            0x00u,
            0x00u,
            0xA0u,
            0xB0u,
            0xB1u,
            0xC0u,
            0xC1u,
            0xC2u,
        }),
        18u);

    return true;
}

static bool test_serialize_list_fixed_and_limits(void)
{
    const uint8_t elements[4] = {0x10u, 0x11u, 0x20u, 0x21u};
    uint8_t out[4] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_serialize_list_fixed(elements, 2u, 2u, 2u, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == sizeof(elements));

    ASSERT_ERR(
        ssz_serialize_list_fixed(elements, 2u, 2u, 2u, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == sizeof(elements));
    ASSERT_MEM_EQ(out, elements, sizeof(elements));

    ASSERT_ERR(
        ssz_serialize_list_fixed(elements, 2u, 1u, 2u, out, sizeof(out), &out_len),
        SSZ_ERR_LIMIT_EXCEEDED);

    return true;
}

static bool test_serialize_list_variable_and_limits(void)
{
    const uint8_t e0[] = {0xE0u, 0xE1u};
    const uint8_t e1[] = {0xF0u};
    const payload_entry_t entries[] = {
        {0u, e0, sizeof(e0)},
        {1u, e1, sizeof(e1)},
    };
    const payload_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    uint8_t out[32] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(ssz_serialize_list_variable(2u, 3u, &codec, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 11u);

    ASSERT_ERR(
        ssz_serialize_list_variable(2u, 3u, &codec, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 11u);
    ASSERT_MEM_EQ(
        out,
        ((const uint8_t[11]){
            0x08u,
            0x00u,
            0x00u,
            0x00u,
            0x0Au,
            0x00u,
            0x00u,
            0x00u,
            0xE0u,
            0xE1u,
            0xF0u,
        }),
        11u);

    ASSERT_ERR(
        ssz_serialize_list_variable(2u, 1u, &codec, out, sizeof(out), &out_len),
        SSZ_ERR_LIMIT_EXCEEDED);

    return true;
}

static bool test_serialize_container_mixed_fields_and_size_query(void)
{
    const size_t field_fixed_sizes[3] = {1u, 0u, 2u};
    const uint8_t f0[] = {0xAAu};
    const uint8_t f1[] = {0xB0u, 0xB1u, 0xB2u};
    const uint8_t f2[] = {0xCCu, 0xDDu};
    const payload_entry_t entries[] = {
        {0u, f0, sizeof(f0)},
        {1u, f1, sizeof(f1)},
        {2u, f2, sizeof(f2)},
    };
    const payload_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    uint8_t out[32] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_container(field_fixed_sizes, 3u, &codec, NULL, 0u, &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 10u);

    ASSERT_ERR(
        ssz_serialize_container(field_fixed_sizes, 3u, &codec, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 10u);
    ASSERT_MEM_EQ(
        out,
        ((const uint8_t[10]){
            0xAAu,
            0x07u,
            0x00u,
            0x00u,
            0x00u,
            0xCCu,
            0xDDu,
            0xB0u,
            0xB1u,
            0xB2u,
        }),
        10u);

    return true;
}

static bool test_serialize_union_none_normal_and_size_query(void)
{
    const ssz_member_codec_t forbidden_codec = {
        .ctx = NULL,
        .write = fail_if_called_write,
        .read = NULL,
        .root = NULL,
    };
    const uint8_t payload_selector2[] = {0xDEu, 0xADu};
    const payload_entry_t entries[] = {
        {2u, payload_selector2, sizeof(payload_selector2)},
    };
    const payload_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_union(0u, 3u, true, &forbidden_codec, NULL, 0u, &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 1u);

    ASSERT_ERR(
        ssz_serialize_union(0u, 3u, true, &forbidden_codec, out, sizeof(out), &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 1u);
    ASSERT_TRUE(out[0] == 0u);

    ASSERT_ERR(ssz_serialize_union(2u, 3u, true, &codec, NULL, 0u, &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);

    ASSERT_ERR(ssz_serialize_union(2u, 3u, true, &codec, out, sizeof(out), &out_len), SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);
    ASSERT_MEM_EQ(out, ((const uint8_t[3]){0x02u, 0xDEu, 0xADu}), 3u);

    return true;
}

static bool test_serialize_compatible_union_valid_and_invalid_selectors(void)
{
    const uint8_t allowed[] = {1u, 2u, 7u};
    const uint8_t payload_selector7[] = {0x77u, 0x88u};
    const payload_entry_t entries[] = {
        {7u, payload_selector7, sizeof(payload_selector7)},
    };
    const payload_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_compatible_union(7u, allowed, sizeof(allowed), &codec, NULL, 0u, &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);

    ASSERT_ERR(
        ssz_serialize_compatible_union(
            7u,
            allowed,
            sizeof(allowed),
            &codec,
            out,
            sizeof(out),
            &out_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_len == 3u);
    ASSERT_MEM_EQ(out, ((const uint8_t[3]){0x07u, 0x77u, 0x88u}), 3u);

    ASSERT_ERR(
        ssz_serialize_compatible_union(
            0u,
            allowed,
            sizeof(allowed),
            &codec,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SELECTOR_INVALID);
    ASSERT_ERR(
        ssz_serialize_compatible_union(
            200u,
            allowed,
            sizeof(allowed),
            &codec,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SELECTOR_INVALID);
    ASSERT_ERR(
        ssz_serialize_compatible_union(
            3u,
            allowed,
            sizeof(allowed),
            &codec,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SELECTOR_INVALID);

    return true;
}

static bool test_serialize_direct_calls(void)
{
    const size_t field_fixed_sizes[3] = {1u, 0u, 2u};
    const uint8_t f0[] = {0x01u};
    const uint8_t f1[] = {0x10u, 0x11u};
    const uint8_t f2[] = {0x20u, 0x21u};
    const payload_entry_t container_entries[] = {
        {0u, f0, sizeof(f0)},
        {1u, f1, sizeof(f1)},
        {2u, f2, sizeof(f2)},
    };
    const payload_map_ctx_t container_ctx = {
        .entries = container_entries,
        .entry_count = sizeof(container_entries) / sizeof(container_entries[0]),
    };
    const ssz_member_codec_t container_codec = {
        .ctx = (void *)&container_ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    const uint8_t lv0[] = {0xA1u};
    const uint8_t lv1[] = {0xB1u, 0xB2u};
    const payload_entry_t list_entries[] = {
        {0u, lv0, sizeof(lv0)},
        {1u, lv1, sizeof(lv1)},
    };
    const payload_map_ctx_t list_ctx = {
        .entries = list_entries,
        .entry_count = sizeof(list_entries) / sizeof(list_entries[0]),
    };
    const ssz_member_codec_t list_codec = {
        .ctx = (void *)&list_ctx,
        .write = payload_map_write,
        .read = NULL,
        .root = NULL,
    };

    const uint8_t fixed_elements[4] = {0x30u, 0x31u, 0x32u, 0x33u};
    const uint8_t bits[2] = {0x03u, 0x01u};

    uint8_t out_a[64] = {0u};
    uint8_t out_b[64] = {0u};
    size_t len_a = 0u;
    size_t len_b = 0u;

    ASSERT_ERR(
        ssz_serialize_container(
            field_fixed_sizes,
            3u,
            &container_codec,
            out_a,
            sizeof(out_a),
            &len_a),
        SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_serialize_container(
            field_fixed_sizes,
            3u,
            &container_codec,
            out_b,
            sizeof(out_b),
            &len_b),
        SSZ_SUCCESS);
    ASSERT_TRUE(len_a == len_b);
    ASSERT_MEM_EQ(out_a, out_b, len_a);

    ASSERT_ERR(
        ssz_serialize_list_fixed(
            fixed_elements,
            2u,
            SSZ_NO_LIMIT,
            2u,
            out_a,
            sizeof(out_a),
            &len_a),
        SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_serialize_list_fixed(
            fixed_elements,
            2u,
            SSZ_NO_LIMIT,
            2u,
            out_b,
            sizeof(out_b),
            &len_b),
        SSZ_SUCCESS);
    ASSERT_TRUE(len_a == len_b);
    ASSERT_MEM_EQ(out_a, out_b, len_a);

    ASSERT_ERR(
        ssz_serialize_list_variable(2u, SSZ_NO_LIMIT, &list_codec, out_a, sizeof(out_a), &len_a),
        SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_serialize_list_variable(2u, SSZ_NO_LIMIT, &list_codec, out_b, sizeof(out_b), &len_b),
        SSZ_SUCCESS);
    ASSERT_TRUE(len_a == len_b);
    ASSERT_MEM_EQ(out_a, out_b, len_a);

    ASSERT_ERR(
        ssz_serialize_bitlist(bits, sizeof(bits), 9u, SSZ_NO_LIMIT, out_a, sizeof(out_a), &len_a),
        SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_serialize_bitlist(bits, sizeof(bits), 9u, SSZ_NO_LIMIT, out_b, sizeof(out_b), &len_b),
        SSZ_SUCCESS);
    ASSERT_TRUE(len_a == len_b);
    ASSERT_MEM_EQ(out_a, out_b, len_a);

    return true;
}

static bool test_serialize_error_cases(void)
{
    uint8_t out8 = 0u;
    uint8_t out16[2] = {0u};
    uint8_t out32[4] = {0u};
    uint8_t out64[8] = {0u};
    uint8_t out128[16] = {0u};
    uint8_t out256[32] = {0u};
    const uint8_t in128[16] = {0u};
    const uint8_t in256[32] = {0u};

    ASSERT_ERR(ssz_serialize_uint8(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint16(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint32(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint64(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint128(in128, sizeof(in128), NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint256(in256, sizeof(in256), NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_boolean(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(ssz_serialize_uint128(NULL, 16u, out128), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_serialize_uint256(NULL, 32u, out256), SSZ_ERR_INVALID_ARGUMENT);

    size_t out_len = 0u;
    ASSERT_ERR(
        ssz_serialize_vector_fixed(
            (const uint8_t[]){0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u},
            3u,
            2u,
            out32,
            sizeof(out32),
            &out_len),
        SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_TRUE(out_len == 6u);

    ASSERT_ERR(
        ssz_serialize_vector_fixed((const uint8_t[]){0x01u}, 0u, 1u, &out8, 1u, &out_len),
        SSZ_ERR_SCHEMA_INVALID);

    ASSERT_ERR(
        ssz_serialize_bitvector((const uint8_t[]){0x00u}, 1u, 0u, &out8, 1u, &out_len),
        SSZ_ERR_SCHEMA_INVALID);

    ASSERT_ERR(
        ssz_serialize_vector_fixed((const uint8_t[]){0x00u}, 1u, 1u, &out8, 1u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(ssz_serialize_uint8(0x12u, &out8), SSZ_SUCCESS);
    ASSERT_TRUE(out8 == 0x12u);
    ASSERT_ERR(ssz_serialize_uint16(0x3412u, out16), SSZ_SUCCESS);
    ASSERT_ERR(ssz_serialize_uint32(0x78563412u, out32), SSZ_SUCCESS);
    ASSERT_ERR(ssz_serialize_uint64(UINT64_C(0x0102030405060708), out64), SSZ_SUCCESS);

    return true;
}

static bool test_serialize_bitvector_and_bitlist_error_paths(void)
{
    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    const uint8_t bitvector_ok[1] = {0x01u};
    ASSERT_ERR(
        ssz_serialize_bitvector(
            bitvector_ok,
            sizeof(bitvector_ok),
            UINT64_MAX,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
    ASSERT_ERR(
        ssz_serialize_bitvector(NULL, 0u, 8u, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_serialize_bitvector(bitvector_ok, sizeof(bitvector_ok), 8u, out, sizeof(out), NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    const uint8_t bitlist_bad_padding[2] = {0xAAu, 0x82u};
    ASSERT_ERR(
        ssz_serialize_bitlist(
            bitvector_ok,
            sizeof(bitvector_ok),
            UINT64_MAX,
            SSZ_NO_LIMIT,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
    ASSERT_ERR(
        ssz_serialize_bitlist(NULL, 0u, 1u, SSZ_NO_LIMIT, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_serialize_bitlist(
            bitlist_bad_padding,
            sizeof(bitlist_bad_padding),
            9u,
            SSZ_NO_LIMIT,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_serialize_bitlist(
            bitvector_ok,
            sizeof(bitvector_ok),
            1u,
            SSZ_NO_LIMIT,
            out,
            sizeof(out),
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_serialize_fixed_collection_error_paths(void)
{
    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_vector_fixed((const uint8_t[]){0x11u}, 1u, 0u, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
#if SIZE_MAX < UINT64_MAX
    ASSERT_ERR(
        ssz_serialize_vector_fixed(
            (const uint8_t[]){0x11u},
            UINT64_MAX,
            1u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_serialize_vector_fixed(
            (const uint8_t[]){0x11u},
            ((uint64_t)SIZE_MAX / 2u) + 1u,
            2u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif
    ASSERT_ERR(
        ssz_serialize_vector_fixed(NULL, 1u, 1u, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_serialize_list_fixed(
            (const uint8_t[]){0x11u},
            1u,
            SSZ_NO_LIMIT,
            0u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SCHEMA_INVALID);
#if SIZE_MAX < UINT64_MAX
    ASSERT_ERR(
        ssz_serialize_list_fixed(
            (const uint8_t[]){0x11u},
            UINT64_MAX,
            SSZ_NO_LIMIT,
            1u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_serialize_list_fixed(
            (const uint8_t[]){0x11u},
            ((uint64_t)SIZE_MAX / 2u) + 1u,
            SSZ_NO_LIMIT,
            2u,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif
    ASSERT_ERR(
        ssz_serialize_list_fixed(NULL, 1u, SSZ_NO_LIMIT, 1u, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_serialize_list_fixed(
            (const uint8_t[]){0x11u},
            1u,
            SSZ_NO_LIMIT,
            1u,
            out,
            sizeof(out),
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_serialize_vector_variable_error_paths(void)
{
    uint8_t out[16] = {0u};
    size_t out_len = 0u;

    const size_t simple_lengths[1] = {1u};
    const ssz_error_t simple_errors[1] = {SSZ_SUCCESS};
    scripted_write_ctx_t simple_ctx = {
        .lengths = simple_lengths,
        .errors = simple_errors,
        .step_count = 1u,
        .step_index = 0u,
        .fill = 0xA1u,
    };
    ssz_member_codec_t simple_codec = make_scripted_codec(&simple_ctx);

    ASSERT_ERR(
        ssz_serialize_vector_variable(0u, &simple_codec, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX < UINT64_MAX
    ASSERT_ERR(
        ssz_serialize_vector_variable(UINT64_MAX, &simple_codec, out, sizeof(out), &out_len),
        SSZ_ERR_OVERFLOW);
#endif
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_serialize_vector_variable(
            ((uint64_t)SIZE_MAX / SSZ_BYTES_PER_LENGTH_OFFSET) + 1u,
            &simple_codec,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif

    const size_t query_err_lengths[1] = {0u};
    const ssz_error_t query_err_errors[1] = {SSZ_ERR_TYPE_MISMATCH};
    scripted_write_ctx_t query_err_ctx = {
        .lengths = query_err_lengths,
        .errors = query_err_errors,
        .step_count = 1u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t query_err_codec = make_scripted_codec(&query_err_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &query_err_codec, out, sizeof(out), &out_len),
        SSZ_ERR_TYPE_MISMATCH);

    const size_t add_overflow_lengths[1] = {SIZE_MAX};
    const ssz_error_t add_overflow_errors[1] = {SSZ_SUCCESS};
    scripted_write_ctx_t add_overflow_ctx = {
        .lengths = add_overflow_lengths,
        .errors = add_overflow_errors,
        .step_count = 1u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t add_overflow_codec = make_scripted_codec(&add_overflow_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &add_overflow_codec, out, sizeof(out), &out_len),
        SSZ_ERR_OVERFLOW);

    const size_t uint32_overflow_lengths[1] = {(size_t)UINT32_MAX};
    const ssz_error_t uint32_overflow_errors[1] = {SSZ_SUCCESS};
    scripted_write_ctx_t uint32_overflow_ctx = {
        .lengths = uint32_overflow_lengths,
        .errors = uint32_overflow_errors,
        .step_count = 1u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t uint32_overflow_codec = make_scripted_codec(&uint32_overflow_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &uint32_overflow_codec, out, sizeof(out), &out_len),
        SSZ_ERR_OVERFLOW);

    const size_t prepare_err_lengths[1] = {1u};
    const ssz_error_t prepare_err_errors[1] = {SSZ_SUCCESS};
    scripted_write_ctx_t prepare_err_ctx = {
        .lengths = prepare_err_lengths,
        .errors = prepare_err_errors,
        .step_count = 1u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t prepare_err_codec = make_scripted_codec(&prepare_err_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &prepare_err_codec, out, sizeof(out), NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    const size_t second_query_err_lengths[2] = {1u, 0u};
    const ssz_error_t second_query_err_errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
    scripted_write_ctx_t second_query_err_ctx = {
        .lengths = second_query_err_lengths,
        .errors = second_query_err_errors,
        .step_count = 2u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t second_query_err_codec = make_scripted_codec(&second_query_err_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &second_query_err_codec, out, sizeof(out), &out_len),
        SSZ_ERR_TYPE_MISMATCH);

    const size_t write_err_lengths[3] = {1u, 1u, 0u};
    const ssz_error_t write_err_errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
    scripted_write_ctx_t write_err_ctx = {
        .lengths = write_err_lengths,
        .errors = write_err_errors,
        .step_count = 3u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t write_err_codec = make_scripted_codec(&write_err_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &write_err_codec, out, sizeof(out), &out_len),
        SSZ_ERR_TYPE_MISMATCH);

    const size_t mismatch_lengths[3] = {1u, 2u, 1u};
    const ssz_error_t mismatch_errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
    scripted_write_ctx_t mismatch_ctx = {
        .lengths = mismatch_lengths,
        .errors = mismatch_errors,
        .step_count = 3u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t mismatch_codec = make_scripted_codec(&mismatch_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &mismatch_codec, out, sizeof(out), &out_len),
        SSZ_ERR_TYPE_MISMATCH);

    const size_t cursor_overflow_lengths[3] = {1u, SIZE_MAX, SIZE_MAX};
    const ssz_error_t cursor_overflow_errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
    scripted_write_ctx_t cursor_overflow_ctx = {
        .lengths = cursor_overflow_lengths,
        .errors = cursor_overflow_errors,
        .step_count = 3u,
        .step_index = 0u,
        .fill = 0u,
    };
    ssz_member_codec_t cursor_overflow_codec = make_scripted_codec(&cursor_overflow_ctx);
    ASSERT_ERR(
        ssz_serialize_vector_variable(1u, &cursor_overflow_codec, out, sizeof(out), &out_len),
        SSZ_ERR_OVERFLOW);

    /* Callback returns different length between first and second pass, causing
       cursor != total after the write loop. Covers ssz_serialize.c:390. */
    {
        /* 1 element: first-pass query returns 2, second-pass query returns 1,
           write returns 1. total = 4 + 2 = 6, cursor = 4 + 1 = 5. */
        const size_t drift_lengths[3] = {2u, 1u, 1u};
        const ssz_error_t drift_errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t drift_ctx = {
            .lengths = drift_lengths,
            .errors = drift_errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0xAAu,
        };
        ssz_member_codec_t drift_codec = make_scripted_codec(&drift_ctx);
        ASSERT_ERR(
            ssz_serialize_vector_variable(1u, &drift_codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    return true;
}

static bool test_serialize_list_variable_error_paths(void)
{
    uint8_t out[16] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX < UINT64_MAX
    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(
                UINT64_MAX,
                SSZ_NO_LIMIT,
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
#if SIZE_MAX > UINT32_MAX
        ASSERT_ERR(
            ssz_serialize_list_variable(
                ((uint64_t)SIZE_MAX / SSZ_BYTES_PER_LENGTH_OFFSET) + 1u,
                SSZ_NO_LIMIT,
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_OVERFLOW);
#endif
    }

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const size_t lengths[2] = {1u, 0u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, 1u, 0u};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, 2u, 1u};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, SIZE_MAX, SIZE_MAX};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    return true;
}

static bool test_serialize_list_variable_out_buffer_edge_paths(void)
{
    uint8_t out[16] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_list_variable(
            2u,
            SSZ_NO_LIMIT,
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = fail_if_called_write,
                                  .read = NULL,
                                  .root = NULL},
            out,
            7u,
            &out_len),
        SSZ_ERR_BUFFER_TOO_SMALL);

#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_serialize_list_variable(
            ((uint64_t)UINT32_MAX / SSZ_BYTES_PER_LENGTH_OFFSET) + 1u,
            SSZ_NO_LIMIT,
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = fail_if_called_write,
                                  .read = NULL,
                                  .root = NULL},
            out,
            SIZE_MAX,
            &out_len),
        SSZ_ERR_OVERFLOW);
#endif

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(2u, SSZ_NO_LIMIT, &codec, out, 8u, &out_len),
            SSZ_ERR_BUFFER_TOO_SMALL);
    }

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, 4u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, 4u, &out_len),
            SSZ_ERR_OVERFLOW);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, out, 4u, &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    return true;
}

static bool test_serialize_list_variable_size_query_edge_paths(void)
{
    size_t out_len = 0u;

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, NULL, 0u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, NULL, 0u, &out_len),
            SSZ_ERR_OVERFLOW);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, NULL, 0u, &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec, NULL, 0u, NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    return true;
}

static bool test_serialize_container_error_paths(void)
{
    uint8_t out[32] = {0u};
    size_t out_len = 0u;

    const size_t one_variable[1] = {0u};

    ASSERT_ERR(
        ssz_serialize_container(NULL, 1u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_container(one_variable, 0u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_container(one_variable, 1u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);

    const size_t overflow_fixed_region[2] = {SIZE_MAX, 1u};
    ASSERT_ERR(
        ssz_serialize_container(
            overflow_fixed_region,
            2u,
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = fail_if_called_write,
                                  .read = NULL,
                                  .root = NULL},
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_OVERFLOW);

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t fixed_sizes[1] = {2u};
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(fixed_sizes, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const size_t lengths[2] = {1u, 0u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, 1u, 0u};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, 2u, 1u};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, SIZE_MAX, SIZE_MAX};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t fixed_sizes[1] = {1u};
        const size_t lengths[2] = {1u, 0u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(fixed_sizes, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t fixed_sizes[1] = {1u};
        const size_t lengths[2] = {1u, 0u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(fixed_sizes, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    {
        const size_t lengths[3] = {1u, 0u, 0u};
        const ssz_error_t errors[3] = {SSZ_SUCCESS, SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 3u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(one_variable, 1u, &codec, out, sizeof(out), &out_len),
            SSZ_SUCCESS);
    }

    return true;
}

static bool test_serialize_container_out_buffer_edge_paths(void)
{
    uint8_t out[16] = {0u};
    size_t out_len = 0u;

    {
        const size_t field_fixed_sizes[2] = {0u, 1u};
        ASSERT_ERR(
            ssz_serialize_container(
                field_fixed_sizes,
                2u,
                &(ssz_member_codec_t){.ctx = NULL,
                                      .write = fail_if_called_write,
                                      .read = NULL,
                                      .root = NULL},
                out,
                4u,
                &out_len),
            SSZ_ERR_BUFFER_TOO_SMALL);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t field_fixed_sizes[2] = {(size_t)UINT32_MAX, 1u};
        ASSERT_ERR(
            ssz_serialize_container(
                field_fixed_sizes,
                2u,
                &(ssz_member_codec_t){.ctx = NULL,
                                      .write = fail_if_called_write,
                                      .read = NULL,
                                      .root = NULL},
                out,
                SIZE_MAX,
                &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    {
        const size_t field_fixed_sizes[2] = {0u, 0u};
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 2u, &codec, out, 8u, &out_len),
            SSZ_ERR_BUFFER_TOO_SMALL);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t field_fixed_sizes[2] = {0u, 0u};
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 2u, &codec, out, 8u, &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, out, 4u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, out, 4u, &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t field_fixed_sizes[1] = {1u};
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, out, 1u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t field_fixed_sizes[1] = {2u};
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, out, 2u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, out, 4u, &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    return true;
}

static bool test_serialize_container_size_query_edge_paths(void)
{
    size_t out_len = 0u;

    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, NULL, 0u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, NULL, 0u, &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t field_fixed_sizes[1] = {2u};
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, NULL, 0u, &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

#if SIZE_MAX > UINT32_MAX
    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {(size_t)UINT32_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, NULL, 0u, &out_len),
            SSZ_ERR_OVERFLOW);
    }
#endif

    {
        const size_t field_fixed_sizes[1] = {0u};
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_container(field_fixed_sizes, 1u, &codec, NULL, 0u, NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    return true;
}

static bool test_serialize_union_error_paths(void)
{
    uint8_t out[8] = {0u};
    size_t out_len = 0u;

    ASSERT_ERR(
        ssz_serialize_union(0u, 0u, false, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_union(0u, 257u, false, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_union(0u, 1u, true, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_union(3u, 3u, false, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SELECTOR_INVALID);
    ASSERT_ERR(
        ssz_serialize_union(1u, 3u, false, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_INVALID_ARGUMENT);

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_union(1u, 3u, false, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_union(1u, 3u, false, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_union(1u, 3u, false, &codec, out, sizeof(out), NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const size_t lengths[2] = {1u, 1u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_union(1u, 3u, false, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[2] = {2u, 1u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_union(1u, 3u, false, &codec, out, sizeof(out), &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    return true;
}

static bool test_serialize_compatible_union_error_paths(void)
{
    uint8_t out[8] = {0u};
    size_t out_len = 0u;
    const uint8_t allowed_valid[] = {1u, 2u};

    ASSERT_ERR(
        ssz_serialize_compatible_union(1u, NULL, 1u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_compatible_union(1u, allowed_valid, 0u, NULL, out, sizeof(out), &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_compatible_union(
            1u,
            (const uint8_t[]){0u, 2u},
            2u,
            NULL,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_serialize_compatible_union(
            1u,
            (const uint8_t[]){1u, 200u},
            2u,
            NULL,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_SCHEMA_INVALID);

    ASSERT_ERR(
        ssz_serialize_compatible_union(
            1u,
            allowed_valid,
            sizeof(allowed_valid),
            NULL,
            out,
            sizeof(out),
            &out_len),
        SSZ_ERR_INVALID_ARGUMENT);

    {
        const size_t lengths[1] = {0u};
        const ssz_error_t errors[1] = {SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_compatible_union(
                1u,
                allowed_valid,
                sizeof(allowed_valid),
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[1] = {SIZE_MAX};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_compatible_union(
                1u,
                allowed_valid,
                sizeof(allowed_valid),
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_OVERFLOW);
    }

    {
        const size_t lengths[1] = {1u};
        const ssz_error_t errors[1] = {SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 1u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_compatible_union(
                1u,
                allowed_valid,
                sizeof(allowed_valid),
                &codec,
                out,
                sizeof(out),
                NULL),
            SSZ_ERR_INVALID_ARGUMENT);
    }

    {
        const size_t lengths[2] = {1u, 1u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_compatible_union(
                1u,
                allowed_valid,
                sizeof(allowed_valid),
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    {
        const size_t lengths[2] = {2u, 1u};
        const ssz_error_t errors[2] = {SSZ_SUCCESS, SSZ_SUCCESS};
        scripted_write_ctx_t ctx = {
            .lengths = lengths,
            .errors = errors,
            .step_count = 2u,
            .step_index = 0u,
            .fill = 0u,
        };
        ssz_member_codec_t codec = make_scripted_codec(&ctx);
        ASSERT_ERR(
            ssz_serialize_compatible_union(
                1u,
                allowed_valid,
                sizeof(allowed_valid),
                &codec,
                out,
                sizeof(out),
                &out_len),
            SSZ_ERR_TYPE_MISMATCH);
    }

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"serialize_basic_types_and_aliases", test_serialize_basic_types_and_aliases},
        {"serialize_boolean_canonical_rejection", test_serialize_boolean_canonical_rejection},
        {"serialize_fixed_width_exact_and_invalid_lengths",
         test_serialize_fixed_width_exact_and_invalid_lengths},
        {"serialize_bitvector_and_padding_validation",
         test_serialize_bitvector_and_padding_validation},
        {"serialize_bitlist_delimiter_and_limit", test_serialize_bitlist_delimiter_and_limit},
        {"serialize_vector_fixed_and_size_query", test_serialize_vector_fixed_and_size_query},
        {"serialize_vector_variable_and_size_query", test_serialize_vector_variable_and_size_query},
        {"serialize_list_fixed_and_limits", test_serialize_list_fixed_and_limits},
        {"serialize_list_variable_and_limits", test_serialize_list_variable_and_limits},
        {"serialize_container_mixed_fields_and_size_query",
         test_serialize_container_mixed_fields_and_size_query},
        {"serialize_union_none_normal_and_size_query",
         test_serialize_union_none_normal_and_size_query},
        {"serialize_compatible_union_valid_and_invalid_selectors",
         test_serialize_compatible_union_valid_and_invalid_selectors},
        {"serialize_direct_calls", test_serialize_direct_calls},
        {"serialize_error_cases", test_serialize_error_cases},
        {"serialize_bitvector_and_bitlist_error_paths",
         test_serialize_bitvector_and_bitlist_error_paths},
        {"serialize_fixed_collection_error_paths", test_serialize_fixed_collection_error_paths},
        {"serialize_vector_variable_error_paths", test_serialize_vector_variable_error_paths},
        {"serialize_list_variable_error_paths", test_serialize_list_variable_error_paths},
        {"serialize_list_variable_out_buffer_edge_paths",
         test_serialize_list_variable_out_buffer_edge_paths},
        {"serialize_list_variable_size_query_edge_paths",
         test_serialize_list_variable_size_query_edge_paths},
        {"serialize_container_error_paths", test_serialize_container_error_paths},
        {"serialize_container_out_buffer_edge_paths",
         test_serialize_container_out_buffer_edge_paths},
        {"serialize_container_size_query_edge_paths",
         test_serialize_container_size_query_edge_paths},
        {"serialize_union_error_paths", test_serialize_union_error_paths},
        {"serialize_compatible_union_error_paths", test_serialize_compatible_union_error_paths},
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

    printf("[OK] %zu/%zu serialize tests passed\n", passed, total);
    return 0;
}
