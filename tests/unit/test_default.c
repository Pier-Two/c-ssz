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

#define ASSERT_TRUE(cond)                                                                            \
    do                                                                                               \
    {                                                                                                \
        if (!(cond))                                                                                 \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
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

#define ASSERT_MEM_EQ(actual, expected, len)                                                         \
    do                                                                                               \
    {                                                                                                \
        if (memcmp((actual), (expected), (len)) != 0)                                               \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: memory mismatch (%s vs %s, len=%zu)\n",            \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    #actual,                                                                          \
                    #expected,                                                                        \
                    (size_t)(len));                                                                   \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

typedef struct
{
    uint64_t id;
    uint8_t current[64];
    size_t current_len;
    uint8_t default_bytes[64];
    size_t default_len;
    size_t default_calls;
    size_t restore_calls;
    size_t write_calls;
    ssz_error_t default_err;
    ssz_error_t restore_err;
    ssz_error_t write_err;
} stateful_entry_t;

typedef struct
{
    stateful_entry_t *entries;
    size_t entry_count;
} stateful_codec_ctx_t;

static stateful_entry_t *find_stateful_entry(stateful_codec_ctx_t *ctx, uint64_t member_id)
{
    if (ctx == NULL)
    {
        return NULL;
    }

    for (size_t i = 0u; i < ctx->entry_count; i++)
    {
        if (ctx->entries[i].id == member_id)
        {
            return &ctx->entries[i];
        }
    }

    return NULL;
}

static const stateful_entry_t *find_stateful_entry_const(
    const stateful_codec_ctx_t *ctx,
    uint64_t member_id)
{
    return find_stateful_entry((stateful_codec_ctx_t *)ctx, member_id);
}

static ssz_error_t stateful_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const stateful_codec_ctx_t *codec_ctx = (const stateful_codec_ctx_t *)ctx;
    const stateful_entry_t *entry = find_stateful_entry_const(codec_ctx, member_id);

    if ((entry == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ((stateful_entry_t *)entry)->write_calls++;
    if (entry->write_err != SSZ_SUCCESS)
    {
        return entry->write_err;
    }

    *out_written = entry->current_len;
    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < entry->current_len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(out, entry->current, entry->current_len);
    return SSZ_SUCCESS;
}

static ssz_error_t stateful_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    stateful_codec_ctx_t *codec_ctx = (stateful_codec_ctx_t *)ctx;
    stateful_entry_t *entry = find_stateful_entry(codec_ctx, member_id);

    if (entry == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_len == 0u))
    {
        entry->default_calls++;
        if (entry->default_err != SSZ_SUCCESS)
        {
            return entry->default_err;
        }

        entry->current_len = entry->default_len;
        memcpy(entry->current, entry->default_bytes, entry->default_len);
        return SSZ_SUCCESS;
    }

    if (data == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    entry->restore_calls++;
    if (entry->restore_err != SSZ_SUCCESS)
    {
        return entry->restore_err;
    }
    if (data_len > sizeof(entry->current))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    entry->current_len = data_len;
    memcpy(entry->current, data, data_len);
    return SSZ_SUCCESS;
}

static ssz_member_codec_t make_stateful_codec(stateful_codec_ctx_t *ctx)
{
    ssz_member_codec_t codec = {
        .ctx = ctx,
        .write = stateful_write,
        .read = stateful_read,
        .root = NULL,
    };
    return codec;
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

static bool test_default_scalar_and_alias_helpers(void)
{
    uint8_t out_u8 = 0xFFu;
    uint16_t out_u16 = UINT16_C(0xFFFF);
    uint32_t out_u32 = UINT32_C(0xFFFFFFFF);
    uint64_t out_u64 = UINT64_C(0xFFFFFFFFFFFFFFFF);
    uint8_t out_u128[16];
    uint8_t out_u256[32];
    uint8_t out_bool = 1u;
    uint8_t out_byte = 0xABu;
    uint8_t out_bit = 1u;

    memset(out_u128, 0xAA, sizeof(out_u128));
    memset(out_u256, 0xBB, sizeof(out_u256));

    ASSERT_ERR(ssz_default_uint8(&out_u8), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint16(&out_u16), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint32(&out_u32), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint64(&out_u64), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint128(out_u128), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint256(out_u256), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_boolean(&out_bool), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_uint8(&out_byte), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_boolean(&out_bit), SSZ_SUCCESS);

    ASSERT_TRUE(out_u8 == 0u);
    ASSERT_TRUE(out_u16 == 0u);
    ASSERT_TRUE(out_u32 == 0u);
    ASSERT_TRUE(out_u64 == 0u);
    ASSERT_MEM_EQ(out_u128, ((const uint8_t[16]){0}), sizeof(out_u128));
    ASSERT_MEM_EQ(out_u256, ((const uint8_t[32]){0}), sizeof(out_u256));
    ASSERT_TRUE(out_bool == 0u);
    ASSERT_TRUE(out_byte == 0u);
    ASSERT_TRUE(out_bit == 0u);

    ASSERT_ERR(ssz_default_uint8(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_uint128(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_boolean(NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_default_sequence_helpers(void)
{
    uint8_t bitvector[4] = {0xAAu, 0xAAu, 0xAAu, 0xAAu};
    uint64_t bit_len = 3u;
    uint8_t vector_fixed[7] = {0xCCu, 0xCCu, 0xCCu, 0xCCu, 0xCCu, 0xCCu, 0xCCu};
    uint64_t list_count = 9u;
    uint64_t second_list_count = 7u;
    uint64_t second_bit_len = 5u;

    ASSERT_ERR(ssz_default_bitvector(bitvector, sizeof(bitvector), 10u), SSZ_SUCCESS);
    ASSERT_MEM_EQ(bitvector, ((const uint8_t[4]){0x00u, 0x00u, 0xAAu, 0xAAu}), sizeof(bitvector));

    ASSERT_ERR(ssz_default_bitlist(&bit_len), SSZ_SUCCESS);
    ASSERT_TRUE(bit_len == 0u);

    ASSERT_ERR(ssz_default_vector_fixed(vector_fixed, 3u, 2u), SSZ_SUCCESS);
    ASSERT_MEM_EQ(vector_fixed,
                  ((const uint8_t[7]){0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xCCu}),
                  sizeof(vector_fixed));

    ASSERT_ERR(ssz_default_list(&list_count), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_list(&second_list_count), SSZ_SUCCESS);
    ASSERT_ERR(ssz_default_bitlist(&second_bit_len), SSZ_SUCCESS);
    ASSERT_TRUE(list_count == 0u);
    ASSERT_TRUE(second_list_count == 0u);
    ASSERT_TRUE(second_bit_len == 0u);

    ASSERT_ERR(ssz_default_bitvector(bitvector, sizeof(bitvector), 0u), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_bitvector(NULL, 0u, 8u), SSZ_ERR_BUFFER_TOO_SMALL);
    ASSERT_ERR(ssz_default_bitlist(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_vector_fixed(NULL, 1u, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_vector_fixed(vector_fixed, 0u, 1u), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_vector_fixed(vector_fixed, 1u, 0u), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_list(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_list(NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_bitlist(NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_default_composite_helpers(void)
{
    stateful_entry_t vector_entries[] = {
        {
            .id = 0u,
            .current = {0x10u},
            .current_len = 1u,
            .default_bytes = {0x01u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0x20u, 0x21u},
            .current_len = 2u,
            .default_bytes = {0x02u, 0x03u},
            .default_len = 2u,
        },
    };
    stateful_codec_ctx_t vector_ctx = {
        .entries = vector_entries,
        .entry_count = sizeof(vector_entries) / sizeof(vector_entries[0]),
    };
    ssz_member_codec_t vector_codec = make_stateful_codec(&vector_ctx);

    stateful_entry_t container_entries[] = {
        {
            .id = 0u,
            .current = {0xA0u},
            .current_len = 1u,
            .default_bytes = {0x11u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0xB0u, 0xB1u},
            .current_len = 2u,
            .default_bytes = {0x22u, 0x23u},
            .default_len = 2u,
        },
        {
            .id = 2u,
            .current = {0xC0u, 0xC1u, 0xC2u},
            .current_len = 3u,
            .default_bytes = {0x33u, 0x34u, 0x35u},
            .default_len = 3u,
        },
    };
    stateful_codec_ctx_t container_ctx = {
        .entries = container_entries,
        .entry_count = sizeof(container_entries) / sizeof(container_entries[0]),
    };
    ssz_member_codec_t container_codec = make_stateful_codec(&container_ctx);
    const size_t field_fixed_sizes[] = {1u, 0u, 3u};

    stateful_entry_t sparse_entries[] = {
        {
            .id = 0u,
            .current = {0xD0u},
            .current_len = 1u,
            .default_bytes = {0x41u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0xE0u},
            .current_len = 1u,
            .default_bytes = {0x42u},
            .default_len = 1u,
        },
    };
    stateful_codec_ctx_t sparse_ctx = {
        .entries = sparse_entries,
        .entry_count = sizeof(sparse_entries) / sizeof(sparse_entries[0]),
    };
    ssz_member_codec_t sparse_codec = make_stateful_codec(&sparse_ctx);

    ASSERT_ERR(ssz_default_vector_composite(2u, &vector_codec), SSZ_SUCCESS);
    ASSERT_TRUE(vector_entries[0].default_calls == 1u);
    ASSERT_TRUE(vector_entries[1].default_calls == 1u);
    ASSERT_MEM_EQ(vector_entries[0].current, vector_entries[0].default_bytes, vector_entries[0].default_len);
    ASSERT_MEM_EQ(vector_entries[1].current, vector_entries[1].default_bytes, vector_entries[1].default_len);

    ASSERT_ERR(ssz_default_container(CONTAINER_SCHEMA(field_fixed_sizes, 3u), &container_codec),
               SSZ_SUCCESS);
    for (size_t i = 0u; i < container_ctx.entry_count; i++)
    {
        ASSERT_TRUE(container_entries[i].default_calls == 1u);
        ASSERT_MEM_EQ(
            container_entries[i].current,
            container_entries[i].default_bytes,
            container_entries[i].default_len);
    }

    ASSERT_ERR(ssz_default_container(CONTAINER_SCHEMA(((const size_t[]){1u, 0u}), 2u), &sparse_codec),
               SSZ_SUCCESS);
    ASSERT_TRUE(sparse_entries[0].default_calls == 1u);
    ASSERT_TRUE(sparse_entries[1].default_calls == 1u);
    ASSERT_MEM_EQ(
        sparse_entries[0].current,
        sparse_entries[0].default_bytes,
        sparse_entries[0].default_len);
    ASSERT_MEM_EQ(
        sparse_entries[1].current,
        sparse_entries[1].default_bytes,
        sparse_entries[1].default_len);

    ASSERT_ERR(ssz_default_vector_composite(0u, &vector_codec), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_default_vector_composite(
            1u,
            &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL}),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_container(NULL, &container_codec), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_container(CONTAINER_SCHEMA(field_fixed_sizes, 0u), &container_codec),
               SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_container(
                   CONTAINER_SCHEMA(((const size_t[]){1u}), 1u),
                   &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL}),
               SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_default_union_helper(void)
{
    stateful_entry_t union_entries[] = {
        {
            .id = 0u,
            .current = {0x90u, 0x91u},
            .current_len = 2u,
            .default_bytes = {0x01u, 0x02u},
            .default_len = 2u,
        },
        {
            .id = 1u,
            .current = {0xA0u},
            .current_len = 1u,
            .default_bytes = {0x03u},
            .default_len = 1u,
        },
    };
    stateful_codec_ctx_t union_ctx = {
        .entries = union_entries,
        .entry_count = sizeof(union_entries) / sizeof(union_entries[0]),
    };
    ssz_member_codec_t union_codec = make_stateful_codec(&union_ctx);
    ssz_member_codec_t fail_codec = {
        .ctx = NULL,
        .write = NULL,
        .read = fail_if_called_read,
        .root = NULL,
    };
    uint8_t selector = 99u;

    ASSERT_ERR(ssz_default_union(2u, false, &union_codec, &selector), SSZ_SUCCESS);
    ASSERT_TRUE(selector == 0u);
    ASSERT_TRUE(union_entries[0].default_calls == 1u);
    ASSERT_TRUE(union_entries[1].default_calls == 0u);
    ASSERT_MEM_EQ(union_entries[0].current, union_entries[0].default_bytes, union_entries[0].default_len);

    selector = 77u;
    ASSERT_ERR(ssz_default_union(2u, true, &fail_codec, &selector), SSZ_SUCCESS);
    ASSERT_TRUE(selector == 0u);

    ASSERT_ERR(ssz_default_union(0u, false, &union_codec, &selector), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_union(257u, false, &union_codec, &selector), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_union(1u, true, &union_codec, &selector), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_default_union(2u, false, NULL, &selector), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_default_union(2u, false, &union_codec, NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_is_zero_scalar_and_alias_helpers(void)
{
    bool is_zero = false;
    uint8_t nonzero_u128[16] = {0u};
    uint8_t nonzero_u256[32] = {0u};

    nonzero_u128[7] = 0x01u;
    nonzero_u256[31] = 0x01u;

    ASSERT_ERR(ssz_is_zero_uint8(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint8(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint16(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint16(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint32(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint32(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint64(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint64(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint128((const uint8_t[16]){0}, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint128(nonzero_u128, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint256((const uint8_t[32]){0}, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_uint256(nonzero_u256, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_boolean(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_boolean(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint8(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_boolean(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_uint8(0u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_uint128(NULL, &is_zero), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_boolean(0u, NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_is_zero_sequence_helpers(void)
{
    bool is_zero = false;
    const uint8_t zero_bits[2] = {0x00u, 0x00u};
    const uint8_t nonzero_bits[2] = {0x00u, 0x01u};
    const uint8_t invalid_bits[2] = {0x00u, 0x80u};
    const uint8_t zero_fixed[6] = {0u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t nonzero_fixed[6] = {0u, 0u, 0u, 0x01u, 0u, 0u};

    ASSERT_ERR(ssz_is_zero_bitvector(zero_bits, sizeof(zero_bits), 10u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_bitvector(nonzero_bits, sizeof(nonzero_bits), 10u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);
    ASSERT_ERR(ssz_is_zero_bitvector(invalid_bits, sizeof(invalid_bits), 10u, &is_zero),
               SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(ssz_is_zero_bitvector(NULL, 0u, 8u, &is_zero), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_bitvector(zero_bits, sizeof(zero_bits), 0u, &is_zero), SSZ_ERR_SCHEMA_INVALID);

    ASSERT_ERR(ssz_is_zero_bitlist(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_bitlist(1u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_vector_fixed(zero_fixed, 3u, 2u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_vector_fixed(nonzero_fixed, 3u, 2u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);
    ASSERT_ERR(ssz_is_zero_vector_fixed(NULL, 1u, 1u, &is_zero), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_vector_fixed(zero_fixed, 0u, 1u, &is_zero), SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_is_zero_vector_fixed(zero_fixed, 1u, 0u, &is_zero), SSZ_ERR_SCHEMA_INVALID);

    ASSERT_ERR(ssz_is_zero_list(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_list(2u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_list(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_list(3u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_bitlist(0u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_bitlist(4u, &is_zero), SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(ssz_is_zero_list(0u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_bitlist(0u, NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_is_zero_composite_helpers(void)
{
    bool is_zero = false;
    uint8_t scratch[4u] = {0u};

    stateful_entry_t vector_true_entries[] = {
        {
            .id = 0u,
            .current = {0x01u},
            .current_len = 1u,
            .default_bytes = {0x01u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0x02u, 0x03u},
            .current_len = 2u,
            .default_bytes = {0x02u, 0x03u},
            .default_len = 2u,
        },
    };
    stateful_codec_ctx_t vector_true_ctx = {
        .entries = vector_true_entries,
        .entry_count = sizeof(vector_true_entries) / sizeof(vector_true_entries[0]),
    };
    ssz_member_codec_t vector_true_codec = make_stateful_codec(&vector_true_ctx);

    ASSERT_ERR(ssz_is_zero_vector_composite(2u, &vector_true_codec, scratch, sizeof(scratch), &is_zero),
               SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    for (size_t i = 0u; i < vector_true_ctx.entry_count; i++)
    {
        ASSERT_TRUE(vector_true_entries[i].default_calls == 1u);
        ASSERT_TRUE(vector_true_entries[i].restore_calls == 1u);
        ASSERT_TRUE(vector_true_entries[i].write_calls == 4u);
        ASSERT_MEM_EQ(
            vector_true_entries[i].current,
            vector_true_entries[i].default_bytes,
            vector_true_entries[i].default_len);
    }

    stateful_entry_t vector_false_entries[] = {
        {
            .id = 0u,
            .current = {0x10u},
            .current_len = 1u,
            .default_bytes = {0x01u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0x02u},
            .current_len = 1u,
            .default_bytes = {0x02u},
            .default_len = 1u,
        },
    };
    stateful_codec_ctx_t vector_false_ctx = {
        .entries = vector_false_entries,
        .entry_count = sizeof(vector_false_entries) / sizeof(vector_false_entries[0]),
    };
    ssz_member_codec_t vector_false_codec = make_stateful_codec(&vector_false_ctx);

    ASSERT_ERR(
        ssz_is_zero_vector_composite(2u, &vector_false_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);
    ASSERT_TRUE(vector_false_entries[0].default_calls == 1u);
    ASSERT_TRUE(vector_false_entries[0].restore_calls == 1u);
    ASSERT_TRUE(vector_false_entries[0].write_calls == 4u);
    ASSERT_TRUE(vector_false_entries[1].default_calls == 0u);
    ASSERT_TRUE(vector_false_entries[1].restore_calls == 0u);
    ASSERT_TRUE(vector_false_entries[1].write_calls == 0u);
    ASSERT_MEM_EQ(vector_false_entries[0].current, ((const uint8_t[1]){0x10u}), 1u);

    stateful_entry_t container_entries[] = {
        {
            .id = 0u,
            .current = {0x11u},
            .current_len = 1u,
            .default_bytes = {0x11u},
            .default_len = 1u,
        },
        {
            .id = 1u,
            .current = {0x22u, 0x23u},
            .current_len = 2u,
            .default_bytes = {0x22u, 0x23u},
            .default_len = 2u,
        },
    };
    stateful_codec_ctx_t container_ctx = {
        .entries = container_entries,
        .entry_count = sizeof(container_entries) / sizeof(container_entries[0]),
    };
    ssz_member_codec_t container_codec = make_stateful_codec(&container_ctx);

    ASSERT_ERR(
        ssz_is_zero_container(
            CONTAINER_SCHEMA(((const size_t[]){1u, 0u}), 2u),
            &container_codec,
            scratch,
            sizeof(scratch),
            &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_ERR(ssz_is_zero_container(
                   CONTAINER_SCHEMA(((const size_t[]){1u, 0u}), 2u),
                   &container_codec,
                   scratch,
                   sizeof(scratch),
                   &is_zero),
               SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);

    ASSERT_ERR(
        ssz_is_zero_vector_composite(
            1u,
            &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = stateful_read, .root = NULL},
            scratch,
            sizeof(scratch),
            &is_zero),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_is_zero_container(NULL, &container_codec, scratch, sizeof(scratch), &is_zero),
               SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_is_zero_container(
                   CONTAINER_SCHEMA(((const size_t[]){1u}), 1u),
                   &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL},
                   scratch,
                   sizeof(scratch),
                   &is_zero),
               SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_is_zero_union_helper(void)
{
    bool is_zero = false;
    uint8_t scratch[4u] = {0u};
    stateful_entry_t default_entry[] = {
        {
            .id = 0u,
            .current = {0x01u, 0x02u},
            .current_len = 2u,
            .default_bytes = {0x01u, 0x02u},
            .default_len = 2u,
        },
    };
    stateful_codec_ctx_t default_ctx = {
        .entries = default_entry,
        .entry_count = sizeof(default_entry) / sizeof(default_entry[0]),
    };
    ssz_member_codec_t default_codec = make_stateful_codec(&default_ctx);

    ASSERT_ERR(
        ssz_is_zero_union(0u, 2u, false, &default_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);
    ASSERT_TRUE(default_entry[0].default_calls == 1u);
    ASSERT_TRUE(default_entry[0].restore_calls == 1u);
    ASSERT_TRUE(default_entry[0].write_calls == 4u);

    stateful_entry_t nondefault_entry[] = {
        {
            .id = 0u,
            .current = {0x09u},
            .current_len = 1u,
            .default_bytes = {0x01u},
            .default_len = 1u,
        },
    };
    stateful_codec_ctx_t nondefault_ctx = {
        .entries = nondefault_entry,
        .entry_count = sizeof(nondefault_entry) / sizeof(nondefault_entry[0]),
    };
    ssz_member_codec_t nondefault_codec = make_stateful_codec(&nondefault_ctx);

    ASSERT_ERR(
        ssz_is_zero_union(0u, 2u, false, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);
    ASSERT_MEM_EQ(nondefault_entry[0].current, ((const uint8_t[1]){0x09u}), 1u);

    ASSERT_ERR(
        ssz_is_zero_union(1u, 2u, false, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(!is_zero);

    ASSERT_ERR(
        ssz_is_zero_union(
            0u,
            2u,
            true,
            &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = fail_if_called_read, .root = NULL},
            scratch,
            sizeof(scratch),
            &is_zero),
        SSZ_SUCCESS);
    ASSERT_TRUE(is_zero);

    ASSERT_ERR(ssz_is_zero_union(2u, 2u, false, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
               SSZ_ERR_SELECTOR_INVALID);
    ASSERT_ERR(ssz_is_zero_union(0u, 0u, false, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
               SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_is_zero_union(0u, 257u, false, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_is_zero_union(0u, 1u, true, &nondefault_codec, scratch, sizeof(scratch), &is_zero),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_is_zero_union(0u, 2u, false, NULL, scratch, sizeof(scratch), &is_zero),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_is_zero_union(0u, 2u, false, &nondefault_codec, scratch, sizeof(scratch), NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"default_scalar_and_alias_helpers", test_default_scalar_and_alias_helpers},
        {"default_sequence_helpers", test_default_sequence_helpers},
        {"default_composite_helpers", test_default_composite_helpers},
        {"default_union_helper", test_default_union_helper},
        {"is_zero_scalar_and_alias_helpers", test_is_zero_scalar_and_alias_helpers},
        {"is_zero_sequence_helpers", test_is_zero_sequence_helpers},
        {"is_zero_composite_helpers", test_is_zero_composite_helpers},
        {"is_zero_union_helper", test_is_zero_union_helper},
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

    printf("[OK] %zu/%zu default tests passed\n", passed, total);
    return 0;
}
