#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ssz.h"

#define CONTAINER_SCHEMA(field_fixed_sizes_value, field_count_value)                              \
    (&(const ssz_container_schema_t){                                                             \
        .field_fixed_sizes = (field_fixed_sizes_value),                                           \
        .field_count = (field_count_value),                                                       \
    })

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
    const uint8_t *expected;
    size_t expected_len;
    bool seen;
} read_expect_entry_t;

typedef struct
{
    read_expect_entry_t *entries;
    size_t entry_count;
} read_expect_ctx_t;

static ssz_error_t read_expect_callback(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    read_expect_ctx_t *expect_ctx = (read_expect_ctx_t *)ctx;

    if (expect_ctx == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < expect_ctx->entry_count; i++)
    {
        if (expect_ctx->entries[i].id != member_id)
        {
            continue;
        }

        if (expect_ctx->entries[i].expected_len != data_len)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        if ((data_len != 0u) && ((data == NULL) || (expect_ctx->entries[i].expected == NULL) ||
                                 (memcmp(data, expect_ctx->entries[i].expected, data_len) != 0)))
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }

        expect_ctx->entries[i].seen = true;
        return SSZ_SUCCESS;
    }

    return SSZ_ERR_INVALID_ARGUMENT;
}

static bool all_read_entries_seen(const read_expect_ctx_t *ctx)
{
    for (size_t i = 0u; i < ctx->entry_count; i++)
    {
        if (!ctx->entries[i].seen)
        {
            return false;
        }
    }
    return true;
}

static ssz_error_t fail_if_called_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)ctx;
    (void)member_id;
    (void)data;
    (void)data_len;
    return SSZ_ERR_TYPE_MISMATCH;
}

typedef struct
{
    const ssz_error_t *errors;
    size_t step_count;
    size_t step_index;
} scripted_read_ctx_t;

static ssz_error_t scripted_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    scripted_read_ctx_t *script = (scripted_read_ctx_t *)ctx;
    (void)member_id;
    (void)data;
    (void)data_len;

    if ((script == NULL) || (script->errors == NULL) || (script->step_index >= script->step_count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    return script->errors[script->step_index++];
}

static ssz_member_codec_t make_scripted_read_codec(scripted_read_ctx_t *ctx)
{
    ssz_member_codec_t codec = {
        .ctx = ctx,
        .write = NULL,
        .read = scripted_read,
        .root = NULL,
    };
    return codec;
}

typedef struct
{
    uint8_t value[32];
    bool invoked;
} union_uint256_read_ctx_t;

static ssz_error_t union_uint256_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    union_uint256_read_ctx_t *read_ctx = (union_uint256_read_ctx_t *)ctx;

    if (read_ctx == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id != 1u)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    read_ctx->invoked = true;
    return ssz_deserialize_uint256(data, data_len, read_ctx->value);
}

typedef struct
{
    size_t *sizes;
} schema_mutating_read_ctx_t;

typedef struct
{
    size_t *sizes;
    bool second_seen;
} schema_wrap_read_ctx_t;

/* Callback that mutates field_fixed_sizes[1] on the first read, so the loop
   advances cursor by less than the precomputed fixed_region. */
static ssz_error_t schema_mutating_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)data;
    (void)data_len;
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    if (member_id == 0u)
    {
        m->sizes[1] = 1u;
    }
    return SSZ_SUCCESS;
}

/* Callback that mutates a later fixed field size to SIZE_MAX. The hardened
   container code must reject the wrapped cursor advance before invoking the
   later field callback. */
static ssz_error_t schema_wrap_fixed_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    schema_wrap_read_ctx_t *m = (schema_wrap_read_ctx_t *)ctx;
    (void)data;
    (void)data_len;

    if (m == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == 0u)
    {
        m->sizes[1] = SIZE_MAX;
    }
    else if (member_id == 1u)
    {
        m->second_seen = true;
    }

    return SSZ_SUCCESS;
}

/* Callback that inflates a fixed field size during the second pass to trigger
   the cursor + fixed_size > fixed_region guard. */
static ssz_error_t schema_inflate_fixed_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)data;
    (void)data_len;
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    /* After the first pass, inflate field 1 so the second pass overflows. */
    if (member_id == 0u)
    {
        m->sizes[1] = 9999u;
    }
    return SSZ_SUCCESS;
}

/* Callback that zeroes a fixed field (making it variable) during the second
   pass, so cursor + OFFSET > fixed_region when hitting the variable slot. */
static ssz_error_t schema_zero_fixed_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)data;
    (void)data_len;
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    if (member_id == 0u)
    {
        m->sizes[1] = 0u;
    }
    return SSZ_SUCCESS;
}

static ssz_error_t schema_inflate_look_cursor_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)data;
    (void)data_len;
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    if (member_id == 0u)
    {
        m->sizes[2] = 9999u;
    }
    return SSZ_SUCCESS;
}

static ssz_error_t schema_wrap_look_cursor_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    (void)data;
    (void)data_len;

    if (m == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == 0u)
    {
        m->sizes[2] = SIZE_MAX;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t schema_force_end_lt_start_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    (void)data;
    (void)data_len;
    schema_mutating_read_ctx_t *m = (schema_mutating_read_ctx_t *)ctx;
    if (member_id == 0u)
    {
        m->sizes[1] = 0u;
    }
    return SSZ_SUCCESS;
}

