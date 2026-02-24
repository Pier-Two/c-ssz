#include <stddef.h>
#include <stdint.h>

#include "ssz.h"

typedef struct
{
    const uint8_t *ptr;
    size_t remaining;
} fuzz_input_t;

typedef struct
{
    uint8_t mode;
} fuzz_codec_ctx_t;

typedef struct
{
    uint8_t *buffer;
    size_t buffer_len;
} fuzz_mutate_ctx_t;

typedef struct
{
    size_t *field_fixed_sizes;
    uint32_t field_count;
    uint64_t trigger_member_id;
    uint32_t target_index;
    size_t replacement_size;
} fuzz_field_size_mutate_ctx_t;

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

static ssz_error_t fuzz_member_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    fuzz_codec_ctx_t *state = (fuzz_codec_ctx_t *)ctx;

    if ((state == NULL) || ((data_len != 0u) && (data == NULL)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    switch (state->mode & 3u)
    {
        case 0u:
            return SSZ_SUCCESS;
        case 1u:
            if ((member_id & 1u) != 0u)
            {
                return SSZ_ERR_TYPE_MISMATCH;
            }
            return SSZ_SUCCESS;
        case 2u:
            if (data_len > 128u)
            {
                return SSZ_ERR_LIMIT_EXCEEDED;
            }
            return SSZ_SUCCESS;
        default:
            if (data_len == 0u)
            {
                return SSZ_ERR_OFFSET_INVALID;
            }
            return SSZ_SUCCESS;
    }
}

static ssz_error_t fuzz_member_read_mutate(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    fuzz_mutate_ctx_t *state = (fuzz_mutate_ctx_t *)ctx;
    (void)data;
    (void)data_len;

    if (state == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((member_id == 2u) && (state->buffer != NULL) && (state->buffer_len >= 8u))
    {
        /* Mutate the second variable offset after the first pass offset checks. */
        state->buffer[4] = 4u;
        state->buffer[5] = 0u;
        state->buffer[6] = 0u;
        state->buffer[7] = 0u;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_member_read_mutate_field_size(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    fuzz_field_size_mutate_ctx_t *state = (fuzz_field_size_mutate_ctx_t *)ctx;
    (void)data;
    (void)data_len;

    if ((state == NULL) || (state->field_fixed_sizes == NULL) || (state->target_index >= state->field_count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (member_id == state->trigger_member_id)
    {
        state->field_fixed_sizes[state->target_index] = state->replacement_size;
    }

    return SSZ_SUCCESS;
}

static void fuzz_cover_deserialize_errors(void)
{
    uint8_t in[32] = {0u};
    uint8_t out_u8 = 0u;
    uint16_t out_u16 = 0u;
    uint32_t out_u32 = 0u;
    uint64_t out_u64 = 0u;
    uint8_t out_u128[16] = {0u};
    uint8_t out_u256[32] = {0u};
    uint8_t out_bits[32] = {0u};
    uint64_t out_bit_len = 0u;
    uint64_t out_element_count = 0u;
    uint8_t out_selector = 0u;

    fuzz_codec_ctx_t codec_ctx = {
        .mode = 0u,
    };
    ssz_member_codec_t codec = {
        .ctx = &codec_ctx,
        .write = NULL,
        .read = fuzz_member_read,
        .root = NULL,
    };
    ssz_member_codec_t codec_no_read = {
        .ctx = &codec_ctx,
        .write = NULL,
        .read = NULL,
        .root = NULL,
    };

    uint8_t bad_bool[1] = {2u};
    uint8_t bad_mask[1] = {0xFEu};
    uint8_t bitlist_data[2] = {0xAAu, 0x01u};
    uint8_t compat_in_valid[2] = {1u, 0u};
    uint8_t container_offset_oob[8] = {9u, 0u, 0u, 0u, 9u, 0u, 0u, 0u};
    uint8_t container_offset_order[8] = {8u, 0u, 0u, 0u, 4u, 0u, 0u, 0u};
    uint8_t container_last_var[12] = {12u, 0u, 0u, 0u, 12u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    uint8_t container_fixed_overflow[5] = {0u};
    uint8_t container_cursor_mismatch[5] = {0u, 2u, 0u, 0u, 0u};

    size_t container_fixed_overflow_sizes[2] = {4u, 1u};
    size_t container_cursor_mismatch_sizes[2] = {1u, 1u};

    fuzz_mutate_ctx_t mutate_ctx = {
        .buffer = container_last_var,
        .buffer_len = sizeof(container_last_var),
    };
    fuzz_field_size_mutate_ctx_t fixed_overflow_ctx = {
        .field_fixed_sizes = container_fixed_overflow_sizes,
        .field_count = 2u,
        .trigger_member_id = 0u,
        .target_index = 1u,
        .replacement_size = 4u,
    };
    fuzz_field_size_mutate_ctx_t cursor_mismatch_ctx = {
        .field_fixed_sizes = container_cursor_mismatch_sizes,
        .field_count = 2u,
        .trigger_member_id = 0u,
        .target_index = 1u,
        .replacement_size = 0u,
    };

    ssz_member_codec_t codec_mutate = {
        .ctx = &mutate_ctx,
        .write = NULL,
        .read = fuzz_member_read_mutate,
        .root = NULL,
    };
    ssz_member_codec_t codec_mutate_fixed_overflow = {
        .ctx = &fixed_overflow_ctx,
        .write = NULL,
        .read = fuzz_member_read_mutate_field_size,
        .root = NULL,
    };
    ssz_member_codec_t codec_mutate_cursor_mismatch = {
        .ctx = &cursor_mismatch_ctx,
        .write = NULL,
        .read = fuzz_member_read_mutate_field_size,
        .root = NULL,
    };

    (void)ssz_deserialize_boolean(in, NULL);
    (void)ssz_deserialize_uint8(in, NULL);
    (void)ssz_deserialize_uint16(in, NULL);
    (void)ssz_deserialize_uint32(in, NULL);
    (void)ssz_deserialize_uint64(in, NULL);
    (void)ssz_deserialize_uint128(in, NULL);
    (void)ssz_deserialize_uint256(in, NULL);
    (void)ssz_deserialize_boolean(bad_bool, &out_u8);

    (void)ssz_deserialize_uint8(NULL, &out_u8);
    (void)ssz_deserialize_uint16(NULL, &out_u16);
    (void)ssz_deserialize_uint32(NULL, &out_u32);
    (void)ssz_deserialize_uint64(NULL, &out_u64);
    (void)ssz_deserialize_uint128(NULL, out_u128);
    (void)ssz_deserialize_uint256(NULL, out_u256);

    (void)ssz_deserialize_bitvector(in, 1u, 0u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_bitvector(in, 1u, UINT64_MAX, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_bitvector(NULL, 1u, 8u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_bitvector(in, 1u, 8u, NULL, 1u);
    (void)ssz_deserialize_bitvector(bad_mask, 1u, 1u, out_bits, sizeof(out_bits));

    (void)ssz_deserialize_bitlist(in, 1u, SSZ_NO_LIMIT, out_bits, sizeof(out_bits), NULL);
    (void)ssz_deserialize_bitlist(NULL, 0u, SSZ_NO_LIMIT, out_bits, sizeof(out_bits), &out_bit_len);
    (void)ssz_deserialize_bitlist((uint8_t[1]){0u}, 1u, SSZ_NO_LIMIT, out_bits, sizeof(out_bits), &out_bit_len);
    (void)ssz_deserialize_bitlist(bitlist_data, 2u, SSZ_NO_LIMIT, NULL, 0u, &out_bit_len);

    (void)ssz_deserialize_vector_fixed(in, 4u, 0u, 1u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_vector_fixed(in, 4u, 1u, 0u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_vector_fixed(in, 4u, UINT64_MAX, 2u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_vector_fixed(in, 4u, 65536u, 65537u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_vector_fixed(NULL, 4u, 1u, 4u, out_bits, sizeof(out_bits));
    (void)ssz_deserialize_vector_fixed(in, 4u, 1u, 4u, NULL, 0u);

    (void)ssz_deserialize_vector_variable(in, 0u, 0u, 0u, &codec);
    (void)ssz_deserialize_vector_variable(in, 1u, 0u, 0u, &codec);
    (void)ssz_deserialize_vector_variable(NULL, 4u, 1u, 0u, &codec);
    (void)ssz_deserialize_vector_variable(in, 4u, 1u, 0u, NULL);
    (void)ssz_deserialize_vector_variable(in, 4u, 1u, 0u, &codec_no_read);
    (void)ssz_deserialize_vector_variable(in, 4u, UINT64_MAX, 0u, &codec);
    (void)ssz_deserialize_vector_variable(in, 4u, (uint64_t)SIZE_MAX, 0u, &codec);
#if UINT64_MAX > SIZE_MAX
    (void)ssz_deserialize_vector_variable(in, 4u, (uint64_t)SIZE_MAX + 1u, 0u, &codec);
#endif

    (void)ssz_deserialize_list_fixed(in, 4u, SSZ_NO_LIMIT, 1u, out_bits, sizeof(out_bits), NULL);
    (void)ssz_deserialize_list_fixed(in, 3u, SSZ_NO_LIMIT, 2u, out_bits, sizeof(out_bits), &out_element_count);
    (void)ssz_deserialize_list_fixed(NULL, 4u, SSZ_NO_LIMIT, 2u, out_bits, sizeof(out_bits), &out_element_count);
    (void)ssz_deserialize_list_fixed(in, 4u, SSZ_NO_LIMIT, 2u, NULL, 0u, &out_element_count);

    (void)ssz_deserialize_list_variable(in, 4u, SSZ_NO_LIMIT, 0u, &codec, NULL);
    (void)ssz_deserialize_list_variable(in, 4u, SSZ_NO_LIMIT, 0u, NULL, &out_element_count);
    (void)ssz_deserialize_list_variable(in, 4u, SSZ_NO_LIMIT, 0u, &codec_no_read, &out_element_count);
    (void)ssz_deserialize_list_variable(NULL, 4u, SSZ_NO_LIMIT, 0u, &codec, &out_element_count);

    (void)ssz_deserialize_container(in, 0u, NULL, 1u, &codec);
    (void)ssz_deserialize_container(NULL, 1u, (size_t[1]){1u}, 1u, &codec);
    (void)ssz_deserialize_container(in, 1u, (size_t[1]){1u}, 1u, &codec_no_read);
    (void)ssz_deserialize_container(in, 1u, (size_t[2]){SIZE_MAX, 1u}, 2u, &codec);
    (void)ssz_deserialize_container(
        container_fixed_overflow,
        sizeof(container_fixed_overflow),
        container_fixed_overflow_sizes,
        2u,
        &codec_mutate_fixed_overflow);
    (void)ssz_deserialize_container(
        container_cursor_mismatch,
        sizeof(container_cursor_mismatch),
        container_cursor_mismatch_sizes,
        2u,
        &codec_mutate_cursor_mismatch);
    (void)ssz_deserialize_container(
        container_offset_oob,
        sizeof(container_offset_oob),
        (size_t[2]){0u, 0u},
        2u,
        &codec);
    (void)ssz_deserialize_container(
        container_offset_order,
        sizeof(container_offset_order),
        (size_t[2]){0u, 0u},
        2u,
        &codec);
    (void)ssz_deserialize_container(
        container_last_var,
        sizeof(container_last_var),
        (size_t[3]){0u, 0u, 4u},
        3u,
        &codec_mutate);

    (void)ssz_deserialize_union(in, 1u, 2u, false, &codec, NULL);
    (void)ssz_deserialize_union(in, 2u, 2u, false, NULL, &out_selector);
    (void)ssz_deserialize_union(in, 2u, 2u, false, &codec_no_read, &out_selector);

    (void)ssz_deserialize_compatible_union(in, 1u, (uint8_t[1]){1u}, 1u, &codec, NULL);
    (void)ssz_deserialize_compatible_union(in, 1u, (uint8_t[1]){1u}, 1u, NULL, &out_selector);
    (void)ssz_deserialize_compatible_union(in, 1u, (uint8_t[1]){1u}, 1u, &codec_no_read, &out_selector);
    (void)ssz_deserialize_compatible_union(
        compat_in_valid,
        sizeof(compat_in_valid),
        (uint8_t[1]){1u},
        1u,
        &codec_no_read,
        &out_selector);
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
            if (input.remaining < 1u)
            {
                return 0;
            }

            uint8_t out_value = 0u;
            (void)ssz_deserialize_boolean(input.ptr, &out_value);
            break;
        }

        case 1u:
        {
            if (input.remaining < 1u)
            {
                return 0;
            }

            uint8_t out_value = 0u;
            (void)ssz_deserialize_uint8(input.ptr, &out_value);
            break;
        }

        case 2u:
        {
            if (input.remaining < 2u)
            {
                return 0;
            }

            uint16_t out_value = 0u;
            (void)ssz_deserialize_uint16(input.ptr, &out_value);
            break;
        }

        case 3u:
        {
            if (input.remaining < 4u)
            {
                return 0;
            }

            uint32_t out_value = 0u;
            (void)ssz_deserialize_uint32(input.ptr, &out_value);
            break;
        }

        case 4u:
        {
            if (input.remaining < 8u)
            {
                return 0;
            }

            uint64_t out_value = 0u;
            (void)ssz_deserialize_uint64(input.ptr, &out_value);
            break;
        }

        case 5u:
        {
            if (input.remaining < 16u)
            {
                return 0;
            }

            uint8_t out_value[16] = {0u};
            (void)ssz_deserialize_uint128(input.ptr, out_value);
            break;
        }

        case 6u:
        {
            if (input.remaining < 32u)
            {
                return 0;
            }

            uint8_t out_value[32] = {0u};
            (void)ssz_deserialize_uint256(input.ptr, out_value);
            break;
        }

        case 7u:
        {
            uint64_t bit_count = fuzz_take_u64_bounded(&input, 512u);
            if (bit_count == 0u)
            {
                bit_count = 1u;
            }

            uint8_t out_bits_le[64] = {0u};
            size_t out_bits_le_len = fuzz_take_size_bounded(&input, sizeof(out_bits_le));

            (void)ssz_deserialize_bitvector(
                input.ptr,
                input.remaining,
                bit_count,
                out_bits_le,
                out_bits_le_len);
            break;
        }

        case 8u:
        {
            uint8_t limit_mode = fuzz_take_u8(&input);
            uint64_t bit_limit = (limit_mode & 1u) != 0u
                                     ? SSZ_NO_LIMIT
                                     : fuzz_take_u64_bounded(&input, 512u);

            uint8_t out_bits_le[64] = {0u};
            size_t out_bits_le_len = fuzz_take_size_bounded(&input, sizeof(out_bits_le));
            uint64_t out_bit_len = 0u;

            (void)ssz_deserialize_bitlist(
                input.ptr,
                input.remaining,
                bit_limit,
                out_bits_le,
                out_bits_le_len,
                &out_bit_len);
            break;
        }

        case 9u:
        {
            uint64_t element_count = fuzz_take_u64_bounded(&input, 64u);
            size_t element_size = fuzz_take_size_bounded(&input, 32u);
            uint8_t out_elements[512] = {0u};
            size_t out_elements_len = fuzz_take_size_bounded(&input, sizeof(out_elements));

            (void)ssz_deserialize_vector_fixed(
                input.ptr,
                input.remaining,
                element_count,
                element_size,
                out_elements,
                out_elements_len);
            break;
        }

        case 10u:
        {
            uint64_t element_count = fuzz_take_u64_bounded(&input, 32u);
            size_t min_element_size = fuzz_take_size_bounded(&input, 32u);
            fuzz_codec_ctx_t codec_ctx = {
                .mode = fuzz_take_u8(&input),
            };
            ssz_member_codec_t codec = {
                .ctx = &codec_ctx,
                .write = NULL,
                .read = fuzz_member_read,
                .root = NULL,
            };

            (void)ssz_deserialize_vector_variable(
                input.ptr,
                input.remaining,
                element_count,
                min_element_size,
                &codec);
            break;
        }

        case 11u:
        {
            uint8_t limit_mode = fuzz_take_u8(&input);
            uint64_t element_limit = (limit_mode & 1u) != 0u
                                         ? SSZ_NO_LIMIT
                                         : fuzz_take_u64_bounded(&input, 64u);
            size_t element_size = fuzz_take_size_bounded(&input, 32u);
            uint8_t out_elements[512] = {0u};
            size_t out_elements_len = fuzz_take_size_bounded(&input, sizeof(out_elements));
            uint64_t out_element_count = 0u;

            (void)ssz_deserialize_list_fixed(
                input.ptr,
                input.remaining,
                element_limit,
                element_size,
                out_elements,
                out_elements_len,
                &out_element_count);
            break;
        }

        case 12u:
        {
            uint8_t limit_mode = fuzz_take_u8(&input);
            uint64_t element_limit = (limit_mode & 1u) != 0u
                                         ? SSZ_NO_LIMIT
                                         : fuzz_take_u64_bounded(&input, 64u);
            size_t min_element_size = fuzz_take_size_bounded(&input, 32u);
            fuzz_codec_ctx_t codec_ctx = {
                .mode = fuzz_take_u8(&input),
            };
            ssz_member_codec_t codec = {
                .ctx = &codec_ctx,
                .write = NULL,
                .read = fuzz_member_read,
                .root = NULL,
            };
            uint64_t out_element_count = 0u;

            (void)ssz_deserialize_list_variable(
                input.ptr,
                input.remaining,
                element_limit,
                min_element_size,
                &codec,
                &out_element_count);
            break;
        }

        case 13u:
        {
            uint32_t field_count = (uint32_t)fuzz_take_u64_bounded(&input, 8u);
            size_t field_fixed_sizes[8] = {0u};

            for (uint32_t i = 0u; i < field_count; i++)
            {
                if ((fuzz_take_u8(&input) & 1u) == 0u)
                {
                    field_fixed_sizes[i] = 0u;
                }
                else
                {
                    field_fixed_sizes[i] = (size_t)(fuzz_take_u64_bounded(&input, 16u) + 1u);
                }
            }

            fuzz_codec_ctx_t codec_ctx = {
                .mode = fuzz_take_u8(&input),
            };
            ssz_member_codec_t codec = {
                .ctx = &codec_ctx,
                .write = NULL,
                .read = fuzz_member_read,
                .root = NULL,
            };

            (void)ssz_deserialize_container(
                input.ptr,
                input.remaining,
                field_fixed_sizes,
                field_count,
                &codec);
            break;
        }

        case 14u:
        {
            uint32_t option_count = (uint32_t)fuzz_take_u64_bounded(&input, 260u);
            bool has_none = (fuzz_take_u8(&input) & 1u) != 0u;
            fuzz_codec_ctx_t codec_ctx = {
                .mode = fuzz_take_u8(&input),
            };
            ssz_member_codec_t codec = {
                .ctx = &codec_ctx,
                .write = NULL,
                .read = fuzz_member_read,
                .root = NULL,
            };
            uint8_t out_selector = 0u;

            (void)ssz_deserialize_union(
                input.ptr,
                input.remaining,
                option_count,
                has_none,
                &codec,
                &out_selector);
            break;
        }

        default:
        {
            uint32_t allowed_selector_count = (uint32_t)fuzz_take_u64_bounded(&input, 8u);
            uint8_t allowed_selectors[8] = {0u};

            for (uint32_t i = 0u; i < allowed_selector_count; i++)
            {
                allowed_selectors[i] = fuzz_take_u8(&input);
            }

            fuzz_codec_ctx_t codec_ctx = {
                .mode = fuzz_take_u8(&input),
            };
            ssz_member_codec_t codec = {
                .ctx = &codec_ctx,
                .write = NULL,
                .read = fuzz_member_read,
                .root = NULL,
            };
            uint8_t out_selector = 0u;

            (void)ssz_deserialize_compatible_union(
                input.ptr,
                input.remaining,
                allowed_selectors,
                allowed_selector_count,
                &codec,
                &out_selector);
            break;
        }
    }

    fuzz_cover_deserialize_errors();

    return 0;
}
