#include <string.h>

#include "ssz_deserialize.h"
#include "ssz_internal.h"

static ssz_error_t ssz_internal_deserialize_variable_sequence(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t min_element_size,
    ssz_member_codec_t *codec)
{
    size_t fixed_region = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    uint32_t first_offset = 0u;
    uint32_t prev_offset = 0u;

    if ((codec == NULL) || (codec->read == NULL) || (in == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, SSZ_BYTES_PER_LENGTH_OFFSET, &fixed_region))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((fixed_region > in_len) || (fixed_region > UINT32_MAX))
    {
        err = SSZ_ERR_OFFSET_INVALID;
    }
    else
    {
        first_offset = ssz_internal_read_u32_le(in);
        if ((size_t)first_offset != fixed_region)
        {
            err = SSZ_ERR_OFFSET_INVALID;
        }
        else
        {
            prev_offset = first_offset;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        for (uint64_t i = 0u; i < element_count; i++)
        {
            size_t offset_pos = (size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET;
            uint32_t offset = ssz_internal_read_u32_le(&in[offset_pos]);

            if (((size_t)offset < fixed_region) || ((size_t)offset > in_len) || (offset < prev_offset))
            {
                err = SSZ_ERR_OFFSET_INVALID;
                break;
            }
            prev_offset = offset;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t offset_pos = (size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET;
            size_t start = (size_t)ssz_internal_read_u32_le(&in[offset_pos]);
            size_t end = in_len;

            if ((i + 1u) < element_count)
            {
                end = (size_t)ssz_internal_read_u32_le(&in[offset_pos + SSZ_BYTES_PER_LENGTH_OFFSET]);
            }
            if ((end < start) || ((end - start) < min_element_size))
            {
                err = SSZ_ERR_OFFSET_INVALID;
            }
            else
            {
                err = codec->read(codec->ctx, i, &in[start], end - start);
            }
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_uint8(const uint8_t in[1], uint8_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = in[0];
    }

    return err;
}

ssz_error_t ssz_deserialize_uint16(const uint8_t in[2], uint16_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = ssz_internal_read_u16_le(in);
    }

    return err;
}

ssz_error_t ssz_deserialize_uint32(const uint8_t in[4], uint32_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = ssz_internal_read_u32_le(in);
    }

    return err;
}

ssz_error_t ssz_deserialize_uint64(const uint8_t in[8], uint64_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = ssz_internal_read_u64_le(in);
    }

    return err;
}

ssz_error_t ssz_deserialize_uint128(const uint8_t in[16], uint8_t out_value[16])
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memcpy(out_value, in, 16u);
    }

    return err;
}

ssz_error_t ssz_deserialize_uint256(const uint8_t in[32], uint8_t out_value[32])
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memcpy(out_value, in, 32u);
    }

    return err;
}

