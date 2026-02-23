#include "snappy_decode.h"

#include <stdint.h>
#include <string.h>

static snappy_status decode_varint(
    const char *data,
    size_t data_len,
    uint32_t *out_value,
    size_t *out_len)
{
    uint32_t value = 0u;
    int shift = 0;

    if ((data == NULL) || (out_value == NULL) || (out_len == NULL))
    {
        return SNAPPY_INVALID_INPUT;
    }

    for (size_t i = 0u; i < data_len && i < 5u; i++)
    {
        uint8_t byte = (uint8_t)data[i];
        value |= (uint32_t)(byte & 0x7Fu) << shift;

        if ((byte & 0x80u) == 0u)
        {
            *out_value = value;
            *out_len = i + 1u;
            return SNAPPY_OK;
        }

        shift += 7;
    }

    return SNAPPY_INVALID_INPUT;
}

snappy_status snappy_uncompressed_length(
    const char *compressed,
    size_t compressed_length,
    size_t *result)
{
    uint32_t length = 0u;
    size_t varint_len = 0u;
    snappy_status status;

    if ((compressed == NULL) || (result == NULL) || (compressed_length == 0u))
    {
        return SNAPPY_INVALID_INPUT;
    }

    status = decode_varint(compressed, compressed_length, &length, &varint_len);
    if (status != SNAPPY_OK)
    {
        return SNAPPY_INVALID_INPUT;
    }

    (void)varint_len;
    *result = (size_t)length;
    return SNAPPY_OK;
}

static snappy_status copy_match(
    uint8_t *out,
    size_t out_pos,
    size_t out_len,
    size_t offset,
    size_t match_len)
{
    if ((offset == 0u) || (offset > out_pos))
    {
        return SNAPPY_INVALID_INPUT;
    }
    if (match_len > (out_len - out_pos))
    {
        return SNAPPY_INVALID_INPUT;
    }

    if (match_len <= offset)
    {
        memcpy(out + out_pos, out + out_pos - offset, match_len);
    }
    else
    {
        for (size_t i = 0u; i < match_len; i++)
        {
            out[out_pos + i] = out[out_pos - offset + (i % offset)];
        }
    }

    return SNAPPY_OK;
}

snappy_status snappy_uncompress(
    const char *compressed,
    size_t compressed_length,
    char *uncompressed,
    size_t *uncompressed_length)
{
    uint32_t expected_u32 = 0u;
    size_t header_len = 0u;
    size_t expected_len;
    const uint8_t *in;
    uint8_t *out;
    size_t in_pos;
    size_t out_pos;
    snappy_status status;

    if ((compressed == NULL) || (uncompressed == NULL) || (uncompressed_length == NULL))
    {
        return SNAPPY_INVALID_INPUT;
    }

    status = decode_varint(compressed, compressed_length, &expected_u32, &header_len);
    if (status != SNAPPY_OK)
    {
        return SNAPPY_INVALID_INPUT;
    }

    expected_len = (size_t)expected_u32;
    if (*uncompressed_length < expected_len)
    {
        *uncompressed_length = expected_len;
        return SNAPPY_BUFFER_TOO_SMALL;
    }

    in = (const uint8_t *)compressed;
    out = (uint8_t *)uncompressed;
    in_pos = header_len;
    out_pos = 0u;

    while ((in_pos < compressed_length) && (out_pos < expected_len))
    {
        uint8_t tag = in[in_pos++];
        uint8_t tag_type = (uint8_t)(tag & 0x03u);

        if (tag_type == 0u)
        {
            uint32_t literal_len = 0u;
            uint8_t len_code = (uint8_t)(tag >> 2u);

            if (len_code < 60u)
            {
                literal_len = (uint32_t)len_code + 1u;
            }
            else
            {
                size_t extra_bytes = (size_t)len_code - 59u;
                literal_len = 0u;

                if ((extra_bytes > 4u) || (in_pos + extra_bytes > compressed_length))
                {
                    return SNAPPY_INVALID_INPUT;
                }

                for (size_t i = 0u; i < extra_bytes; i++)
                {
                    literal_len |= (uint32_t)in[in_pos + i] << (8u * i);
                }
                literal_len += 1u;
                in_pos += extra_bytes;
            }

            if ((size_t)literal_len > (compressed_length - in_pos))
            {
                return SNAPPY_INVALID_INPUT;
            }
            if ((size_t)literal_len > (expected_len - out_pos))
            {
                return SNAPPY_INVALID_INPUT;
            }

            memcpy(out + out_pos, in + in_pos, literal_len);
            out_pos += (size_t)literal_len;
            in_pos += (size_t)literal_len;
            continue;
        }

        if (tag_type == 1u)
        {
            size_t match_len = (size_t)((tag >> 2u) & 0x07u) + 4u;
            size_t offset;

            if (in_pos >= compressed_length)
            {
                return SNAPPY_INVALID_INPUT;
            }

            offset = (size_t)((tag >> 5u) << 8u) | (size_t)in[in_pos++];
            status = copy_match(out, out_pos, expected_len, offset, match_len);
            if (status != SNAPPY_OK)
            {
                return status;
            }
            out_pos += match_len;
            continue;
        }

        if (tag_type == 2u)
        {
            size_t match_len = (size_t)(tag >> 2u) + 1u;
            size_t offset;

            if (in_pos + 1u >= compressed_length)
            {
                return SNAPPY_INVALID_INPUT;
            }

            offset = (size_t)in[in_pos] | ((size_t)in[in_pos + 1u] << 8u);
            in_pos += 2u;

            status = copy_match(out, out_pos, expected_len, offset, match_len);
            if (status != SNAPPY_OK)
            {
                return status;
            }
            out_pos += match_len;
            continue;
        }

        {
            size_t match_len = (size_t)(tag >> 2u) + 1u;
            size_t offset;

            if (in_pos + 3u >= compressed_length)
            {
                return SNAPPY_INVALID_INPUT;
            }

            offset = (size_t)in[in_pos] | ((size_t)in[in_pos + 1u] << 8u) |
                ((size_t)in[in_pos + 2u] << 16u) | ((size_t)in[in_pos + 3u] << 24u);
            in_pos += 4u;

            status = copy_match(out, out_pos, expected_len, offset, match_len);
            if (status != SNAPPY_OK)
            {
                return status;
            }
            out_pos += match_len;
        }
    }

    if ((out_pos != expected_len) || (in_pos != compressed_length))
    {
        return SNAPPY_INVALID_INPUT;
    }

    *uncompressed_length = expected_len;
    return SNAPPY_OK;
}