static bool test_deserialize_basic_round_trips(void)
{
    uint8_t out_u8 = 0u;
    uint16_t out_u16 = 0u;
    uint32_t out_u32 = 0u;
    uint64_t out_u64 = 0u;
    uint8_t out_u128[16] = {0u};
    uint8_t out_u256[32] = {0u};
    uint8_t out_bool = 0u;
    uint8_t out_byte = 0u;
    uint8_t out_bit = 0u;

    uint8_t in_u8[1] = {0u};
    uint8_t in_u16[2] = {0u};
    uint8_t in_u32[4] = {0u};
    uint8_t in_u64[8] = {0u};
    uint8_t in_u128[16] = {
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
    uint8_t in_u256[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    };
    uint8_t in_bool[1] = {0u};

    ASSERT_ERR(ssz_serialize_uint8(0xABu, in_u8), SSZ_SUCCESS);
    ASSERT_ERR(ssz_deserialize_uint8(in_u8, sizeof(in_u8), &out_u8), SSZ_SUCCESS);
    ASSERT_TRUE(out_u8 == 0xABu);

    ASSERT_ERR(ssz_serialize_uint16(UINT16_C(0xBEEF), in_u16), SSZ_SUCCESS);
    ASSERT_ERR(ssz_deserialize_uint16(in_u16, sizeof(in_u16), &out_u16), SSZ_SUCCESS);
    ASSERT_TRUE(out_u16 == UINT16_C(0xBEEF));

    ASSERT_ERR(ssz_serialize_uint32(UINT32_C(0x78563412), in_u32), SSZ_SUCCESS);
    ASSERT_ERR(ssz_deserialize_uint32(in_u32, sizeof(in_u32), &out_u32), SSZ_SUCCESS);
    ASSERT_TRUE(out_u32 == UINT32_C(0x78563412));

    ASSERT_ERR(ssz_serialize_uint64(UINT64_C(0x0102030405060708), in_u64), SSZ_SUCCESS);
    ASSERT_ERR(ssz_deserialize_uint64(in_u64, sizeof(in_u64), &out_u64), SSZ_SUCCESS);
    ASSERT_TRUE(out_u64 == UINT64_C(0x0102030405060708));

    ASSERT_ERR(ssz_deserialize_uint128(in_u128, sizeof(in_u128), out_u128), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out_u128, in_u128, sizeof(in_u128));

    ASSERT_ERR(ssz_deserialize_uint256(in_u256, sizeof(in_u256), out_u256), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out_u256, in_u256, sizeof(in_u256));

    ASSERT_ERR(ssz_serialize_boolean(1u, in_bool), SSZ_SUCCESS);
    ASSERT_ERR(ssz_deserialize_boolean(in_bool, sizeof(in_bool), &out_bool), SSZ_SUCCESS);
    ASSERT_TRUE(out_bool == 1u);

    ASSERT_ERR(ssz_deserialize_uint8(in_u8, sizeof(in_u8), &out_byte), SSZ_SUCCESS);
    ASSERT_TRUE(out_byte == 0xABu);

    ASSERT_ERR(ssz_deserialize_boolean(in_bool, sizeof(in_bool), &out_bit), SSZ_SUCCESS);
    ASSERT_TRUE(out_bit == 1u);

    return true;
}

static bool test_deserialize_boolean_canonical_enforcement(void)
{
    uint8_t out = 0u;

    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0x00u}, 1u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 0u);

    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0x01u}, 1u, &out), SSZ_SUCCESS);
    ASSERT_TRUE(out == 1u);

    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0x02u}, 1u, &out), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0x80u}, 1u, &out), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0xFFu}, 1u, &out), SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_deserialize_boolean_collection_canonical_enforcement(void)
{
    const uint8_t valid_vector[3] = {0x00u, 0x01u, 0x00u};
    const uint8_t invalid_vector[3] = {0x00u, 0x02u, 0x01u};
    const uint8_t valid_list[2] = {0x01u, 0x00u};
    const uint8_t invalid_list[2] = {0x01u, 0xFFu};
    uint8_t out[3] = {0u};
    uint64_t count = 0u;

    ASSERT_ERR(
        ssz_deserialize_vector_boolean(valid_vector, sizeof(valid_vector), 3u, out, sizeof(out)),
        SSZ_SUCCESS);
    ASSERT_MEM_EQ(out, valid_vector, sizeof(valid_vector));

    memset(out, 0xA5, sizeof(out));
    ASSERT_ERR(
        ssz_deserialize_vector_boolean(invalid_vector, sizeof(invalid_vector), 3u, out, sizeof(out)),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_MEM_EQ(out, ((const uint8_t[3]){0xA5u, 0xA5u, 0xA5u}), sizeof(out));

    ASSERT_ERR(
        ssz_deserialize_list_boolean(valid_list, sizeof(valid_list), 2u, out, sizeof(out), &count),
        SSZ_SUCCESS);
    ASSERT_TRUE(count == 2u);
    ASSERT_MEM_EQ(out, valid_list, sizeof(valid_list));

    memset(out, 0x5A, sizeof(out));
    count = 99u;
    ASSERT_ERR(
        ssz_deserialize_list_boolean(
            invalid_list,
            sizeof(invalid_list),
            2u,
            out,
            sizeof(out),
            &count),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_TRUE(count == 99u);
    ASSERT_MEM_EQ(out, ((const uint8_t[3]){0x5Au, 0x5Au, 0x5Au}), sizeof(out));

    count = 99u;
    ASSERT_ERR(ssz_deserialize_list_boolean(NULL, 0u, SSZ_NO_LIMIT, NULL, 0u, &count), SSZ_SUCCESS);
    ASSERT_TRUE(count == 0u);

    return true;
}

static bool test_deserialize_fixed_width_short_inputs(void)
{
    const uint8_t in_u8[1] = {0xABu};
    const uint8_t in_u16[1] = {0xEFu};
    const uint8_t in_u32[3] = {0x12u, 0x34u, 0x56u};
    const uint8_t in_u64[7] = {0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u};
    const uint8_t in_u128[15] = {
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
    const uint8_t in_u256[31] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu,
    };
    uint8_t out_u8 = 0x11u;
    uint16_t out_u16 = UINT16_C(0x2233);
    uint32_t out_u32 = UINT32_C(0x44556677);
    uint64_t out_u64 = UINT64_C(0x8899AABBCCDDEEFF);
    uint8_t out_u128[16];
    uint8_t out_u256[32];
    uint8_t out_bool = 0x44u;
    const uint8_t expected_u8 = 0x11u;
    const uint16_t expected_u16 = UINT16_C(0x2233);
    const uint32_t expected_u32 = UINT32_C(0x44556677);
    const uint64_t expected_u64 = UINT64_C(0x8899AABBCCDDEEFF);
    uint8_t expected_u128[16];
    uint8_t expected_u256[32];
    const uint8_t expected_bool = 0x44u;

    memset(out_u128, 0xA5, sizeof(out_u128));
    memset(out_u256, 0x5A, sizeof(out_u256));
    memset(expected_u128, 0xA5, sizeof(expected_u128));
    memset(expected_u256, 0x5A, sizeof(expected_u256));

    ASSERT_ERR(ssz_deserialize_uint8(in_u8, 0u, &out_u8), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_deserialize_uint16(in_u16, sizeof(in_u16), &out_u16), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_deserialize_uint32(in_u32, sizeof(in_u32), &out_u32), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_deserialize_uint64(in_u64, sizeof(in_u64), &out_u64), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(
        ssz_deserialize_uint128(in_u128, sizeof(in_u128), out_u128),
        SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(
        ssz_deserialize_uint256(in_u256, sizeof(in_u256), out_u256),
        SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_deserialize_boolean(in_u8, 0u, &out_bool), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_TRUE(out_u8 == expected_u8);
    ASSERT_TRUE(out_u16 == expected_u16);
    ASSERT_TRUE(out_u32 == expected_u32);
    ASSERT_TRUE(out_u64 == expected_u64);
    ASSERT_MEM_EQ(out_u128, expected_u128, sizeof(out_u128));
    ASSERT_MEM_EQ(out_u256, expected_u256, sizeof(out_u256));
    ASSERT_TRUE(out_bool == expected_bool);

    return true;
}

static bool test_deserialize_fixed_width_overlong_inputs(void)
{
    const uint8_t in_u8[2] = {0xABu, 0xCDu};
    const uint8_t in_u16[3] = {0xEFu, 0xBEu, 0xAAu};
    const uint8_t in_u32[5] = {0x12u, 0x34u, 0x56u, 0x78u, 0x9Au};
    const uint8_t in_u64[9] = {0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u, 0x99u};
    const uint8_t in_u128[17] = {
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
        0xA5u,
    };
    const uint8_t in_u256[33] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu, 0xA5u,
    };
    const uint8_t in_bool[2] = {0x01u, 0xA5u};
    uint8_t out_u8 = 0x11u;
    uint16_t out_u16 = UINT16_C(0x2233);
    uint32_t out_u32 = UINT32_C(0x44556677);
    uint64_t out_u64 = UINT64_C(0x8899AABBCCDDEEFF);
    uint8_t out_u128[16];
    uint8_t out_u256[32];
    uint8_t out_bool = 0x44u;
    const uint8_t expected_u8 = 0x11u;
    const uint16_t expected_u16 = UINT16_C(0x2233);
    const uint32_t expected_u32 = UINT32_C(0x44556677);
    const uint64_t expected_u64 = UINT64_C(0x8899AABBCCDDEEFF);
    uint8_t expected_u128[16];
    uint8_t expected_u256[32];
    const uint8_t expected_bool = 0x44u;

    memset(out_u128, 0xA5, sizeof(out_u128));
    memset(out_u256, 0x5A, sizeof(out_u256));
    memset(expected_u128, 0xA5, sizeof(expected_u128));
    memset(expected_u256, 0x5A, sizeof(expected_u256));

    ASSERT_ERR(ssz_deserialize_uint8(in_u8, sizeof(in_u8), &out_u8), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_deserialize_uint16(in_u16, sizeof(in_u16), &out_u16), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_deserialize_uint32(in_u32, sizeof(in_u32), &out_u32), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_deserialize_uint64(in_u64, sizeof(in_u64), &out_u64), SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_uint128(in_u128, sizeof(in_u128), out_u128),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_uint256(in_u256, sizeof(in_u256), out_u256),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_boolean(in_bool, sizeof(in_bool), &out_bool),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_TRUE(out_u8 == expected_u8);
    ASSERT_TRUE(out_u16 == expected_u16);
    ASSERT_TRUE(out_u32 == expected_u32);
    ASSERT_TRUE(out_u64 == expected_u64);
    ASSERT_MEM_EQ(out_u128, expected_u128, sizeof(out_u128));
    ASSERT_MEM_EQ(out_u256, expected_u256, sizeof(out_u256));
    ASSERT_TRUE(out_bool == expected_bool);

    return true;
}

static bool test_deserialize_bitvector_padding_validation(void)
{
    const uint8_t valid[2] = {0x55u, 0x01u};
    const uint8_t invalid_padding[2] = {0x55u, 0x81u};
    uint8_t out[2] = {0u};

    ASSERT_ERR(ssz_deserialize_bitvector(valid, sizeof(valid), 10u, out, sizeof(out)), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out, valid, sizeof(valid));

    ASSERT_ERR(
        ssz_deserialize_bitvector(invalid_padding, sizeof(invalid_padding), 10u, out, sizeof(out)),
        SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_deserialize_bitlist_delimiter_and_errors(void)
{
    const uint8_t encoded[2] = {0xAAu, 0x06u};
    uint8_t out_bits[2] = {0u};
    uint64_t out_bit_len = 0u;

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            encoded,
            sizeof(encoded),
            10u,
            out_bits,
            sizeof(out_bits),
            &out_bit_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_bit_len == 10u);
    ASSERT_MEM_EQ(out_bits, ((const uint8_t[2]){0xAAu, 0x02u}), 2u);

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            encoded,
            sizeof(encoded),
            9u,
            out_bits,
            sizeof(out_bits),
            &out_bit_len),
        SSZ_ERR_LIMIT_EXCEEDED);

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            (const uint8_t[1]){0x00u},
            1u,
            SSZ_NO_LIMIT,
            out_bits,
            sizeof(out_bits),
            &out_bit_len),
        SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_deserialize_vector_fixed_exact_scope(void)
{
    const uint8_t encoded[6] = {0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u};
    uint8_t out[6] = {0u};

    ASSERT_ERR(
        ssz_deserialize_vector_fixed(encoded, sizeof(encoded), 3u, 2u, out, sizeof(out)),
        SSZ_SUCCESS);
    ASSERT_MEM_EQ(out, encoded, sizeof(encoded));

    ASSERT_ERR(
        ssz_deserialize_vector_fixed(encoded, 5u, 3u, 2u, out, sizeof(out)),
        SSZ_ERR_ENCODING_INVALID);

    return true;
}

static bool test_deserialize_vector_variable_offsets_and_dispatch(void)
{
    const uint8_t encoded[18] = {
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
    };

    read_expect_entry_t entries[] = {
        {0u, (const uint8_t[]){0xA0u}, 1u, false},
        {1u, (const uint8_t[]){0xB0u, 0xB1u}, 2u, false},
        {2u, (const uint8_t[]){0xC0u, 0xC1u, 0xC2u}, 3u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_vector_variable(encoded, sizeof(encoded), 3u, 1u, &codec),
        SSZ_SUCCESS);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    const uint8_t non_monotonic[18] = {
        0x0Cu,
        0x00u,
        0x00u,
        0x00u,
        0x0Bu,
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
    };
    ASSERT_ERR(
        ssz_deserialize_vector_variable(non_monotonic, sizeof(non_monotonic), 3u, 1u, &codec),
        SSZ_ERR_OFFSET_INVALID);

    const uint8_t out_of_bounds[18] = {
        0x0Cu,
        0x00u,
        0x00u,
        0x00u,
        0x20u,
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
    };
    ASSERT_ERR(
        ssz_deserialize_vector_variable(out_of_bounds, sizeof(out_of_bounds), 3u, 1u, &codec),
        SSZ_ERR_OFFSET_INVALID);

    return true;
}

static bool test_deserialize_list_fixed_count_and_limits(void)
{
    const uint8_t encoded[4] = {0x10u, 0x11u, 0x20u, 0x21u};
    uint8_t out[4] = {0u};
    uint64_t count = 0u;

    ASSERT_ERR(
        ssz_deserialize_list_fixed(encoded, sizeof(encoded), 2u, 2u, out, sizeof(out), &count),
        SSZ_SUCCESS);
    ASSERT_TRUE(count == 2u);
    ASSERT_MEM_EQ(out, encoded, sizeof(encoded));

    ASSERT_ERR(
        ssz_deserialize_list_fixed(encoded, sizeof(encoded), 1u, 2u, out, sizeof(out), &count),
        SSZ_ERR_LIMIT_EXCEEDED);

    return true;
}

static bool test_deserialize_list_variable_count_limits_and_offsets(void)
{
    const uint8_t encoded[11] = {
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
    };

    read_expect_entry_t entries[] = {
        {0u, (const uint8_t[]){0xE0u, 0xE1u}, 2u, false},
        {1u, (const uint8_t[]){0xF0u}, 1u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    uint64_t out_count = 0u;

    ASSERT_ERR(
        ssz_deserialize_list_variable(encoded, sizeof(encoded), 2u, 1u, &codec, &out_count),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_count == 2u);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    ASSERT_ERR(
        ssz_deserialize_list_variable(encoded, sizeof(encoded), 1u, 1u, &codec, &out_count),
        SSZ_ERR_LIMIT_EXCEEDED);

    const uint8_t non_monotonic[11] = {
        0x08u,
        0x00u,
        0x00u,
        0x00u,
        0x07u,
        0x00u,
        0x00u,
        0x00u,
        0xE0u,
        0xE1u,
        0xF0u,
    };
    ASSERT_ERR(
        ssz_deserialize_list_variable(
            non_monotonic,
            sizeof(non_monotonic),
            SSZ_NO_LIMIT,
            1u,
            &codec,
            &out_count),
        SSZ_ERR_OFFSET_INVALID);

    const uint8_t first_offset_oob[8] = {
        0x0Cu,
        0x00u,
        0x00u,
        0x00u,
        0x0Cu,
        0x00u,
        0x00u,
        0x00u,
    };
    ASSERT_ERR(
        ssz_deserialize_list_variable(
            first_offset_oob,
            sizeof(first_offset_oob),
            SSZ_NO_LIMIT,
            0u,
            &codec,
            &out_count),
        SSZ_ERR_OFFSET_INVALID);

    return true;
}

static bool test_deserialize_container_mixed_fields(void)
{
    const size_t field_fixed_sizes[3] = {1u, 0u, 2u};
    const uint8_t encoded[10] = {
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
    };

    read_expect_entry_t entries[] = {
        {0u, (const uint8_t[]){0xAAu}, 1u, false},
        {1u, (const uint8_t[]){0xB0u, 0xB1u, 0xB2u}, 3u, false},
        {2u, (const uint8_t[]){0xCCu, 0xDDu}, 2u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_container(
            encoded, sizeof(encoded), CONTAINER_SCHEMA(field_fixed_sizes, 3u), &codec),
        SSZ_SUCCESS);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    const uint8_t out_of_bounds_offset[8] = {
        0xAAu,
        0x09u,
        0x00u,
        0x00u,
        0x00u,
        0xCCu,
        0xDDu,
        0x00u,
    };
    ASSERT_ERR(
        ssz_deserialize_container(
            out_of_bounds_offset,
            sizeof(out_of_bounds_offset),
            CONTAINER_SCHEMA(field_fixed_sizes, 3u),
            &codec),
        SSZ_ERR_OFFSET_INVALID);

    const size_t variable_sizes[2] = {0u, 0u};
    const uint8_t non_monotonic_offsets[8] = {
        0x08u,
        0x00u,
        0x00u,
        0x00u,
        0x07u,
        0x00u,
        0x00u,
        0x00u,
    };
    ASSERT_ERR(
        ssz_deserialize_container(
            non_monotonic_offsets,
            sizeof(non_monotonic_offsets),
            CONTAINER_SCHEMA(variable_sizes, 2u),
            &codec),
        SSZ_ERR_OFFSET_INVALID);

    return true;
}

static bool test_deserialize_union_cases(void)
{
    uint8_t selector = 0u;

    ssz_member_codec_t forbidden_codec = {
        .ctx = NULL,
        .write = NULL,
        .read = fail_if_called_read,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x00u}, 1u, 3u, true, &forbidden_codec, &selector),
        SSZ_SUCCESS);
    ASSERT_TRUE(selector == 0u);

    ASSERT_ERR(
        ssz_deserialize_union(
            (const uint8_t[2]){0x00u, 0xAAu},
            2u,
            3u,
            true,
            &forbidden_codec,
            &selector),
        SSZ_ERR_ENCODING_INVALID);

    read_expect_entry_t entries[] = {
        {2u, (const uint8_t[]){0xDEu, 0xADu}, 2u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_union(
            (const uint8_t[3]){0x02u, 0xDEu, 0xADu},
            3u,
            3u,
            true,
            &codec,
            &selector),
        SSZ_SUCCESS);
    ASSERT_TRUE(selector == 2u);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    return true;
}

static bool test_deserialize_union_fixed_width_length_enforcement(void)
{
    static const uint8_t expected_value[32] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u,
        0x08u, 0x09u, 0x0Au, 0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu,
        0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u,
        0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu,
    };
    uint8_t canonical[33] = {0u};
    uint8_t overlong[34] = {0u};
    uint8_t selector = 0u;
    union_uint256_read_ctx_t read_ctx;
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = union_uint256_read,
        .root = NULL,
    };

    memset(&read_ctx, 0, sizeof(read_ctx));
    canonical[0] = 1u;
    memcpy(&canonical[1], expected_value, sizeof(expected_value));

    ASSERT_ERR(
        ssz_deserialize_union(canonical, sizeof(canonical), 2u, true, &codec, &selector),
        SSZ_SUCCESS);
    ASSERT_TRUE(read_ctx.invoked);
    ASSERT_TRUE(selector == 1u);
    ASSERT_MEM_EQ(read_ctx.value, expected_value, sizeof(expected_value));

    memset(&read_ctx, 0, sizeof(read_ctx));
    selector = 0u;
    overlong[0] = 1u;
    memcpy(&overlong[1], expected_value, sizeof(expected_value));
    overlong[33] = 0xA5u;

    ASSERT_ERR(
        ssz_deserialize_union(overlong, sizeof(overlong), 2u, true, &codec, &selector),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_TRUE(read_ctx.invoked);
    ASSERT_TRUE(selector == 0u);
    ASSERT_MEM_EQ(read_ctx.value, ((const uint8_t[32]){0u}), sizeof(read_ctx.value));

    return true;
}

static bool test_deserialize_compatible_union_valid_invalid(void)
{
    const uint8_t allowed[] = {1u, 2u, 7u};
    uint8_t selector = 0u;

    read_expect_entry_t entries[] = {
        {7u, (const uint8_t[]){0x77u, 0x88u}, 2u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[3]){0x07u, 0x77u, 0x88u},
            3u,
            allowed,
            sizeof(allowed),
            &codec,
            &selector),
        SSZ_SUCCESS);
    ASSERT_TRUE(selector == 7u);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[2]){0x00u, 0x00u},
            2u,
            allowed,
            sizeof(allowed),
            &codec,
            &selector),
        SSZ_ERR_SELECTOR_INVALID);

    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[2]){0xC8u, 0x00u},
            2u,
            allowed,
            sizeof(allowed),
            &codec,
            &selector),
        SSZ_ERR_SELECTOR_INVALID);

    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[2]){0x03u, 0x00u},
            2u,
            allowed,
            sizeof(allowed),
            &codec,
            &selector),
        SSZ_ERR_SELECTOR_INVALID);

    return true;
}

