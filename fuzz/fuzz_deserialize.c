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

    return 0;
}