ssz_error_t ssz_deserialize_boolean(const uint8_t in[1], uint8_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((in == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (in[0] > 1u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        *out_value = in[0];
    }

    return err;
}

ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *in,
    size_t in_len,
    uint64_t bit_count,
    uint8_t *out_bits_le,
    size_t out_bits_le_len)
{
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (bit_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_bits_to_bytes(bit_count, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((in == NULL) || (in_len != required))
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else if ((required != 0u) && ((out_bits_le == NULL) || (out_bits_le_len < required)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        if ((bit_count % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
            if ((in[required - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if ((err == SSZ_SUCCESS) && (required != 0u))
        {
            (void)memcpy(out_bits_le, in, required);
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *in,
    size_t in_len,
    uint64_t bit_limit,
    uint8_t *out_bits_le,
    size_t out_bits_le_len,
    uint64_t *out_bit_len)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t last = 0u;
    uint8_t delimiter_pos = 0u;
    uint64_t prefix_bits = 0u;
    size_t out_bytes = 0u;

    if (out_bit_len == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((in == NULL) || (in_len == 0u))
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        last = in[in_len - 1u];
        if (last == 0u)
        {
            err = SSZ_ERR_ENCODING_INVALID;
        }
    }
    if (err == SSZ_SUCCESS)
    {
        uint8_t tmp = last;

        while (tmp > 1u)
        {
            tmp >>= 1u;
            delimiter_pos++;
        }
    }
    if ((err == SSZ_SUCCESS) &&
        (ssz_internal_mul_overflow_u64((uint64_t)(in_len - 1u), 8u, &prefix_bits) ||
         ssz_internal_add_overflow_u64(prefix_bits, delimiter_pos, &prefix_bits)))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    if ((err == SSZ_SUCCESS) && (bit_limit != SSZ_NO_LIMIT) && (prefix_bits > bit_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    if ((err == SSZ_SUCCESS) && !ssz_internal_bits_to_bytes(prefix_bits, &out_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    if ((err == SSZ_SUCCESS) && (out_bytes != 0u) && ((out_bits_le == NULL) || (out_bits_le_len < out_bytes)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    if ((err == SSZ_SUCCESS) && (out_bytes != 0u))
    {
        if (delimiter_pos == 0u)
        {
            (void)memcpy(out_bits_le, in, in_len - 1u);
        }
        else
        {
            (void)memcpy(out_bits_le, in, in_len);
            out_bits_le[out_bytes - 1u] &= (uint8_t)((1u << delimiter_pos) - 1u);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_bit_len = prefix_bits;
    }

    return err;
}

ssz_error_t ssz_deserialize_vector_fixed(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t element_size,
    uint8_t *out_elements,
    size_t out_elements_len)
{
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((in == NULL) || (in_len != required))
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else if ((required != 0u) && ((out_elements == NULL) || (out_elements_len < required)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        if (required != 0u)
        {
            (void)memcpy(out_elements, in, required);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_vector_variable(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t min_element_size,
    ssz_member_codec_t *codec)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        err = ssz_internal_deserialize_variable_sequence(
            in,
            in_len,
            element_count,
            min_element_size,
            codec);
    }

    return err;
}

ssz_error_t ssz_deserialize_list_fixed(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_limit,
    size_t element_size,
    uint8_t *out_elements,
    size_t out_elements_len,
    uint64_t *out_element_count)
{
    ssz_error_t err = SSZ_SUCCESS;
    size_t count = 0u;

    if (out_element_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((in_len % element_size) != 0u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        count = in_len / element_size;
        if ((element_limit != SSZ_NO_LIMIT) && ((uint64_t)count > element_limit))
        {
            err = SSZ_ERR_LIMIT_EXCEEDED;
        }
        else if ((in == NULL) && (in_len != 0u))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if ((in_len != 0u) && ((out_elements == NULL) || (out_elements_len < in_len)))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else
        {
            if (in_len != 0u)
            {
                (void)memcpy(out_elements, in, in_len);
            }
            *out_element_count = (uint64_t)count;
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_list_variable(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_limit,
    size_t min_element_size,
    ssz_member_codec_t *codec,
    uint64_t *out_element_count)
{
    uint64_t element_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    uint32_t first_offset = 0u;
    uint32_t element_count_u32 = 0u;

    if (out_element_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if ((codec == NULL) || (codec->read == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (in_len == 0u)
    {
        *out_element_count = 0u;
    }
    else if ((in == NULL) || (in_len < SSZ_BYTES_PER_LENGTH_OFFSET))
    {
        err = SSZ_ERR_OFFSET_INVALID;
    }
    else
    {
        first_offset = ssz_internal_read_u32_le(in);
        if (((size_t)first_offset > in_len) || ((first_offset % SSZ_BYTES_PER_LENGTH_OFFSET) != 0u))
        {
            err = SSZ_ERR_OFFSET_INVALID;
        }
        else
        {
            element_count_u32 = first_offset / SSZ_BYTES_PER_LENGTH_OFFSET;
            element_count = (uint64_t)element_count_u32;
            if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
            {
                err = SSZ_ERR_LIMIT_EXCEEDED;
            }
            else if (element_count == 0u)
            {
                err = SSZ_ERR_OFFSET_INVALID;
            }
            else
            {
                err = ssz_internal_deserialize_variable_sequence(
                    in,
                    in_len,
                    element_count,
                    min_element_size,
                    codec);
                if (err == SSZ_SUCCESS)
                {
                    *out_element_count = element_count;
                }
            }
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_container(
    const uint8_t *in,
    size_t in_len,
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec)
{
    size_t fixed_region = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    size_t cursor = 0u;
    bool saw_variable = false;
    uint32_t prev_offset = 0u;

    if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->read == NULL) || ((in == NULL) && (in_len != 0u)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint32_t i = 0u; i < field_count; i++)
        {
            size_t fixed_size = field_fixed_sizes[i];
            size_t contribution = (fixed_size == 0u) ? SSZ_BYTES_PER_LENGTH_OFFSET : fixed_size;

            if (ssz_internal_add_overflow_size(fixed_region, contribution, &fixed_region))
            {
                err = SSZ_ERR_OVERFLOW;
                break;
            }
        }
        if ((err == SSZ_SUCCESS) && (fixed_region > in_len))
        {
            err = SSZ_ERR_OFFSET_INVALID;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        for (uint32_t i = 0u; (i < field_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t fixed_size = field_fixed_sizes[i];

                    if (fixed_size == 0u)
            {
                uint32_t offset = ssz_internal_read_u32_le(&in[cursor]);

                if ((size_t)offset > in_len)
                {
                    err = SSZ_ERR_OFFSET_INVALID;
                }
                else if (!saw_variable)
                {
                    if ((size_t)offset != fixed_region)
                    {
                        err = SSZ_ERR_OFFSET_INVALID;
                    }
                    else
                    {
                        saw_variable = true;
                    }
                }
                else if (offset < prev_offset)
                {
                    err = SSZ_ERR_OFFSET_INVALID;
                }
                else
                {
                    /* intentionally empty */
                }

                if (err == SSZ_SUCCESS)
                {
                    prev_offset = offset;
                    cursor += SSZ_BYTES_PER_LENGTH_OFFSET;
                }
            }
            else
            {
                if ((cursor + fixed_size) > fixed_region)
                {
                    err = SSZ_ERR_OFFSET_INVALID;
                }
                else
                {
                    err = codec->read(codec->ctx, i, &in[cursor], fixed_size);
                    if (err == SSZ_SUCCESS)
                    {
                        cursor += fixed_size;
                    }
                }
            }
                }
    }

    if ((err == SSZ_SUCCESS) && (cursor != fixed_region))
    {
        err = SSZ_ERR_OFFSET_INVALID;
    }
    if ((err == SSZ_SUCCESS) && !saw_variable)
    {
        if (fixed_region != in_len)
        {
            err = SSZ_ERR_OFFSET_INVALID;
        }
    }
    if ((err == SSZ_SUCCESS) && saw_variable)
    {
        cursor = 0u;
        for (uint32_t i = 0u; (i < field_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t fixed_size = field_fixed_sizes[i];

            if (fixed_size != 0u)
            {
                cursor += fixed_size;
            }
            else
            {
                size_t start = (size_t)ssz_internal_read_u32_le(&in[cursor]);
                size_t end = in_len;
                size_t look_cursor = cursor + SSZ_BYTES_PER_LENGTH_OFFSET;

                for (uint32_t j = i + 1u; j < field_count; j++)
                {
                    if (field_fixed_sizes[j] == 0u)
                    {
                        end = (size_t)ssz_internal_read_u32_le(&in[look_cursor]);
                        break;
                    }
                    look_cursor += field_fixed_sizes[j];
                }

                if (end < start)
                {
                    err = SSZ_ERR_OFFSET_INVALID;
                }
                else
                {
                    err = codec->read(codec->ctx, i, &in[start], end - start);
                    if (err == SSZ_SUCCESS)
                    {
                        cursor += SSZ_BYTES_PER_LENGTH_OFFSET;
                    }
                }
            }
        }
    }

    return err;
}

ssz_error_t ssz_deserialize_union(
    const uint8_t *in,
    size_t in_len,
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t selector = 0u;

    if (out_selector == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (option_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (option_count > 256u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (has_none && (option_count < 2u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((in == NULL) || (in_len < 1u))
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        selector = in[0];
        if ((uint32_t)selector >= option_count)
        {
            err = SSZ_ERR_SELECTOR_INVALID;
        }
        else if (has_none && (selector == 0u))
        {
            if (in_len != 1u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }
        else if ((codec == NULL) || (codec->read == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = codec->read(codec->ctx, selector, &in[1u], in_len - 1u);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_selector = selector;
    }

    return err;
}

ssz_error_t ssz_deserialize_compatible_union(
    const uint8_t *in,
    size_t in_len,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t selector = 0u;

    if (out_selector == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_validate_compatible_union_schema(allowed_selectors, allowed_selector_count);
        if (err == SSZ_SUCCESS)
        {
            if ((in == NULL) || (in_len < 1u))
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
            else
            {
                selector = in[0];
                if ((selector == 0u) || (selector > 127u) ||
                    !ssz_internal_selector_allowed(selector, allowed_selectors, allowed_selector_count))
                {
                    err = SSZ_ERR_SELECTOR_INVALID;
                }
                else if ((codec == NULL) || (codec->read == NULL))
                {
                    err = SSZ_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    err = codec->read(codec->ctx, selector, &in[1u], in_len - 1u);
                }
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_selector = selector;
    }

    return err;
}
