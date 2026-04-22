#include <stddef.h>
#include <stdint.h>

#include "ssz.h"

#define CONTAINER_SCHEMA(field_fixed_sizes_value, field_count_value)                              \
    (&(const ssz_container_schema_t){                                                             \
        .field_fixed_sizes = (field_fixed_sizes_value),                                           \
        .field_count = (field_count_value),                                                       \
    })

typedef struct
{
    const uint8_t *ptr;
    size_t remaining;
} fuzz_input_t;

typedef struct
{
    const uint8_t *data;
    size_t data_len;
    uint8_t mode;
} fuzz_write_ctx_t;

typedef struct
{
    size_t len;
    uint8_t mode;
    size_t call_count;
} fuzz_test_write_ctx_t;

static uint8_t fuzz_take_u8(fuzz_input_t *input)
{
    if ((input == NULL) || (input->remaining == 0u))
    {
        return 0u;
    }

    uint8_t value = input->ptr[0];
    input->ptr++;
    input->remaining--;
    return value;
}

static uint64_t fuzz_take_u64(fuzz_input_t *input)
{
    uint64_t value = 0u;

    for (size_t i = 0u; i < 8u; i++)
    {
        value |= ((uint64_t)fuzz_take_u8(input)) << (8u * i);
    }

    return value;
}

static uint64_t fuzz_take_u64_bounded(fuzz_input_t *input, uint64_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    if (max_inclusive == UINT64_MAX)
    {
        return value;
    }

    return value % (max_inclusive + 1u);
}

static size_t fuzz_take_size_bounded(fuzz_input_t *input, size_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    uint64_t bound = (uint64_t)max_inclusive;
    return (size_t)(value % (bound + 1u));
}

static void fuzz_fill_bytes(fuzz_input_t *input, uint8_t *out, size_t out_len)
{
    if (out == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < out_len; i++)
    {
        out[i] = fuzz_take_u8(input);
    }
}

static uint8_t fuzz_codec_byte(const fuzz_write_ctx_t *ctx, size_t index)
{
    if ((ctx == NULL) || (ctx->data == NULL) || (ctx->data_len == 0u))
    {
        return (uint8_t)index;
    }

    return ctx->data[index % ctx->data_len];
}

static size_t fuzz_member_len(const fuzz_write_ctx_t *ctx, uint64_t member_id)
{
    uint8_t seed = fuzz_codec_byte(ctx, (size_t)member_id);
    if ((ctx != NULL) && ((ctx->mode & 2u) != 0u))
    {
        return (size_t)(((seed >> 3u) % 4u) + 1u);
    }
    return (size_t)((seed % 8u) + 1u);
}