static bool test_deserialize_direct_calls(void)
{
    const size_t field_fixed_sizes[3] = {1u, 0u, 2u};
    const uint8_t container_encoded[9] = {
        0x01u,
        0x07u,
        0x00u,
        0x00u,
        0x00u,
        0x20u,
        0x21u,
        0x10u,
        0x11u,
    };
    read_expect_entry_t container_entries[] = {
        {0u, (const uint8_t[]){0x01u}, 1u, false},
        {1u, (const uint8_t[]){0x10u, 0x11u}, 2u, false},
        {2u, (const uint8_t[]){0x20u, 0x21u}, 2u, false},
    };
    read_expect_ctx_t container_ctx = {
        .entries = container_entries,
        .entry_count = sizeof(container_entries) / sizeof(container_entries[0]),
    };
    ssz_member_codec_t container_codec = {
        .ctx = &container_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_container(
            container_encoded,
            sizeof(container_encoded),
            CONTAINER_SCHEMA(field_fixed_sizes, 3u),
            &container_codec),
        SSZ_SUCCESS);
    ASSERT_TRUE(all_read_entries_seen(&container_ctx));

    const uint8_t list_fixed_encoded[4] = {0x30u, 0x31u, 0x32u, 0x33u};
    uint8_t list_fixed_out[4] = {0u};
    uint64_t list_fixed_count = 0u;

    ASSERT_ERR(
        ssz_deserialize_list_fixed(
            list_fixed_encoded,
            sizeof(list_fixed_encoded),
            SSZ_NO_LIMIT,
            2u,
            list_fixed_out,
            sizeof(list_fixed_out),
            &list_fixed_count),
        SSZ_SUCCESS);
    ASSERT_TRUE(list_fixed_count == 2u);
    ASSERT_MEM_EQ(list_fixed_out, list_fixed_encoded, sizeof(list_fixed_encoded));

    const uint8_t list_variable_encoded[11] = {
        0x08u,
        0x00u,
        0x00u,
        0x00u,
        0x09u,
        0x00u,
        0x00u,
        0x00u,
        0xA1u,
        0xB1u,
        0xB2u,
    };
    read_expect_entry_t list_entries[] = {
        {0u, (const uint8_t[]){0xA1u}, 1u, false},
        {1u, (const uint8_t[]){0xB1u, 0xB2u}, 2u, false},
    };
    read_expect_ctx_t list_ctx = {
        .entries = list_entries,
        .entry_count = sizeof(list_entries) / sizeof(list_entries[0]),
    };
    ssz_member_codec_t list_codec = {
        .ctx = &list_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };
    uint64_t list_count = 0u;

    ASSERT_ERR(
        ssz_deserialize_list_variable(
            list_variable_encoded,
            sizeof(list_variable_encoded),
            SSZ_NO_LIMIT,
            1u,
            &list_codec,
            &list_count),
        SSZ_SUCCESS);
    ASSERT_TRUE(list_count == 2u);
    ASSERT_TRUE(all_read_entries_seen(&list_ctx));

    const uint8_t bitlist_encoded[2] = {0x03u, 0x03u};
    uint8_t bitlist_out[2] = {0u};
    uint64_t bit_len = 0u;

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            bitlist_encoded,
            sizeof(bitlist_encoded),
            SSZ_NO_LIMIT,
            bitlist_out,
            sizeof(bitlist_out),
            &bit_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(bit_len == 9u);
    ASSERT_MEM_EQ(bitlist_out, ((const uint8_t[2]){0x03u, 0x01u}), 2u);

    return true;
}

