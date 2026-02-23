#include <string.h>

#include "ssz_deserialize.h"
#include "ssz_internal.h"

static bool ssz_internal_selector_allowed(
    uint8_t selector,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count)
{
    for (uint32_t i = 0u; i < allowed_selector_count; i++)
    {
        if (allowed_selectors[i] == selector)
        {
            return true;
        }
    }
    return false;
}

static ssz_error_t ssz_internal_validate_compatible_union_schema(
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count)
{
    if ((allowed_selectors == NULL) || (allowed_selector_count == 0u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    for (uint32_t i = 0u; i < allowed_selector_count; i++)
    {
        if ((allowed_selectors[i] == 0u) || (allowed_selectors[i] > 127u))
        {
            return SSZ_ERR_SCHEMA_INVALID;
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_deserialize_variable_sequence(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t min_element_size,
    ssz_member_codec_t *codec)
{
    size_t fixed_region = 0u;

    if (element_count == 0u)
    {
        return (in_len == 0u) ? SSZ_SUCCESS : SSZ_ERR_OFFSET_INVALID;
    }
    if ((codec == NULL) || (codec->read == NULL) || (in == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size((size_t)element_count, SSZ_BYTES_PER_LENGTH_OFFSET, &fixed_region))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((fixed_region > in_len) || (fixed_region > UINT32_MAX))
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    uint32_t first_offset = ssz_internal_read_u32_le(in);
    if ((size_t)first_offset != fixed_region)
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    uint32_t prev_offset = first_offset;
    for (uint64_t i = 0u; i < element_count; i++)
    {
        size_t offset_pos = (size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET;
        uint32_t offset = ssz_internal_read_u32_le(in + offset_pos);

        if (((size_t)offset < fixed_region) || ((size_t)offset > in_len) || (offset < prev_offset))
        {
            return SSZ_ERR_OFFSET_INVALID;
        }
        prev_offset = offset;
    }

    for (uint64_t i = 0u; i < element_count; i++)
    {
        size_t offset_pos = (size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET;
        size_t start = (size_t)ssz_internal_read_u32_le(in + offset_pos);
        size_t end = in_len;

        if ((i + 1u) < element_count)
        {
            end = (size_t)ssz_internal_read_u32_le(in + offset_pos + SSZ_BYTES_PER_LENGTH_OFFSET);
        }
        if ((end < start) || ((end - start) < min_element_size))
        {
            return SSZ_ERR_OFFSET_INVALID;
        }

        ssz_error_t err = codec->read(codec->ctx, i, in + start, end - start);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint8(const uint8_t in[1], uint8_t *out_value)
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_value = in[0];
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint16(const uint8_t in[2], uint16_t *out_value)
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_value = ssz_internal_read_u16_le(in);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint32(const uint8_t in[4], uint32_t *out_value)
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_value = ssz_internal_read_u32_le(in);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint64(const uint8_t in[8], uint64_t *out_value)
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    *out_value = ssz_internal_read_u64_le(in);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint128(const uint8_t in[16], uint8_t out_value[16])
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    memcpy(out_value, in, 16u);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_uint256(const uint8_t in[32], uint8_t out_value[32])
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    memcpy(out_value, in, 32u);
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_boolean(const uint8_t in[1], uint8_t *out_value)
{
    if ((in == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (in[0] > 1u)
    {
        return SSZ_ERR_ENCODING_INVALID;
    }
    *out_value = in[0];
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *in,
    size_t in_len,
    uint64_t bit_count,
    uint8_t *out_bits_le,
    size_t out_bits_le_len)
{
    size_t required = 0u;

    if (bit_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (!ssz_internal_bits_to_bytes(bit_count, &required))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((in == NULL) || (in_len != required))
    {
        return SSZ_ERR_ENCODING_INVALID;
    }
    if ((required != 0u) && ((out_bits_le == NULL) || (out_bits_le_len < required)))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if ((bit_count % 8u) != 0u)
    {
        uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
        if ((in[required - 1u] & (uint8_t)(~mask)) != 0u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
    }

    if (required != 0u)
    {
        memcpy(out_bits_le, in, required);
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *in,
    size_t in_len,
    uint64_t bit_limit,
    uint8_t *out_bits_le,
    size_t out_bits_le_len,
    uint64_t *out_bit_len)
{
    if (out_bit_len == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((in == NULL) || (in_len == 0u))
    {
        return SSZ_ERR_ENCODING_INVALID;
    }

    uint8_t last = in[in_len - 1u];
    if (last == 0u)
    {
        return SSZ_ERR_ENCODING_INVALID;
    }

    uint8_t delimiter_pos = 0u;
    for (uint8_t tmp = last; (tmp >>= 1u) != 0u;)
    {
        delimiter_pos++;
    }

    uint64_t prefix_bits = 0u;
    if (!ssz_internal_u64_to_size((uint64_t)(in_len - 1u), NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_u64((uint64_t)(in_len - 1u), 8u, &prefix_bits) ||
        ssz_internal_add_overflow_u64(prefix_bits, delimiter_pos, &prefix_bits))
    {
        return SSZ_ERR_OVERFLOW;
    }

    if ((bit_limit != SSZ_NO_LIMIT) && (prefix_bits > bit_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    size_t out_bytes = 0u;
    if (!ssz_internal_bits_to_bytes(prefix_bits, &out_bytes))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((out_bytes != 0u) && ((out_bits_le == NULL) || (out_bits_le_len < out_bytes)))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (out_bytes != 0u)
    {
        if (delimiter_pos == 0u)
        {
            memcpy(out_bits_le, in, in_len - 1u);
        }
        else
        {
            memcpy(out_bits_le, in, in_len);
            out_bits_le[out_bytes - 1u] &= (uint8_t)((1u << delimiter_pos) - 1u);
        }
    }

    *out_bit_len = prefix_bits;
    return SSZ_SUCCESS;
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

    if (element_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (element_size == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &required))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if ((in == NULL) || (in_len != required))
    {
        return SSZ_ERR_ENCODING_INVALID;
    }
    if ((required != 0u) && ((out_elements == NULL) || (out_elements_len < required)))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (required != 0u)
    {
        memcpy(out_elements, in, required);
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_vector_variable(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t min_element_size,
    ssz_member_codec_t *codec)
{
    if (element_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    return ssz_internal_deserialize_variable_sequence(
        in,
        in_len,
        element_count,
        min_element_size,
        codec);
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
    if (out_element_count == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (element_size == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((in_len % element_size) != 0u)
    {
        return SSZ_ERR_ENCODING_INVALID;
    }

    size_t count = in_len / element_size;
    if ((element_limit != SSZ_NO_LIMIT) && ((uint64_t)count > element_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if ((in == NULL) && (in_len != 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((in_len != 0u) && ((out_elements == NULL) || (out_elements_len < in_len)))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (in_len != 0u)
    {
        memcpy(out_elements, in, in_len);
    }

    *out_element_count = (uint64_t)count;
    return SSZ_SUCCESS;
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

    if (out_element_count == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (in_len == 0u)
    {
        *out_element_count = 0u;
        return SSZ_SUCCESS;
    }
    if ((in == NULL) || (in_len < SSZ_BYTES_PER_LENGTH_OFFSET))
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    uint32_t first_offset = ssz_internal_read_u32_le(in);
    if (((size_t)first_offset > in_len) || ((first_offset % SSZ_BYTES_PER_LENGTH_OFFSET) != 0u))
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    element_count = (uint64_t)(first_offset / SSZ_BYTES_PER_LENGTH_OFFSET);
    if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }
    if (element_count == 0u)
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    ssz_error_t err = ssz_internal_deserialize_variable_sequence(
        in,
        in_len,
        element_count,
        min_element_size,
        codec);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    *out_element_count = element_count;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_container(
    const uint8_t *in,
    size_t in_len,
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec)
{
    size_t fixed_region = 0u;

    if ((field_fixed_sizes == NULL) || (field_count == 0u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((codec == NULL) || (codec->read == NULL) || ((in == NULL) && (in_len != 0u)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < field_count; i++)
    {
        size_t fixed_size = field_fixed_sizes[i];
        size_t contribution = (fixed_size == 0u) ? SSZ_BYTES_PER_LENGTH_OFFSET : fixed_size;
        if (ssz_internal_add_overflow_size(fixed_region, contribution, &fixed_region))
        {
            return SSZ_ERR_OVERFLOW;
        }
    }
    if (fixed_region > in_len)
    {
        return SSZ_ERR_OFFSET_INVALID;
    }

    size_t cursor = 0u;
    bool saw_variable = false;
    uint32_t prev_offset = 0u;

    for (uint32_t i = 0u; i < field_count; i++)
    {
        size_t fixed_size = field_fixed_sizes[i];

        if (fixed_size == 0u)
        {
            uint32_t offset = ssz_internal_read_u32_le(in + cursor);
            if ((size_t)offset > in_len)
            {
                return SSZ_ERR_OFFSET_INVALID;
            }
            if (!saw_variable)
            {
                if ((size_t)offset != fixed_region)
                {
                    return SSZ_ERR_OFFSET_INVALID;
                }
                saw_variable = true;
            }
            else if (offset < prev_offset)
            {
                return SSZ_ERR_OFFSET_INVALID;
            }

            prev_offset = offset;
            cursor += SSZ_BYTES_PER_LENGTH_OFFSET;
        }
        else
        {
            if (cursor + fixed_size > fixed_region)
            {
                return SSZ_ERR_OFFSET_INVALID;
            }
            ssz_error_t err = codec->read(codec->ctx, i, in + cursor, fixed_size);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
            cursor += fixed_size;
        }
    }

    if (cursor != fixed_region)
    {
        return SSZ_ERR_OFFSET_INVALID;
    }
    if (!saw_variable)
    {
        return (fixed_region == in_len) ? SSZ_SUCCESS : SSZ_ERR_OFFSET_INVALID;
    }

    cursor = 0u;
    for (uint32_t i = 0u; i < field_count; i++)
    {
        size_t fixed_size = field_fixed_sizes[i];
        if (fixed_size != 0u)
        {
            cursor += fixed_size;
            continue;
        }

        size_t start = (size_t)ssz_internal_read_u32_le(in + cursor);
        size_t end = in_len;

        size_t look_cursor = cursor + SSZ_BYTES_PER_LENGTH_OFFSET;
        for (uint32_t j = i + 1u; j < field_count; j++)
        {
            if (field_fixed_sizes[j] == 0u)
            {
                end = (size_t)ssz_internal_read_u32_le(in + look_cursor);
                break;
            }
            look_cursor += field_fixed_sizes[j];
        }

        if (end < start)
        {
            return SSZ_ERR_OFFSET_INVALID;
        }

        ssz_error_t err = codec->read(codec->ctx, i, in + start, end - start);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }

        cursor += SSZ_BYTES_PER_LENGTH_OFFSET;
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_union(
    const uint8_t *in,
    size_t in_len,
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    if (out_selector == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (option_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (option_count > 256u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if (has_none && (option_count < 2u))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }
    if ((in == NULL) || (in_len < 1u))
    {
        return SSZ_ERR_ENCODING_INVALID;
    }

    uint8_t selector = in[0];
    if ((uint32_t)selector >= option_count)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    if (has_none && (selector == 0u))
    {
        if (in_len != 1u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_selector = selector;
        return SSZ_SUCCESS;
    }

    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = codec->read(codec->ctx, selector, in + 1u, in_len - 1u);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    *out_selector = selector;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_deserialize_compatible_union(
    const uint8_t *in,
    size_t in_len,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count,
    ssz_member_codec_t *codec,
    uint8_t *out_selector)
{
    if (out_selector == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t schema_err =
        ssz_internal_validate_compatible_union_schema(allowed_selectors, allowed_selector_count);
    if (schema_err != SSZ_SUCCESS)
    {
        return schema_err;
    }
    if ((in == NULL) || (in_len < 1u))
    {
        return SSZ_ERR_ENCODING_INVALID;
    }

    uint8_t selector = in[0];
    if ((selector == 0u) || (selector > 127u) ||
        !ssz_internal_selector_allowed(selector, allowed_selectors, allowed_selector_count))
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }
    if ((codec == NULL) || (codec->read == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = codec->read(codec->ctx, selector, in + 1u, in_len - 1u);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    *out_selector = selector;
    return SSZ_SUCCESS;
}