static ssz_error_t fuzz_member_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const fuzz_write_ctx_t *state = (const fuzz_write_ctx_t *)ctx;

    if ((state == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (((state->mode & 1u) != 0u) && ((member_id & 1u) != 0u))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    size_t produced = fuzz_member_len(state, member_id);

    if (out == NULL)
    {
        *out_written = produced;
        return SSZ_SUCCESS;
    }
    if (out_cap < produced)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0u; i < produced; i++)
    {
        out[i] = fuzz_codec_byte(state, (size_t)member_id + i + 1u);
    }

    if ((state->mode & 4u) != 0u)
    {
        *out_written = produced - 1u;
    }
    else
    {
        *out_written = produced;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_test_member_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    (void)member_id;

    fuzz_test_write_ctx_t *state = (fuzz_test_write_ctx_t *)ctx;
    if ((state == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    size_t len = state->len;
    if (state->mode == 5u)
    {
        len = SIZE_MAX;
    }
    else if (state->mode == 6u)
    {
        len = (size_t)UINT32_MAX;
    }
    else if (state->mode == 8u)
    {
        len = (state->call_count == 0u) ? 1u : 2u;
    }
    else if (state->mode == 9u)
    {
        len = (state->call_count == 0u) ? 1u : SIZE_MAX;
    }

    if (out == NULL)
    {
        if (state->mode == 1u)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        if ((state->mode == 2u) && (state->call_count > 0u))
        {
            state->call_count++;
            return SSZ_ERR_TYPE_MISMATCH;
        }
        *out_written = len;
        state->call_count++;
        return SSZ_SUCCESS;
    }

    if (state->mode == 3u)
    {
        state->call_count++;
        return SSZ_ERR_TYPE_MISMATCH;
    }
    if (state->mode == 9u)
    {
        *out_written = SIZE_MAX;
        state->call_count++;
        return SSZ_SUCCESS;
    }
    if (out_cap < len)
    {
        state->call_count++;
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0u; i < len; i++)
    {
        out[i] = (uint8_t)i;
    }

    if ((state->mode == 4u) && (len != 0u))
    {
        *out_written = len - 1u;
    }
    else
    {
        *out_written = len;
    }
    state->call_count++;
    return SSZ_SUCCESS;
}

static void fuzz_cover_serialize_errors(void)
{
    uint8_t out[32] = {0u};
    uint8_t bits_ok[2] = {0u};
    uint8_t bits_bad[2] = {0xFFu, 0xFFu};
    uint8_t elems[8] = {0u};
    size_t out_len = 0u;

    (void)ssz_serialize_bitvector(bits_ok, 1u, 8u, out, 0u, &out_len);
    (void)ssz_serialize_bitvector(bits_ok, 1u, UINT64_MAX, out, sizeof(out), &out_len);
    (void)ssz_serialize_bitvector(NULL, 0u, 8u, out, sizeof(out), &out_len);
    (void)ssz_serialize_bitvector(bits_bad, 1u, 1u, out, sizeof(out), &out_len);

    (void)ssz_serialize_bitlist(bits_ok, 1u, UINT64_MAX, SSZ_NO_LIMIT, out, sizeof(out), &out_len);
    (void)ssz_serialize_bitlist(NULL, 0u, 8u, SSZ_NO_LIMIT, out, sizeof(out), &out_len);
    (void)ssz_serialize_bitlist(bits_bad, 2u, 9u, SSZ_NO_LIMIT, out, sizeof(out), &out_len);

    (void)ssz_serialize_vector_fixed(elems, 2u, 2u, out, 1u, &out_len);
    (void)ssz_serialize_vector_fixed(NULL, 2u, 2u, out, sizeof(out), &out_len);

    {
        fuzz_test_write_ctx_t ctx_ok = {
            .len = 2u,
            .mode = 0u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_ok = {
            .ctx = &ctx_ok,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_size_err = {
            .len = 2u,
            .mode = 1u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_size_err = {
            .ctx = &ctx_size_err,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_second_size_err = {
            .len = 2u,
            .mode = 2u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_second_size_err = {
            .ctx = &ctx_second_size_err,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_write_err = {
            .len = 2u,
            .mode = 3u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_write_err = {
            .ctx = &ctx_write_err,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_short_write = {
            .len = 2u,
            .mode = 4u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_short_write = {
            .ctx = &ctx_short_write,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_size_max = {
            .len = 2u,
            .mode = 5u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_size_max = {
            .ctx = &ctx_size_max,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_u32_max = {
            .len = 2u,
            .mode = 6u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_u32_max = {
            .ctx = &ctx_u32_max,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        fuzz_test_write_ctx_t ctx_cursor_mismatch = {
            .len = 2u,
            .mode = 8u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_cursor_mismatch = {
            .ctx = &ctx_cursor_mismatch,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };
        fuzz_test_write_ctx_t ctx_cursor_overflow = {
            .len = 2u,
            .mode = 9u,
            .call_count = 0u,
        };
        ssz_member_codec_t codec_cursor_overflow = {
            .ctx = &ctx_cursor_overflow,
            .write = fuzz_test_member_write,
            .read = NULL,
            .root = NULL,
        };

        (void)ssz_serialize_vector_variable(1u, NULL, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(
            (uint64_t)SIZE_MAX,
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
#if UINT64_MAX > SIZE_MAX
        (void)ssz_serialize_vector_variable(
            (uint64_t)SIZE_MAX + 1u,
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
#endif
        (void)ssz_serialize_vector_variable(2u, &codec_size_err, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(2u, &codec_size_max, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(2u, &codec_u32_max, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_ok, out, 0u, &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_second_size_err, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_write_err, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_short_write, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_cursor_mismatch, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(1u, &codec_cursor_overflow, out, sizeof(out), &out_len);

        (void)ssz_serialize_list_fixed(NULL, 2u, SSZ_NO_LIMIT, 2u, out, sizeof(out), &out_len);
        (void)ssz_serialize_list_fixed(
            elems,
            (uint64_t)SIZE_MAX,
            SSZ_NO_LIMIT,
            2u,
            out,
            sizeof(out),
            &out_len);
#if UINT64_MAX > SIZE_MAX
        (void)ssz_serialize_list_fixed(
            elems,
            (uint64_t)SIZE_MAX + 1u,
            SSZ_NO_LIMIT,
            1u,
            out,
            sizeof(out),
            &out_len);
#endif
        (void)ssz_serialize_list_fixed(elems, 2u, SSZ_NO_LIMIT, 2u, out, 1u, &out_len);

        (void)ssz_serialize_list_variable(2u, SSZ_NO_LIMIT, NULL, out, sizeof(out), &out_len);
        (void)ssz_serialize_list_variable(
            (uint64_t)SIZE_MAX,
            SSZ_NO_LIMIT,
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
#if UINT64_MAX > SIZE_MAX
        (void)ssz_serialize_list_variable(
            (uint64_t)SIZE_MAX + 1u,
            SSZ_NO_LIMIT,
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
#endif
        (void)ssz_serialize_list_variable(
            2u,
            SSZ_NO_LIMIT,
            &codec_size_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            2u,
            SSZ_NO_LIMIT,
            &codec_size_max,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            2u,
            SSZ_NO_LIMIT,
            &codec_u32_max,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(1u, SSZ_NO_LIMIT, &codec_ok, out, 0u, &out_len);
        (void)ssz_serialize_list_variable(
            1u,
            SSZ_NO_LIMIT,
            &codec_second_size_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            1u,
            SSZ_NO_LIMIT,
            &codec_write_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            1u,
            SSZ_NO_LIMIT,
            &codec_short_write,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            1u,
            SSZ_NO_LIMIT,
            &codec_cursor_mismatch,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            1u,
            SSZ_NO_LIMIT,
            &codec_cursor_overflow,
            out,
            sizeof(out),
            &out_len);

        const size_t field_fixed_sizes_bad[2] = {SIZE_MAX, 1u};
        const size_t field_fixed_sizes_var[1] = {0u};
        const size_t field_fixed_sizes_fixed2[1] = {2u};
        const size_t field_fixed_sizes_fixed3[1] = {3u};
        (void)ssz_serialize_container(CONTAINER_SCHEMA(field_fixed_sizes_var, 1u), NULL, out, sizeof(out), &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_bad, 2u),
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_size_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_size_max,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_fixed3, 1u),
            &codec_ok,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_u32_max,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(CONTAINER_SCHEMA(field_fixed_sizes_var, 1u), &codec_ok, out, 0u, &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_second_size_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_write_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_short_write,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_var, 1u),
            &codec_cursor_overflow,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_fixed2, 1u),
            &codec_write_err,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes_fixed2, 1u),
            &codec_short_write,
            out,
            sizeof(out),
            &out_len);

        (void)ssz_serialize_union(0u, 0u, false, &codec_ok, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(0u, 257u, false, &codec_ok, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(0u, 1u, true, &codec_ok, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(2u, 2u, false, &codec_ok, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(1u, 2u, false, NULL, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(1u, 2u, false, &codec_size_err, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(1u, 2u, false, &codec_size_max, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(1u, 2u, false, &codec_ok, out, 1u, &out_len);
        (void)ssz_serialize_union(1u, 2u, false, &codec_write_err, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(1u, 2u, false, &codec_short_write, out, sizeof(out), &out_len);

        {
            const uint8_t allowed_good[1] = {1u};
            const uint8_t allowed_bad[1] = {0u};
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_bad,
                1u,
                &codec_ok,
                out,
                sizeof(out),
                &out_len);
            (void)ssz_serialize_compatible_union(
                2u,
                allowed_good,
                1u,
                &codec_ok,
                out,
                sizeof(out),
                &out_len);
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_good,
                1u,
                NULL,
                out,
                sizeof(out),
                &out_len);
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_good,
                1u,
                &codec_size_err,
                out,
                sizeof(out),
                &out_len);
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_good,
                1u,
                &codec_size_max,
                out,
                sizeof(out),
                &out_len);
            (void)
                ssz_serialize_compatible_union(1u, allowed_good, 1u, &codec_ok, out, 1u, &out_len);
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_good,
                1u,
                &codec_write_err,
                out,
                sizeof(out),
                &out_len);
            (void)ssz_serialize_compatible_union(
                1u,
                allowed_good,
                1u,
                &codec_short_write,
                out,
                sizeof(out),
                &out_len);
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if ((data == NULL) || (size == 0u))
    {
        return 0;
    }

    fuzz_input_t input = {
        .ptr = data,
        .remaining = size,
    };

    uint8_t api_selector = fuzz_take_u8(&input);

    switch (api_selector % 16u)
    {
    case 0u:
    {
        uint8_t value = fuzz_take_u8(&input);
        uint8_t out[1] = {0u};
        (void)ssz_serialize_boolean(value, out);
        (void)ssz_serialize_boolean(value, NULL);
        break;
    }

    case 1u:
    {
        uint8_t value = fuzz_take_u8(&input);
        uint8_t out[1] = {0u};
        (void)ssz_serialize_uint8(value, out);
        (void)ssz_serialize_uint8(value, NULL);
        break;
    }

    case 2u:
    {
        uint16_t value = (uint16_t)fuzz_take_u64(&input);
        uint8_t out[2] = {0u};
        (void)ssz_serialize_uint16(value, out);
        (void)ssz_serialize_uint16(value, NULL);
        break;
    }

    case 3u:
    {
        uint32_t value = (uint32_t)fuzz_take_u64(&input);
        uint8_t out[4] = {0u};
        (void)ssz_serialize_uint32(value, out);
        (void)ssz_serialize_uint32(value, NULL);
        break;
    }

    case 4u:
    {
        uint64_t value = fuzz_take_u64(&input);
        uint8_t out[8] = {0u};
        (void)ssz_serialize_uint64(value, out);
        (void)ssz_serialize_uint64(value, NULL);
        break;
    }

    case 5u:
    {
        uint8_t value[16] = {0u};
        uint8_t out[16] = {0u};
        fuzz_fill_bytes(&input, value, sizeof(value));
        (void)ssz_serialize_uint128(value, sizeof(value), out);
        (void)ssz_serialize_uint128(NULL, sizeof(value), out);
        (void)ssz_serialize_uint128(value, sizeof(value), NULL);
        break;
    }

    case 6u:
    {
        uint8_t value[32] = {0u};
        uint8_t out[32] = {0u};
        fuzz_fill_bytes(&input, value, sizeof(value));
        (void)ssz_serialize_uint256(value, sizeof(value), out);
        (void)ssz_serialize_uint256(NULL, sizeof(value), out);
        (void)ssz_serialize_uint256(value, sizeof(value), NULL);
        break;
    }

    case 7u:
    {
        uint64_t bit_count = fuzz_take_u64_bounded(&input, 511u) + 1u;
        size_t byte_count = (size_t)((bit_count + 7u) / 8u);
        uint8_t bits[64] = {0u};
        fuzz_fill_bytes(&input, bits, byte_count);
        if ((bit_count % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
            bits[byte_count - 1u] = (uint8_t)(bits[byte_count - 1u] & mask);
        }

        uint8_t out[64] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_bitvector(bits, byte_count, bit_count, NULL, 0u, &out_len);
        (void)ssz_serialize_bitvector(bits, byte_count, bit_count, out, sizeof(out), &out_len);
        (void)ssz_serialize_bitvector(bits, byte_count, bit_count, out, sizeof(out), NULL);
        (void)ssz_serialize_bitvector(bits, byte_count, 0u, out, sizeof(out), &out_len);
        break;
    }

    case 8u:
    {
        uint64_t bit_len = fuzz_take_u64_bounded(&input, 512u);
        size_t data_bytes = (size_t)((bit_len + 7u) / 8u);
        uint8_t bits[64] = {0u};
        fuzz_fill_bytes(&input, bits, data_bytes);
        if ((bit_len != 0u) && ((bit_len % 8u) != 0u))
        {
            uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
            bits[data_bytes - 1u] = (uint8_t)(bits[data_bytes - 1u] & mask);
        }

        uint64_t bit_limit = ((fuzz_take_u8(&input) & 1u) != 0u)
                                 ? SSZ_NO_LIMIT
                                 : (bit_len + fuzz_take_u64_bounded(&input, 32u));

        uint8_t out[80] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_bitlist(bits, data_bytes, bit_len, bit_limit, NULL, 0u, &out_len);
        (void)
            ssz_serialize_bitlist(bits, data_bytes, bit_len, bit_limit, out, sizeof(out), &out_len);
        (void)ssz_serialize_bitlist(bits, data_bytes, bit_len, bit_limit, out, sizeof(out), NULL);
        if (bit_len != 0u)
        {
            (void)ssz_serialize_bitlist(
                bits,
                data_bytes,
                bit_len,
                bit_len - 1u,
                out,
                sizeof(out),
                &out_len);
        }
        break;
    }

    case 9u:
    {
        uint64_t element_count = fuzz_take_u64_bounded(&input, 63u) + 1u;
        size_t element_size = fuzz_take_size_bounded(&input, 31u) + 1u;
        size_t required = (size_t)element_count * element_size;

        uint8_t elements[2048] = {0u};
        uint8_t out[2048] = {0u};
        size_t out_len = 0u;

        fuzz_fill_bytes(&input, elements, required);

        (void)ssz_serialize_vector_fixed(elements, element_count, element_size, NULL, 0u, &out_len);
        (void)ssz_serialize_vector_fixed(
            elements,
            element_count,
            element_size,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_vector_fixed(
            elements,
            element_count,
            element_size,
            out,
            sizeof(out),
            NULL);
        (void)ssz_serialize_vector_fixed(elements, 0u, element_size, out, sizeof(out), &out_len);
        break;
    }

    case 10u:
    {
        uint64_t element_count = fuzz_take_u64_bounded(&input, 63u) + 1u;
        fuzz_write_ctx_t write_ctx = {
            .data = input.ptr,
            .data_len = input.remaining,
            .mode = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &write_ctx,
            .write = fuzz_member_write,
            .read = NULL,
            .root = NULL,
        };

        uint8_t out[1024] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_vector_variable(element_count, &codec, NULL, 0u, &out_len);
        (void)ssz_serialize_vector_variable(element_count, &codec, out, sizeof(out), &out_len);
        (void)ssz_serialize_vector_variable(element_count, &codec, out, sizeof(out), NULL);
        (void)ssz_serialize_vector_variable(0u, &codec, out, sizeof(out), &out_len);
        break;
    }

    case 11u:
    {
        uint64_t element_count = fuzz_take_u64_bounded(&input, 64u);
        size_t element_size = fuzz_take_size_bounded(&input, 31u) + 1u;
        size_t required = (size_t)element_count * element_size;
        uint64_t element_limit = ((fuzz_take_u8(&input) & 1u) != 0u)
                                     ? SSZ_NO_LIMIT
                                     : (element_count + fuzz_take_u64_bounded(&input, 8u));

        uint8_t elements[2048] = {0u};
        uint8_t out[2048] = {0u};
        size_t out_len = 0u;

        fuzz_fill_bytes(&input, elements, required);

        (void)ssz_serialize_list_fixed(
            elements,
            element_count,
            element_limit,
            element_size,
            NULL,
            0u,
            &out_len);
        (void)ssz_serialize_list_fixed(
            elements,
            element_count,
            element_limit,
            element_size,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_fixed(
            elements,
            element_count,
            element_limit,
            element_size,
            out,
            sizeof(out),
            NULL);
        if (element_count != 0u)
        {
            (void)ssz_serialize_list_fixed(
                elements,
                element_count,
                element_count - 1u,
                element_size,
                out,
                sizeof(out),
                &out_len);
        }
        (void)ssz_serialize_list_fixed(
            elements,
            element_count,
            element_limit,
            0u,
            out,
            sizeof(out),
            &out_len);
        break;
    }

    case 12u:
    {
        uint64_t element_count = fuzz_take_u64_bounded(&input, 64u);
        uint64_t element_limit = ((fuzz_take_u8(&input) & 1u) != 0u)
                                     ? SSZ_NO_LIMIT
                                     : (element_count + fuzz_take_u64_bounded(&input, 8u));

        fuzz_write_ctx_t write_ctx = {
            .data = input.ptr,
            .data_len = input.remaining,
            .mode = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &write_ctx,
            .write = fuzz_member_write,
            .read = NULL,
            .root = NULL,
        };

        uint8_t out[1024] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_list_variable(element_count, element_limit, &codec, NULL, 0u, &out_len);
        (void)ssz_serialize_list_variable(
            element_count,
            element_limit,
            &codec,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_list_variable(
            element_count,
            element_limit,
            &codec,
            out,
            sizeof(out),
            NULL);
        if (element_count != 0u)
        {
            (void)ssz_serialize_list_variable(
                element_count,
                element_count - 1u,
                &codec,
                out,
                sizeof(out),
                &out_len);
        }
        break;
    }

    case 13u:
    {
        uint32_t field_count = (uint32_t)(fuzz_take_u64_bounded(&input, 7u) + 1u);
        size_t field_fixed_sizes[8] = {0u};

        fuzz_write_ctx_t write_ctx = {
            .data = input.ptr,
            .data_len = input.remaining,
            .mode = 0u,
        };
        for (uint32_t i = 0u; i < field_count; i++)
        {
            field_fixed_sizes[i] = ((i & 1u) == 0u) ? fuzz_member_len(&write_ctx, i) : 0u;
        }

        ssz_member_codec_t codec = {
            .ctx = &write_ctx,
            .write = fuzz_member_write,
            .read = NULL,
            .root = NULL,
        };

        uint8_t out[512] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_container(CONTAINER_SCHEMA(field_fixed_sizes, field_count), &codec, NULL, 0u, &out_len);
        (void)ssz_serialize_container(
            CONTAINER_SCHEMA(field_fixed_sizes, field_count),
            &codec,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_container(CONTAINER_SCHEMA(field_fixed_sizes, field_count), &codec, out, sizeof(out), NULL);
        (void)ssz_serialize_container(NULL, &codec, out, sizeof(out), &out_len);
        (void)ssz_serialize_container(CONTAINER_SCHEMA(field_fixed_sizes, 0u), &codec, out, sizeof(out), &out_len);
        break;
    }

    case 14u:
    {
        uint32_t option_count = (uint32_t)(fuzz_take_u64_bounded(&input, 15u) + 2u);
        uint8_t selector = (uint8_t)fuzz_take_u64_bounded(&input, option_count - 1u);
        uint8_t normal_selector = (uint8_t)((selector % (option_count - 1u)) + 1u);

        fuzz_write_ctx_t write_ctx = {
            .data = input.ptr,
            .data_len = input.remaining,
            .mode = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &write_ctx,
            .write = fuzz_member_write,
            .read = NULL,
            .root = NULL,
        };

        uint8_t out[256] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_union(0u, option_count, true, &codec, NULL, 0u, &out_len);
        (void)ssz_serialize_union(0u, option_count, true, &codec, out, sizeof(out), &out_len);
        (void)ssz_serialize_union(normal_selector, option_count, true, &codec, NULL, 0u, &out_len);
        (void)ssz_serialize_union(
            normal_selector,
            option_count,
            true,
            &codec,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_union(
            normal_selector,
            option_count,
            true,
            &codec,
            out,
            sizeof(out),
            NULL);
        (void)ssz_serialize_union(
            (uint8_t)option_count,
            option_count,
            true,
            &codec,
            out,
            sizeof(out),
            &out_len);
        break;
    }

    default:
    {
        uint32_t allowed_selector_count = (uint32_t)(fuzz_take_u64_bounded(&input, 7u) + 1u);
        uint8_t allowed_selectors[8] = {0u};
        for (uint32_t i = 0u; i < allowed_selector_count; i++)
        {
            allowed_selectors[i] = (uint8_t)(1u + (fuzz_take_u8(&input) % 127u));
        }
        uint8_t selector = allowed_selectors[fuzz_take_u8(&input) % allowed_selector_count];

        fuzz_write_ctx_t write_ctx = {
            .data = input.ptr,
            .data_len = input.remaining,
            .mode = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &write_ctx,
            .write = fuzz_member_write,
            .read = NULL,
            .root = NULL,
        };

        uint8_t out[256] = {0u};
        size_t out_len = 0u;

        (void)ssz_serialize_compatible_union(
            selector,
            allowed_selectors,
            allowed_selector_count,
            &codec,
            NULL,
            0u,
            &out_len);
        (void)ssz_serialize_compatible_union(
            selector,
            allowed_selectors,
            allowed_selector_count,
            &codec,
            out,
            sizeof(out),
            &out_len);
        (void)ssz_serialize_compatible_union(
            selector,
            allowed_selectors,
            allowed_selector_count,
            &codec,
            out,
            sizeof(out),
            NULL);
        (void)ssz_serialize_compatible_union(
            0u,
            allowed_selectors,
            allowed_selector_count,
            &codec,
            out,
            sizeof(out),
            &out_len);
        (void)
            ssz_serialize_compatible_union(selector, NULL, 0u, &codec, out, sizeof(out), &out_len);
        break;
    }
    }

    fuzz_cover_serialize_errors();

    (void)ssz_error_string(SSZ_SUCCESS);
    (void)ssz_error_string(SSZ_ERR_INVALID_ARGUMENT);
    (void)ssz_error_string(SSZ_ERR_BUFFER_TOO_SMALL);
    (void)ssz_error_string(SSZ_ERR_OVERFLOW);
    (void)ssz_error_string(SSZ_ERR_LIMIT_EXCEEDED);
    (void)ssz_error_string(SSZ_ERR_SCHEMA_INVALID);
    (void)ssz_error_string(SSZ_ERR_ENCODING_INVALID);
    (void)ssz_error_string(SSZ_ERR_OFFSET_INVALID);
    (void)ssz_error_string(SSZ_ERR_TYPE_MISMATCH);
    (void)ssz_error_string(SSZ_ERR_SELECTOR_INVALID);
    (void)ssz_error_string(SSZ_ERR_GINDEX_INVALID);
    (void)ssz_error_string(SSZ_ERR_PROOF_INVALID);
    (void)ssz_error_string(SSZ_ERR_HASH_FAILURE);
    (void)ssz_error_string((ssz_error_t)99);

    return 0;
}