static bool test_deserialize_error_cases(void)
{
    uint8_t out_u8 = 0u;
    uint16_t out_u16 = 0u;
    uint32_t out_u32 = 0u;
    uint64_t out_u64 = 0u;
    uint8_t out128[16] = {0u};
    uint8_t out256[32] = {0u};
    uint8_t out_bool = 0u;
    uint64_t out_count = 0u;

    ASSERT_ERR(ssz_deserialize_uint8(NULL, 1u, &out_u8), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_uint16(NULL, 2u, &out_u16), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_uint32(NULL, 4u, &out_u32), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_uint64(NULL, 8u, &out_u64), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_uint128(NULL, 16u, out128), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_uint256(NULL, 32u, out256), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_boolean(NULL, 1u, &out_bool), SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_deserialize_uint8((const uint8_t[1]){0x00u}, 1u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_uint16((const uint8_t[2]){0x00u, 0x00u}, 2u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_uint32((const uint8_t[4]){0u, 0u, 0u, 0u}, 4u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_uint64((const uint8_t[8]){0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, 8u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_uint128((const uint8_t[16]){0u}, 16u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_uint256((const uint8_t[32]){0u}, 32u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_deserialize_boolean((const uint8_t[1]){0u}, 1u, NULL), SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_deserialize_bitvector((const uint8_t[2]){0x00u, 0x00u}, 2u, 9u, NULL, 0u),
        SSZ_ERR_BUFFER_TOO_SMALL);

    ssz_member_codec_t null_codec = {
        .ctx = NULL,
        .write = NULL,
        .read = NULL,
        .root = NULL,
    };

    ASSERT_ERR(
        ssz_deserialize_list_variable(
            (const uint8_t[1]){0x00u},
            1u,
            SSZ_NO_LIMIT,
            0u,
            &null_codec,
            &out_count),
        SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_deserialize_bitfield_error_paths(void)
{
    uint8_t out_bits[2] = {0u};
    uint64_t out_bit_len = 0u;

    ASSERT_ERR(
        ssz_deserialize_bitvector((const uint8_t[1]){0x00u}, 1u, 0u, out_bits, sizeof(out_bits)),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_bitvector(
            (const uint8_t[1]){0x00u},
            1u,
            UINT64_MAX,
            out_bits,
            sizeof(out_bits)),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_bitvector(NULL, 1u, 8u, out_bits, sizeof(out_bits)),
        SSZ_ERR_ENCODING_INVALID);

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            (const uint8_t[1]){0x01u},
            1u,
            SSZ_NO_LIMIT,
            out_bits,
            sizeof(out_bits),
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_bitlist(NULL, 1u, SSZ_NO_LIMIT, out_bits, sizeof(out_bits), &out_bit_len),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_bitlist(
            (const uint8_t[2]){0xFFu, 0x03u},
            2u,
            SSZ_NO_LIMIT,
            NULL,
            0u,
            &out_bit_len),
        SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(
        ssz_deserialize_bitlist(
            (const uint8_t[2]){0xABu, 0x01u},
            2u,
            SSZ_NO_LIMIT,
            out_bits,
            sizeof(out_bits),
            &out_bit_len),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_bit_len == 8u);
    ASSERT_MEM_EQ(out_bits, ((const uint8_t[1]){0xABu}), 1u);

    return true;
}

static bool test_deserialize_variable_sequence_error_paths(void)
{
    const ssz_error_t ok_errors[1] = {SSZ_SUCCESS};
    scripted_read_ctx_t ok_ctx = {
        .errors = ok_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t ok_codec = make_scripted_read_codec(&ok_ctx);

    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[4]){0x04u, 0x00u, 0x00u, 0x00u},
            4u,
            0u,
            0u,
            &ok_codec),
        SSZ_ERR_SCHEMA_INVALID);

    ssz_member_codec_t null_read_codec = {
        .ctx = NULL,
        .write = NULL,
        .read = NULL,
        .root = NULL,
    };
    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[4]){0x04u, 0x00u, 0x00u, 0x00u},
            4u,
            1u,
            0u,
            &null_read_codec),
        SSZ_ERR_INVALID_ARGUMENT);

#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[4]){0x04u, 0x00u, 0x00u, 0x00u},
            4u,
            ((uint64_t)SIZE_MAX / SSZ_BYTES_PER_LENGTH_OFFSET) + 1u,
            0u,
            &ok_codec),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_deserialize_vector_variable((const uint8_t[8]){0u}, 8u, 3u, 0u, &ok_codec),
        SSZ_ERR_OFFSET_INVALID);

    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[12]){
                0x08u,
                0x00u,
                0x00u,
                0x00u,
                0x08u,
                0x00u,
                0x00u,
                0x00u,
                0x08u,
                0x00u,
                0x00u,
                0x00u,
            },
            12u,
            3u,
            0u,
            &ok_codec),
        SSZ_ERR_OFFSET_INVALID);

    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[8]){
                0x08u,
                0x00u,
                0x00u,
                0x00u,
                0x08u,
                0x00u,
                0x00u,
                0x00u,
            },
            8u,
            2u,
            1u,
            &ok_codec),
        SSZ_ERR_OFFSET_INVALID);

    const ssz_error_t fail_errors[1] = {SSZ_ERR_TYPE_MISMATCH};
    scripted_read_ctx_t fail_ctx = {
        .errors = fail_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t fail_codec = make_scripted_read_codec(&fail_ctx);
    ASSERT_ERR(
        ssz_deserialize_vector_variable(
            (const uint8_t[4]){0x04u, 0x00u, 0x00u, 0x00u},
            4u,
            1u,
            0u,
            &fail_codec),
        SSZ_ERR_TYPE_MISMATCH);

    return true;
}

