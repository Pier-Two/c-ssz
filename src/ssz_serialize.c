#include "ssz_serialize.h"

#include <string.h>

#include "ssz_internal.h"

static ssz_error_t ssz_internal_prepare_output(
    size_t required,
    const uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_len == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_len = required;
        if ((out != NULL) && (out_cap < required))
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
    }

    return err;
}

ssz_error_t ssz_serialize_uint8(uint8_t value, uint8_t out[1])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        out[0] = value;
    }

    return err;
}

ssz_error_t ssz_serialize_uint16(uint16_t value, uint8_t out[2])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_write_u16_le(out, value);
    }

    return err;
}

ssz_error_t ssz_serialize_uint32(uint32_t value, uint8_t out[4])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_write_u32_le(out, value);
    }

    return err;
}

ssz_error_t ssz_serialize_uint64(uint64_t value, uint8_t out[8])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ssz_internal_write_u64_le(out, value);
    }

    return err;
}

ssz_error_t ssz_serialize_uint128(const uint8_t *value, size_t value_len, uint8_t out[16])
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (value_len != 16u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        (void)memcpy(out, value, 16u);
    }

    return err;
}

ssz_error_t ssz_serialize_uint256(const uint8_t *value, size_t value_len, uint8_t out[32])
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (value_len != 32u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        (void)memcpy(out, value, 32u);
    }

    return err;
}

ssz_error_t ssz_serialize_boolean(uint8_t value, uint8_t out[1])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (value > 1u)
    {
        err = SSZ_ERR_ENCODING_INVALID;
    }
    else
    {
        out[0] = value;
    }

    return err;
}

