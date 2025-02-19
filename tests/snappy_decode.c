#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  SNAPPY_OK = 0,
  SNAPPY_INVALID_INPUT = 1,
  SNAPPY_BUFFER_TOO_SMALL = 2
} snappy_status;

static snappy_status decode_varint(const char *data, size_t n, uint32_t *value, size_t *varint_len)
{
  uint32_t result = 0;
  size_t i = 0;
  int shift = 0;
  for (; i < n && i < 5; i++)
  {
    uint8_t byte = (uint8_t)data[i];
    result |= (uint32_t)(byte & 0x7F) << shift;
    if (!(byte & 0x80))
    {
      *value = result;
      *varint_len = i + 1;
      return SNAPPY_OK;
    }
    shift += 7;
  }
  return SNAPPY_INVALID_INPUT;
}

snappy_status snappy_uncompressed_length(const char *compressed,
                                         size_t compressed_length,
                                         size_t *result)
{
  if (compressed == NULL || result == NULL || compressed_length == 0)
    return SNAPPY_INVALID_INPUT;

  uint32_t len;
  size_t varint_len;
  snappy_status status = decode_varint(compressed, compressed_length, &len, &varint_len);
  if (status != SNAPPY_OK)
    return SNAPPY_INVALID_INPUT;

  *result = (size_t)len;
  return SNAPPY_OK;
}

snappy_status snappy_uncompress(const char *compressed,
                                size_t compressed_length,
                                char *uncompressed,
                                size_t *uncompressed_length)
{
  if (compressed == NULL || uncompressed == NULL || uncompressed_length == NULL)
    return SNAPPY_INVALID_INPUT;

  uint32_t expected_len32;
  size_t varint_len;
  snappy_status status = decode_varint(compressed, compressed_length, &expected_len32, &varint_len);
  if (status != SNAPPY_OK)
    return SNAPPY_INVALID_INPUT;
  size_t expected_length = (size_t)expected_len32;

  if (*uncompressed_length < expected_length)
  {
    *uncompressed_length = expected_length;
    return SNAPPY_BUFFER_TOO_SMALL;
  }

  const uint8_t *in = (const uint8_t *)compressed;
  size_t in_pos = varint_len;
  uint8_t *out = (uint8_t *)uncompressed;
  size_t out_pos = 0;

  while (in_pos < compressed_length && out_pos < expected_length)
  {
    uint8_t tag = in[in_pos++];
    switch (tag & 0x03)
    {

    case 0:
    {
      uint32_t literal_length;
      uint8_t len_val = tag >> 2;
      if (len_val < 60)
      {
        literal_length = len_val + 1;
      }
      else
      {
        int extra_bytes = len_val - 59;
        if (in_pos + extra_bytes > compressed_length)
          return SNAPPY_INVALID_INPUT;
        literal_length = 0;
        for (int i = 0; i < extra_bytes; i++)
        {
          literal_length |= ((uint32_t)in[in_pos + i]) << (8 * i);
        }
        literal_length += 1;
        in_pos += extra_bytes;
      }

      if (in_pos + literal_length > compressed_length ||
          out_pos + literal_length > expected_length)
        return SNAPPY_INVALID_INPUT;

      memcpy(out + out_pos, in + in_pos, literal_length);
      out_pos += literal_length;
      in_pos += literal_length;
      break;
    }

    case 1:
    {
      if (in_pos >= compressed_length)
        return SNAPPY_INVALID_INPUT;
      int length = ((tag >> 2) & 0x07) + 4;
      int offset = ((tag >> 5) << 8) | in[in_pos++];
      if (offset == 0 || offset > (int)out_pos)
        return SNAPPY_INVALID_INPUT;
      if (out_pos + length > expected_length)
        return SNAPPY_INVALID_INPUT;
      if (length <= offset)
      {
        memcpy(out + out_pos, out + out_pos - offset, length);
      }
      else
      {
        for (int i = 0; i < length; i++)
        {
          out[out_pos + i] = out[out_pos - offset + (i % offset)];
        }
      }
      out_pos += length;
      break;
    }

    case 2:
    {
      if (in_pos + 1 >= compressed_length)
        return SNAPPY_INVALID_INPUT;
      int length = (tag >> 2) + 1;
      int offset = in[in_pos] | (in[in_pos + 1] << 8);
      in_pos += 2;
      if (offset == 0 || offset > (int)out_pos)
        return SNAPPY_INVALID_INPUT;
      if (out_pos + length > expected_length)
        return SNAPPY_INVALID_INPUT;
      if (length <= offset)
      {
        memcpy(out + out_pos, out + out_pos - offset, length);
      }
      else
      {
        for (int i = 0; i < length; i++)
        {
          out[out_pos + i] = out[out_pos - offset + (i % offset)];
        }
      }
      out_pos += length;
      break;
    }

    case 3:
    {
      if (in_pos + 3 >= compressed_length)
        return SNAPPY_INVALID_INPUT;
      int length = (tag >> 2) + 1;
      int offset = in[in_pos] | (in[in_pos + 1] << 8) |
                   (in[in_pos + 2] << 16) | (in[in_pos + 3] << 24);
      in_pos += 4;
      if (offset == 0 || offset > (int)out_pos)
        return SNAPPY_INVALID_INPUT;
      if (out_pos + length > expected_length)
        return SNAPPY_INVALID_INPUT;
      if (length <= offset)
      {
        memcpy(out + out_pos, out + out_pos - offset, length);
      }
      else
      {
        for (int i = 0; i < length; i++)
        {
          out[out_pos + i] = out[out_pos - offset + (i % offset)];
        }
      }
      out_pos += length;
      break;
    }
    }
  }

  if (out_pos != expected_length)
    return SNAPPY_INVALID_INPUT;

  if (in_pos != compressed_length)
    return SNAPPY_INVALID_INPUT;

  *uncompressed_length = expected_length;
  return SNAPPY_OK;
}