static bool test_deserialize_collection_error_paths(void)
{
    uint8_t out[4] = {0u};
    uint64_t out_count = 0u;

    ASSERT_ERR(
        ssz_deserialize_vector_fixed((const uint8_t[1]){0x00u}, 1u, 0u, 1u, out, sizeof(out)),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_vector_fixed((const uint8_t[1]){0x00u}, 1u, 1u, 0u, out, sizeof(out)),
        SSZ_ERR_SCHEMA_INVALID);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_deserialize_vector_fixed(
            (const uint8_t[1]){0x00u},
            1u,
            ((uint64_t)SIZE_MAX / 2u) + 1u,
            2u,
            out,
            sizeof(out)),
        SSZ_ERR_OVERFLOW);
#endif
    ASSERT_ERR(
        ssz_deserialize_vector_fixed((const uint8_t[2]){0x11u, 0x22u}, 2u, 2u, 1u, out, 1u),
        SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(
        ssz_deserialize_vector_boolean((const uint8_t[1]){0x00u}, 1u, 0u, out, sizeof(out)),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_vector_boolean((const uint8_t[2]){0x00u, 0x01u}, 2u, 1u, out, sizeof(out)),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_vector_boolean((const uint8_t[2]){0x00u, 0x01u}, 2u, 2u, out, 1u),
        SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(
        ssz_deserialize_list_fixed(
            (const uint8_t[2]){0x00u, 0x01u},
            2u,
            SSZ_NO_LIMIT,
            1u,
            out,
            sizeof(out),
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_list_fixed(
            (const uint8_t[1]){0x00u},
            1u,
            SSZ_NO_LIMIT,
            0u,
            out,
            sizeof(out),
            &out_count),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_list_fixed(
            (const uint8_t[3]){0x00u, 0x01u, 0x02u},
            3u,
            SSZ_NO_LIMIT,
            2u,
            out,
            sizeof(out),
            &out_count),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_list_fixed(NULL, 2u, SSZ_NO_LIMIT, 1u, out, sizeof(out), &out_count),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_list_fixed(
            (const uint8_t[2]){0x00u, 0x01u},
            2u,
            SSZ_NO_LIMIT,
            1u,
            out,
            1u,
            &out_count),
        SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(
        ssz_deserialize_list_boolean((const uint8_t[1]){0x00u}, 1u, SSZ_NO_LIMIT, out, sizeof(out), NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_list_boolean((const uint8_t[2]){0x00u, 0x01u}, 2u, 1u, out, sizeof(out), &out_count),
        SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(
        ssz_deserialize_list_boolean(NULL, 1u, SSZ_NO_LIMIT, out, sizeof(out), &out_count),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_list_boolean((const uint8_t[2]){0x00u, 0x01u}, 2u, SSZ_NO_LIMIT, out, 1u, &out_count),
        SSZ_ERR_BUFFER_TOO_SMALL);

    ASSERT_ERR(
        ssz_deserialize_list_variable(
            (const uint8_t[4]){0x04u, 0x00u, 0x00u, 0x00u},
            4u,
            SSZ_NO_LIMIT,
            0u,
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = NULL,
                                  .read = fail_if_called_read,
                                  .root = NULL},
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    const ssz_error_t ok_errors[1] = {SSZ_SUCCESS};
    scripted_read_ctx_t ok_ctx = {
        .errors = ok_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t ok_codec = make_scripted_read_codec(&ok_ctx);
    ASSERT_ERR(
        ssz_deserialize_list_variable(NULL, 0u, SSZ_NO_LIMIT, 0u, &ok_codec, &out_count),
        SSZ_SUCCESS);
    ASSERT_TRUE(out_count == 0u);

    ASSERT_ERR(
        ssz_deserialize_list_variable(
            (const uint8_t[3]){0x00u, 0x00u, 0x00u},
            3u,
            SSZ_NO_LIMIT,
            0u,
            &ok_codec,
            &out_count),
        SSZ_ERR_OFFSET_INVALID);
    ASSERT_ERR(
        ssz_deserialize_list_variable(
            (const uint8_t[4]){0x00u, 0x00u, 0x00u, 0x00u},
            4u,
            SSZ_NO_LIMIT,
            0u,
            &ok_codec,
            &out_count),
        SSZ_ERR_OFFSET_INVALID);

    return true;
}

static bool test_deserialize_container_additional_error_paths(void)
{
    ASSERT_ERR(
        ssz_deserialize_container((const uint8_t[1]){0x00u}, 1u, NULL, NULL),
        SSZ_ERR_SCHEMA_INVALID);

    const size_t one_fixed[1] = {1u};
    ASSERT_ERR(
        ssz_deserialize_container((const uint8_t[1]){0x00u}, 1u, CONTAINER_SCHEMA(one_fixed, 1u), NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_container(
            NULL,
            1u,
            CONTAINER_SCHEMA(one_fixed, 1u),
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = NULL,
                                  .read = fail_if_called_read,
                                  .root = NULL}),
        SSZ_ERR_INVALID_ARGUMENT);

    const size_t overflow_fixed_region[2] = {SIZE_MAX, 1u};
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[1]){0x00u},
            1u,
            CONTAINER_SCHEMA(overflow_fixed_region, 2u),
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = NULL,
                                  .read = fail_if_called_read,
                                  .root = NULL}),
        SSZ_ERR_OVERFLOW);

    const size_t two_fixed[2] = {4u, 4u};
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[4]){0x00u, 0x00u, 0x00u, 0x00u},
            4u,
            CONTAINER_SCHEMA(two_fixed, 2u),
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = NULL,
                                  .read = fail_if_called_read,
                                  .root = NULL}),
        SSZ_ERR_OFFSET_INVALID);

    const size_t variable_then_fixed[2] = {0u, 1u};
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[6]){
                0x04u,
                0x00u,
                0x00u,
                0x00u,
                0xAAu,
                0xBBu,
            },
            6u,
            CONTAINER_SCHEMA(variable_then_fixed, 2u),
            &(ssz_member_codec_t){.ctx = NULL,
                                  .write = NULL,
                                  .read = fail_if_called_read,
                                  .root = NULL}),
        SSZ_ERR_OFFSET_INVALID);

    const ssz_error_t fixed_read_fail_errors[1] = {SSZ_ERR_TYPE_MISMATCH};
    scripted_read_ctx_t fixed_read_fail_ctx = {
        .errors = fixed_read_fail_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t fixed_read_fail_codec = make_scripted_read_codec(&fixed_read_fail_ctx);
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[1]){0xAAu},
            1u,
            CONTAINER_SCHEMA(one_fixed, 1u),
            &fixed_read_fail_codec),
        SSZ_ERR_TYPE_MISMATCH);

    const ssz_error_t fixed_ok_errors[1] = {SSZ_SUCCESS};
    scripted_read_ctx_t fixed_ok_ctx = {
        .errors = fixed_ok_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t fixed_ok_codec = make_scripted_read_codec(&fixed_ok_ctx);
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[2]){0xAAu, 0xBBu},
            2u,
            CONTAINER_SCHEMA(one_fixed, 1u),
            &fixed_ok_codec),
        SSZ_ERR_OFFSET_INVALID);

    const size_t mixed_with_two_variables[3] = {0u, 1u, 0u};
    read_expect_entry_t entries[] = {
        {0u, (const uint8_t[]){0xB0u}, 1u, false},
        {1u, (const uint8_t[]){0xAAu}, 1u, false},
        {2u, (const uint8_t[]){0xC0u}, 1u, false},
    };
    read_expect_ctx_t read_ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    ssz_member_codec_t expect_codec = {
        .ctx = &read_ctx,
        .write = NULL,
        .read = read_expect_callback,
        .root = NULL,
    };
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[11]){
                0x09u,
                0x00u,
                0x00u,
                0x00u,
                0xAAu,
                0x0Au,
                0x00u,
                0x00u,
                0x00u,
            0xB0u,
            0xC0u,
        },
        11u,
        CONTAINER_SCHEMA(mixed_with_two_variables, 3u),
        &expect_codec),
        SSZ_SUCCESS);
    ASSERT_TRUE(all_read_entries_seen(&read_ctx));

    const ssz_error_t variable_read_fail_errors[2] = {SSZ_SUCCESS, SSZ_ERR_TYPE_MISMATCH};
    scripted_read_ctx_t variable_read_fail_ctx = {
        .errors = variable_read_fail_errors,
        .step_count = 2u,
        .step_index = 0u,
    };
    ssz_member_codec_t variable_read_fail_codec = make_scripted_read_codec(&variable_read_fail_ctx);
    ASSERT_ERR(
        ssz_deserialize_container(
            (const uint8_t[11]){
                0x09u,
                0x00u,
                0x00u,
                0x00u,
                0xAAu,
                0x0Au,
                0x00u,
                0x00u,
                0x00u,
            0xB0u,
            0xC0u,
        },
        11u,
        CONTAINER_SCHEMA(mixed_with_two_variables, 3u),
        &variable_read_fail_codec),
        SSZ_ERR_TYPE_MISMATCH);

    /* Test: callback mutates field_fixed_sizes during read, causing cursor != fixed_region.
       This covers the guard at ssz_deserialize.c:622. */
    {
        size_t mutating_sizes[2] = {2u, 2u};
        schema_mutating_read_ctx_t mut_ctx = {.sizes = mutating_sizes};
        ssz_member_codec_t mut_codec = {
            .ctx = &mut_ctx,
            .write = NULL,
            .read = schema_mutating_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                (const uint8_t[4]){0xAAu, 0xBBu, 0xCCu, 0xDDu},
                4u,
                CONTAINER_SCHEMA(mutating_sizes, 2u),
                &mut_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    {
        size_t wrap_sizes[2] = {1u, 1u};
        schema_wrap_read_ctx_t wrap_ctx = {
            .sizes = wrap_sizes,
            .second_seen = false,
        };
        ssz_member_codec_t wrap_codec = {
            .ctx = &wrap_ctx,
            .write = NULL,
            .read = schema_wrap_fixed_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                (const uint8_t[2]){0xAAu, 0xBBu},
                2u,
                CONTAINER_SCHEMA(wrap_sizes, 2u),
                &wrap_codec),
            SSZ_ERR_OFFSET_INVALID);
        ASSERT_TRUE(!wrap_ctx.second_seen);
    }

    /* Test: second-pass guard (cursor + fixed_size) > fixed_region.
       Container: [variable, fixed(2), variable]. Callback inflates field 1 to 9999 during
       the first variable-field read, so the second pass overflows on the fixed field. */
    {
        /* Layout: offset(4) | fixed(2) | offset(4) | var_data_0(1) | var_data_2(1) */
        const uint8_t payload[] = {
            0x0Au, 0x00u, 0x00u, 0x00u, /* offset to var field 0: 10 */
            0xFFu, 0xFFu,               /* fixed field 1: 2 bytes */
            0x0Bu, 0x00u, 0x00u, 0x00u, /* offset to var field 2: 11 */
            0xAAu,                      /* var field 0 data */
            0xBBu,                      /* var field 2 data */
        };
        size_t inflate_sizes[3] = {0u, 2u, 0u};
        schema_mutating_read_ctx_t inflate_ctx = {.sizes = inflate_sizes};
        ssz_member_codec_t inflate_codec = {
            .ctx = &inflate_ctx,
            .write = NULL,
            .read = schema_inflate_fixed_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                payload,
                sizeof(payload),
                CONTAINER_SCHEMA(inflate_sizes, 3u),
                &inflate_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    /* Test: second-pass guard (cursor + OFFSET) > fixed_region for a variable slot.
       Container: [variable, fixed(2)]. Callback zeroes field 1 (making it variable) during
       the first read, so the second pass sees two variable fields but cursor is past the
       fixed region when it hits the now-variable slot. */
    {
        /* Layout: offset(4) | fixed(2) | var_data(1) */
        const uint8_t payload[] = {
            0x06u, 0x00u, 0x00u, 0x00u, /* offset to var field 0: 6 */
            0xFFu, 0xFFu,               /* fixed field 1: 2 bytes */
            0xAAu,                      /* var field 0 data */
        };
        size_t zero_sizes[2] = {0u, 2u};
        schema_mutating_read_ctx_t zero_ctx = {.sizes = zero_sizes};
        ssz_member_codec_t zero_codec = {
            .ctx = &zero_ctx,
            .write = NULL,
            .read = schema_zero_fixed_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                payload,
                sizeof(payload),
                CONTAINER_SCHEMA(zero_sizes, 2u),
                &zero_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    {
        const uint8_t payload[] = {
            0x0Cu, 0x00u, 0x00u, 0x00u,
            0xFFu, 0xFFu,
            0xEEu, 0xEEu,
            0x0Du, 0x00u, 0x00u, 0x00u,
            0xAAu,
            0xBBu,
        };
        size_t wrap_look_sizes[4] = {0u, 2u, 2u, 0u};
        schema_mutating_read_ctx_t wrap_look_ctx = {.sizes = wrap_look_sizes};
        ssz_member_codec_t wrap_look_codec = {
            .ctx = &wrap_look_ctx,
            .write = NULL,
            .read = schema_wrap_look_cursor_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                payload,
                sizeof(payload),
                CONTAINER_SCHEMA(wrap_look_sizes, 4u),
                &wrap_look_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    {
        const uint8_t payload[] = {
            0x0Cu, 0x00u, 0x00u, 0x00u,
            0xFFu, 0xFFu,
            0xEEu, 0xEEu,
            0x0Du, 0x00u, 0x00u, 0x00u,
            0xAAu,
            0xBBu,
        };
        size_t look_sizes[4] = {0u, 2u, 2u, 0u};
        schema_mutating_read_ctx_t look_ctx = {.sizes = look_sizes};
        ssz_member_codec_t look_codec = {
            .ctx = &look_ctx,
            .write = NULL,
            .read = schema_inflate_look_cursor_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                payload,
                sizeof(payload),
                CONTAINER_SCHEMA(look_sizes, 4u),
                &look_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    {
        const uint8_t payload[] = {
            0x0Cu, 0x00u, 0x00u, 0x00u,
            0xFFu, 0xFFu,
            0x0Du, 0x00u, 0x00u, 0x00u,
            0x0Cu, 0x00u, 0x00u, 0x00u,
            0xAAu,
            0xBBu,
        };
        size_t end_sizes[3] = {0u, 2u, 0u};
        schema_mutating_read_ctx_t end_ctx = {.sizes = end_sizes};
        ssz_member_codec_t end_codec = {
            .ctx = &end_ctx,
            .write = NULL,
            .read = schema_force_end_lt_start_read,
            .root = NULL,
        };
        ASSERT_ERR(
            ssz_deserialize_container(
                payload,
                sizeof(payload),
                CONTAINER_SCHEMA(end_sizes, 3u),
                &end_codec),
            SSZ_ERR_OFFSET_INVALID);
    }

    return true;
}

static bool test_deserialize_union_additional_error_paths(void)
{
    uint8_t selector = 0u;

    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x00u}, 1u, 2u, false, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x00u}, 1u, 0u, false, NULL, &selector),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x00u}, 1u, 257u, false, NULL, &selector),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x00u}, 1u, 1u, true, NULL, &selector),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_union(NULL, 0u, 2u, false, NULL, &selector),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[1]){0x02u}, 1u, 2u, false, NULL, &selector),
        SSZ_ERR_SELECTOR_INVALID);
    ASSERT_ERR(
        ssz_deserialize_union((const uint8_t[2]){0x01u, 0xAAu}, 2u, 2u, false, NULL, &selector),
        SSZ_ERR_INVALID_ARGUMENT);

    const ssz_error_t read_fail_errors[1] = {SSZ_ERR_TYPE_MISMATCH};
    scripted_read_ctx_t read_fail_ctx = {
        .errors = read_fail_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t read_fail_codec = make_scripted_read_codec(&read_fail_ctx);
    ASSERT_ERR(
        ssz_deserialize_union(
            (const uint8_t[2]){0x01u, 0xAAu},
            2u,
            2u,
            false,
            &read_fail_codec,
            &selector),
        SSZ_ERR_TYPE_MISMATCH);

    const uint8_t allowed_valid[] = {1u, 2u};
    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[1]){0x01u},
            1u,
            allowed_valid,
            sizeof(allowed_valid),
            NULL,
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_deserialize_compatible_union((const uint8_t[1]){0x01u}, 1u, NULL, 1u, NULL, &selector),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[1]){0x01u},
            1u,
            (const uint8_t[]){0u, 2u},
            2u,
            NULL,
            &selector),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            NULL,
            0u,
            allowed_valid,
            sizeof(allowed_valid),
            NULL,
            &selector),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[2]){0x01u, 0xAAu},
            2u,
            allowed_valid,
            sizeof(allowed_valid),
            NULL,
            &selector),
        SSZ_ERR_INVALID_ARGUMENT);

    scripted_read_ctx_t compat_read_fail_ctx = {
        .errors = read_fail_errors,
        .step_count = 1u,
        .step_index = 0u,
    };
    ssz_member_codec_t compat_read_fail_codec = make_scripted_read_codec(&compat_read_fail_ctx);
    ASSERT_ERR(
        ssz_deserialize_compatible_union(
            (const uint8_t[2]){0x01u, 0xAAu},
            2u,
            allowed_valid,
            sizeof(allowed_valid),
            &compat_read_fail_codec,
            &selector),
        SSZ_ERR_TYPE_MISMATCH);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"deserialize_basic_round_trips", test_deserialize_basic_round_trips},
        {"deserialize_boolean_canonical_enforcement",
         test_deserialize_boolean_canonical_enforcement},
        {"deserialize_boolean_collection_canonical_enforcement",
         test_deserialize_boolean_collection_canonical_enforcement},
        {"deserialize_fixed_width_short_inputs", test_deserialize_fixed_width_short_inputs},
        {"deserialize_fixed_width_overlong_inputs", test_deserialize_fixed_width_overlong_inputs},
        {"deserialize_bitvector_padding_validation", test_deserialize_bitvector_padding_validation},
        {"deserialize_bitlist_delimiter_and_errors", test_deserialize_bitlist_delimiter_and_errors},
        {"deserialize_vector_fixed_exact_scope", test_deserialize_vector_fixed_exact_scope},
        {"deserialize_vector_variable_offsets_and_dispatch",
         test_deserialize_vector_variable_offsets_and_dispatch},
        {"deserialize_list_fixed_count_and_limits", test_deserialize_list_fixed_count_and_limits},
        {"deserialize_list_variable_count_limits_and_offsets",
         test_deserialize_list_variable_count_limits_and_offsets},
        {"deserialize_container_mixed_fields", test_deserialize_container_mixed_fields},
        {"deserialize_union_cases", test_deserialize_union_cases},
        {"deserialize_union_fixed_width_length_enforcement",
         test_deserialize_union_fixed_width_length_enforcement},
        {"deserialize_compatible_union_valid_invalid",
         test_deserialize_compatible_union_valid_invalid},
        {"deserialize_direct_calls", test_deserialize_direct_calls},
        {"deserialize_error_cases", test_deserialize_error_cases},
        {"deserialize_bitfield_error_paths", test_deserialize_bitfield_error_paths},
        {"deserialize_variable_sequence_error_paths",
         test_deserialize_variable_sequence_error_paths},
        {"deserialize_collection_error_paths", test_deserialize_collection_error_paths},
        {"deserialize_container_additional_error_paths",
         test_deserialize_container_additional_error_paths},
        {"deserialize_union_additional_error_paths", test_deserialize_union_additional_error_paths},
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

    printf("[OK] %zu/%zu deserialize tests passed\n", passed, total);
    return 0;
}