ssz_error_t ssz_serialize_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t byte_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (bit_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_internal_bits_to_bytes(bit_count, &byte_count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((bits_le == NULL) || (bits_le_len < byte_count))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ((bit_count % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
            if ((bits_le[byte_count - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_prepare_output(byte_count, out, out_cap, out_len);
            if ((err == SSZ_SUCCESS) && (out != NULL))
            {
                (void)memcpy(out, bits_le, byte_count);
            }
        }
    }

    return err;
}

ssz_error_t ssz_serialize_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t data_bytes = 0u;
    size_t delimiter_byte = 0u;
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((bit_limit != SSZ_NO_LIMIT) && (bit_len > bit_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_bits_to_bytes(bit_len, &data_bytes))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (
        !ssz_internal_u64_to_size(bit_len / 8u, &delimiter_byte) ||
        ssz_internal_add_overflow_size(delimiter_byte, 1u, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        if (data_bytes != 0u)
        {
            if ((bits_le == NULL) || (bits_le_len < data_bytes))
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else if ((bit_len % 8u) != 0u)
            {
                uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
                if ((bits_le[data_bytes - 1u] & (uint8_t)(~mask)) != 0u)
                {
                    err = SSZ_ERR_ENCODING_INVALID;
                }
            }
            else
            {
                /* intentionally empty */
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_prepare_output(required, out, out_cap, out_len);
            if ((err == SSZ_SUCCESS) && (out != NULL))
            {
                uint8_t delimiter_bit = (uint8_t)(1u << (bit_len % 8u));

                (void)memset(out, 0, required);
                if (data_bytes != 0u)
                {
                    (void)memcpy(out, bits_le, data_bytes);
                }

                out[delimiter_byte] = (uint8_t)(out[delimiter_byte] | delimiter_bit);
            }
        }
    }

    return err;
}

ssz_error_t ssz_serialize_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
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
    else if (required > UINT32_MAX)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((required != 0u) && (elements == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_prepare_output(required, out, out_cap, out_len);
        if ((err == SSZ_SUCCESS) && (out != NULL) && (required != 0u))
        {
            (void)memcpy(out, elements, required);
        }
    }

    return err;
}

ssz_error_t ssz_serialize_vector_variable(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t fixed_region = 0u;
    size_t total = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size(
                 (size_t)element_count,
                 SSZ_BYTES_PER_LENGTH_OFFSET,
                 &fixed_region))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (fixed_region > UINT32_MAX)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        total = fixed_region;

        for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t encoded_len = 0u;

            err = codec->write(codec->ctx, i, NULL, 0u, &encoded_len);
            if ((err == SSZ_SUCCESS) && ssz_internal_add_overflow_size(total, encoded_len, &total))
            {
                err = SSZ_ERR_OVERFLOW;
            }
        }
    }
    if ((err == SSZ_SUCCESS) && (total > UINT32_MAX))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_internal_prepare_output(total, out, out_cap, out_len);
    }
    if ((err == SSZ_SUCCESS) && (out != NULL))
    {
        size_t cursor = fixed_region;

        for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
        {
            if (cursor > UINT32_MAX)
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                size_t expected_len = 0u;
                size_t written = 0u;

                err = codec->write(codec->ctx, i, NULL, 0u, &expected_len);
                if (err == SSZ_SUCCESS)
                {
                    ssz_internal_write_u32_le(
                        &out[(size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET],
                        (uint32_t)cursor);
                    err = codec->write(codec->ctx, i, &out[cursor], out_cap - cursor, &written);
                    if ((err == SSZ_SUCCESS) && (written != expected_len))
                    {
                        err = SSZ_ERR_TYPE_MISMATCH;
                    }
                    if ((err == SSZ_SUCCESS) &&
                        ssz_internal_add_overflow_size(cursor, written, &cursor))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                }
            }
        }

        if ((err == SSZ_SUCCESS) && (cursor != total))
        {
            err = SSZ_ERR_TYPE_MISMATCH;
        }
    }

    return err;
}

ssz_error_t ssz_serialize_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size((size_t)element_count, element_size, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (required > UINT32_MAX)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((required != 0u) && (elements == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_internal_prepare_output(required, out, out_cap, out_len);
        if ((err == SSZ_SUCCESS) && (out != NULL) && (required != 0u))
        {
            (void)memcpy(out, elements, required);
        }
    }

    return err;
}

ssz_error_t ssz_serialize_list_variable(
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t fixed_region = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((element_limit != SSZ_NO_LIMIT) && (element_count > element_limit))
    {
        err = SSZ_ERR_LIMIT_EXCEEDED;
    }
    else if ((codec == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_internal_mul_overflow_size(
                 (size_t)element_count,
                 SSZ_BYTES_PER_LENGTH_OFFSET,
                 &fixed_region))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (fixed_region > UINT32_MAX)
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (out != NULL)
    {
        if (out_len == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (out_cap < fixed_region)
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else
        {
            size_t cursor = fixed_region;

            for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
            {
                if (cursor > UINT32_MAX)
                {
                    err = SSZ_ERR_OVERFLOW;
                }
                else
                {
                    ssz_internal_write_u32_le(
                        &out[(size_t)i * SSZ_BYTES_PER_LENGTH_OFFSET],
                        (uint32_t)cursor);

                    if (cursor > out_cap)
                    {
                        err = SSZ_ERR_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        size_t written = 0u;

                        err = codec->write(codec->ctx, i, &out[cursor], out_cap - cursor, &written);
                        if ((err == SSZ_SUCCESS) &&
                            ssz_internal_add_overflow_size(cursor, written, &cursor))
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                    }
                }
            }

            if ((err == SSZ_SUCCESS) && (cursor > UINT32_MAX))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            if (err == SSZ_SUCCESS)
            {
                *out_len = cursor;
            }
        }
    }
    else
    {
        size_t total = fixed_region;

        for (uint64_t i = 0u; (i < element_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t encoded_len = 0u;

            err = codec->write(codec->ctx, i, NULL, 0u, &encoded_len);
            if ((err == SSZ_SUCCESS) && ssz_internal_add_overflow_size(total, encoded_len, &total))
            {
                err = SSZ_ERR_OVERFLOW;
            }
        }
        if ((err == SSZ_SUCCESS) && (total > UINT32_MAX))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_prepare_output(total, out, out_cap, out_len);
        }
    }

    return err;
}

ssz_error_t ssz_serialize_container(
    const ssz_container_schema_t *schema,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    const size_t *field_fixed_sizes = NULL;
    uint32_t field_count = 0u;
    size_t fixed_region = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((schema == NULL) || (schema->field_fixed_sizes == NULL) || (schema->field_count == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        field_fixed_sizes = schema->field_fixed_sizes;
        field_count = schema->field_count;
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
    }
    if ((err == SSZ_SUCCESS) && (out != NULL))
    {
        if (out_len == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else if (fixed_region > UINT32_MAX)
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else if (out_cap < fixed_region)
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else
        {
            size_t fixed_cursor = 0u;
            size_t variable_cursor = fixed_region;

            for (uint32_t i = 0u; (i < field_count) && (err == SSZ_SUCCESS); i++)
            {
                size_t fixed_size = field_fixed_sizes[i];
                size_t written = 0u;

                if (fixed_size == 0u)
                {
                    size_t next_fixed_cursor = 0u;

                    if (ssz_internal_add_overflow_size(
                            fixed_cursor, SSZ_BYTES_PER_LENGTH_OFFSET, &next_fixed_cursor))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                    else if ((next_fixed_cursor > fixed_region) || (next_fixed_cursor > out_cap))
                    {
                        err = SSZ_ERR_TYPE_MISMATCH;
                    }
                    else if (variable_cursor > UINT32_MAX)
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                    else if (variable_cursor > out_cap)
                    {
                        err = SSZ_ERR_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        ssz_internal_write_u32_le(&out[fixed_cursor], (uint32_t)variable_cursor);
                        fixed_cursor = next_fixed_cursor;

                        err =
                            codec->write(codec->ctx, i, &out[variable_cursor], out_cap - variable_cursor, &written);
                        if ((err == SSZ_SUCCESS) &&
                            ssz_internal_add_overflow_size(variable_cursor, written, &variable_cursor))
                        {
                            err = SSZ_ERR_OVERFLOW;
                        }
                    }
                }
                else
                {
                    size_t next_fixed_cursor = 0u;

                    if (ssz_internal_add_overflow_size(fixed_cursor, fixed_size, &next_fixed_cursor))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                    else if ((next_fixed_cursor > fixed_region) || (next_fixed_cursor > out_cap))
                    {
                        err = SSZ_ERR_TYPE_MISMATCH;
                    }
                    else
                    {
                        err = codec->write(codec->ctx, i, &out[fixed_cursor], fixed_size, &written);
                        if ((err == SSZ_SUCCESS) && (written != fixed_size))
                        {
                            err = SSZ_ERR_TYPE_MISMATCH;
                        }
                        else if (err == SSZ_SUCCESS)
                        {
                            fixed_cursor = next_fixed_cursor;
                        }
                        else
                        {
                            /* intentionally empty */
                        }
                    }
                }
            }

            if ((err == SSZ_SUCCESS) && (fixed_cursor != fixed_region))
            {
                err = SSZ_ERR_TYPE_MISMATCH;
            }
            if ((err == SSZ_SUCCESS) && (variable_cursor > UINT32_MAX))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            if (err == SSZ_SUCCESS)
            {
                *out_len = variable_cursor;
            }
        }
    }
    if ((err == SSZ_SUCCESS) && (out == NULL))
    {
        size_t total = fixed_region;

        for (uint32_t i = 0u; (i < field_count) && (err == SSZ_SUCCESS); i++)
        {
            size_t encoded_len = 0u;

            err = codec->write(codec->ctx, i, NULL, 0u, &encoded_len);
            if (err == SSZ_SUCCESS)
            {
                if (field_fixed_sizes[i] == 0u)
                {
                    if (ssz_internal_add_overflow_size(total, encoded_len, &total))
                    {
                        err = SSZ_ERR_OVERFLOW;
                    }
                }
                else if (encoded_len != field_fixed_sizes[i])
                {
                    err = SSZ_ERR_TYPE_MISMATCH;
                }
                else
                {
                    /* intentionally empty */
                }
            }
        }
        if ((err == SSZ_SUCCESS) && (total > UINT32_MAX))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_prepare_output(total, out, out_cap, out_len);
        }
    }

    return err;
}

ssz_error_t ssz_serialize_union(
    uint8_t selector,
    uint32_t option_count,
    bool has_none,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t payload_len = 0u;
    size_t total = 1u;
    ssz_error_t err = SSZ_SUCCESS;

    if (option_count == 0u)
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
    else if ((uint32_t)selector >= option_count)
    {
        err = SSZ_ERR_SELECTOR_INVALID;
    }
    else if (!(has_none && (selector == 0u)))
    {
        if ((codec == NULL) || (codec->write == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = codec->write(codec->ctx, selector, NULL, 0u, &payload_len);
            if (err == SSZ_SUCCESS)
            {
                if (ssz_internal_add_overflow_size(total, payload_len, &total))
                {
                    err = SSZ_ERR_OVERFLOW;
                }
            }
        }
    }
    else
    {
        /* intentionally empty */
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_internal_prepare_output(total, out, out_cap, out_len);
    }
    if ((err == SSZ_SUCCESS) && (out != NULL))
    {
        out[0] = selector;
        if (total != 1u)
        {
            size_t written = 0u;

            err = codec->write(codec->ctx, selector, &out[1u], out_cap - 1u, &written);
            if ((err == SSZ_SUCCESS) && (written != payload_len))
            {
                err = SSZ_ERR_TYPE_MISMATCH;
            }
        }
    }

    return err;
}

ssz_error_t ssz_serialize_compatible_union(
    uint8_t selector,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    size_t payload_len = 0u;
    size_t total = 1u;
    ssz_error_t err =
        ssz_internal_validate_compatible_union_schema(allowed_selectors, allowed_selector_count);

    if (err != SSZ_SUCCESS)
    {
        /* schema validation error already captured */
    }
    else if (
        (selector == 0u) || (selector > 127u) ||
        !ssz_internal_selector_allowed(selector, allowed_selectors, allowed_selector_count))
    {
        err = SSZ_ERR_SELECTOR_INVALID;
    }
    else if ((codec == NULL) || (codec->write == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = codec->write(codec->ctx, selector, NULL, 0u, &payload_len);
        if (err == SSZ_SUCCESS)
        {
            if (ssz_internal_add_overflow_size(total, payload_len, &total))
            {
                err = SSZ_ERR_OVERFLOW;
            }
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_internal_prepare_output(total, out, out_cap, out_len);
        }
        if ((err == SSZ_SUCCESS) && (out != NULL))
        {
            size_t written = 0u;

            out[0] = selector;
            err = codec->write(codec->ctx, selector, &out[1u], out_cap - 1u, &written);
            if ((err == SSZ_SUCCESS) && (written != payload_len))
            {
                err = SSZ_ERR_TYPE_MISMATCH;
            }
        }
    }

    return err;
}
