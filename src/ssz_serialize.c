#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ssz_serialize.h"
#include "ssz_constants.h"
#include "ssz_types.h"
#include "ssz_utils.h"

/**
 * Serializes an 8-bit unsigned integer into a single byte.
 *
 * @param value Pointer to the 8-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint8(const void *value, uint8_t *out_buf, size_t *out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 1)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    out_buf[0] = *(const uint8_t *)value;
    *out_size = 1;
    return SSZ_SUCCESS;
}

/**
 * Serializes a 16-bit unsigned integer into two bytes.
 *
 * @param value Pointer to the 16-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint16(const void *value, uint8_t *out_buf, size_t *out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 2)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    uint16_t val;
    memcpy(&val, value, sizeof(val));
    out_buf[0] = (uint8_t)(val);
    out_buf[1] = (uint8_t)(val >> 8);
    *out_size = 2;
    return SSZ_SUCCESS;
}

/**
 * Serializes a 32-bit unsigned integer into four bytes.
 *
 * @param value Pointer to the 32-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint32(const void *value, uint8_t *out_buf, size_t *out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 4)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    uint32_t val;
    memcpy(&val, value, sizeof(val));
    out_buf[0] = (uint8_t)(val);
    out_buf[1] = (uint8_t)(val >> 8);
    out_buf[2] = (uint8_t)(val >> 16);
    out_buf[3] = (uint8_t)(val >> 24);
    *out_size = 4;
    return SSZ_SUCCESS;
}

/**
 * Serializes a 64-bit unsigned integer into eight bytes.
 *
 * @param value Pointer to the 64-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint64(const void *value, uint8_t *out_buf, size_t *out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 8)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    uint64_t val = *(const uint64_t *)value;
    out_buf[0] = (uint8_t)(val);
    out_buf[1] = (uint8_t)(val >> 8);
    out_buf[2] = (uint8_t)(val >> 16);
    out_buf[3] = (uint8_t)(val >> 24);
    out_buf[4] = (uint8_t)(val >> 32);
    out_buf[5] = (uint8_t)(val >> 40);
    out_buf[6] = (uint8_t)(val >> 48);
    out_buf[7] = (uint8_t)(val >> 56);
    *out_size = 8;
    return SSZ_SUCCESS;
}

/**
 * Serializes a 128-bit unsigned integer into sixteen bytes.
 *
 * @param value Pointer to the 128-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint128(
    const void *restrict value,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 16)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t *endian_check = (const uint8_t *)&test_value;
    if (endian_check[0] == 0x01)
    {
        memcpy(out_buf, value, 16);
    }
    else
    {
        const uint8_t *src = (const uint8_t *)value;
        for (size_t i = 0; i < 16; i++)
        {
            out_buf[i] = src[15 - i];
        }
    }
    *out_size = 16;
    return SSZ_SUCCESS;
}

/**
 * Serializes a 256-bit unsigned integer into thirty-two bytes.
 *
 * @param value Pointer to the 256-bit unsigned integer to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint256(
    const void *restrict value,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (value == NULL || out_buf == NULL || out_size == NULL || *out_size < 32)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, value, 32);
    }
    else
    {
        const uint8_t *src = (const uint8_t *)value;
        for (size_t i = 0; i < 32; i++)
        {
            out_buf[i] = src[31 - i];
        }
    }
    *out_size = 32;
    return SSZ_SUCCESS;
}

/**
 * Serializes a boolean value into a single byte.
 *
 * @param value The boolean value to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_boolean(bool value, uint8_t *out_buf, size_t *out_size)
{
    if (out_buf == NULL || out_size == NULL || *out_size < 1)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    out_buf[0] = (uint8_t)value;
    *out_size = 1;
    return SSZ_SUCCESS;
}

/**
 * Serializes a bitvector into a compact byte array. Each bit in the input
 * is packed into the output buffer, with unused bits in the last byte set to 0.
 *
 * @param bits Pointer to the input bit array.
 * @param num_bits The number of bits to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_bitvector(const bool *restrict bits, size_t num_bits, uint8_t *restrict out_buf, size_t *restrict out_size)
{
    if (bits == NULL || out_buf == NULL || out_size == NULL || num_bits == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    const size_t byte_count = (num_bits + 7) / 8;
    if (*out_size < byte_count)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    const size_t full_bytes = num_bits / 8;
    const size_t remainder_bits = num_bits % 8;
    const bool *bit_ptr = bits;
    uint8_t *out_ptr = out_buf;
    for (size_t i = 0; i < full_bytes; ++i)
    {
        *out_ptr++ = (uint8_t)((bit_ptr[0] << 0) | (bit_ptr[1] << 1) |
                               (bit_ptr[2] << 2) | (bit_ptr[3] << 3) |
                               (bit_ptr[4] << 4) | (bit_ptr[5] << 5) |
                               (bit_ptr[6] << 6) | (bit_ptr[7] << 7));
        bit_ptr += 8;
    }
    if (remainder_bits > 0)
    {
        uint8_t value = 0;
        for (size_t bit = 0; bit < remainder_bits; ++bit)
        {
            value |= (*bit_ptr++) << bit;
        }
        *out_ptr = value;
    }
    *out_size = byte_count;
    return SSZ_SUCCESS;
}

/**
 * Serializes a bitlist into a compact byte array. A bitlist is similar to a bitvector,
 * but includes an additional "end bit" to indicate the end of the list.
 *
 * @param bits Pointer to the input bit array.
 * @param num_bits The number of bits to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_bitlist(const bool *bits, size_t num_bits, uint8_t *out_buf, size_t *out_size)
{
    if (bits == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    const size_t total_bits = num_bits + 1;
    const size_t byte_count = (total_bits + 7) / 8;
    if (*out_size < byte_count)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    const size_t delimiter_byte = num_bits / 8;
    const size_t delimiter_bit = num_bits % 8;
    const bool *bit_ptr = bits;
    for (size_t j = 0; j < byte_count; j++)
    {
        uint8_t byte = 0;
        const size_t start = j * 8;
        if (start + 8 <= num_bits)
        {
            byte |= (uint8_t)(*bit_ptr++) << 0;
            byte |= (uint8_t)(*bit_ptr++) << 1;
            byte |= (uint8_t)(*bit_ptr++) << 2;
            byte |= (uint8_t)(*bit_ptr++) << 3;
            byte |= (uint8_t)(*bit_ptr++) << 4;
            byte |= (uint8_t)(*bit_ptr++) << 5;
            byte |= (uint8_t)(*bit_ptr++) << 6;
            byte |= (uint8_t)(*bit_ptr++) << 7;
        }
        else
        {
            for (size_t k = 0; k < 8; k++)
            {
                if (start + k >= num_bits)
                    break;
                byte |= (uint8_t)(*bit_ptr++) << k;
            }
        }
        if (j == delimiter_byte)
        {
            byte |= (1 << delimiter_bit);
        }
        out_buf[j] = byte;
    }
    *out_size = byte_count;
    return SSZ_SUCCESS;
}

/**
 * Serializes a union type, which includes a selector and optional data.
 * The selector determines the type of the union, and the data is serialized
 * using the provided serialization function.
 *
 * @param u Pointer to the union to serialize.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_union(const ssz_union_t *u, uint8_t *out_buf, size_t *out_size)
{
    if (u == NULL || out_buf == NULL || out_size == NULL || *out_size < 1 || u->selector > 127)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (u->selector == 0)
    {
        if (u->data != NULL)
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        out_buf[0] = 0;
        *out_size = 1;
        return SSZ_SUCCESS;
    }
    out_buf[0] = (uint8_t)(u->selector & 0x7F);
    size_t used = 1;
    if (u->data != NULL)
    {
        size_t space_remaining = *out_size - used;
        ssz_error_t ret = u->serialize_fn(u->data, &out_buf[used], &space_remaining);
        if (ret != SSZ_SUCCESS)
        {
            return ret;
        }
        used += space_remaining;
    }
    *out_size = used;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint8 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint8(
    const uint8_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    memcpy(out_buf, elements, total_bytes);
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint16 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint16(
    const uint16_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count * sizeof(uint16_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint16_t val = elements[i];
            out_buf[2 * i] = (uint8_t)(val);
            out_buf[2 * i + 1] = (uint8_t)(val >> 8);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint32 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint32(
    const uint32_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count * sizeof(uint32_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint32_t val = elements[i];
            size_t base = i * 4;
            out_buf[base + 0] = (uint8_t)(val);
            out_buf[base + 1] = (uint8_t)(val >> 8);
            out_buf[base + 2] = (uint8_t)(val >> 16);
            out_buf[base + 3] = (uint8_t)(val >> 24);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint64 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint64(
    const uint64_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count * sizeof(uint64_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint64_t val = elements[i];
            size_t base = i * 8;
            out_buf[base + 0] = (uint8_t)(val);
            out_buf[base + 1] = (uint8_t)(val >> 8);
            out_buf[base + 2] = (uint8_t)(val >> 16);
            out_buf[base + 3] = (uint8_t)(val >> 24);
            out_buf[base + 4] = (uint8_t)(val >> 32);
            out_buf[base + 5] = (uint8_t)(val >> 40);
            out_buf[base + 6] = (uint8_t)(val >> 48);
            out_buf[base + 7] = (uint8_t)(val >> 56);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint128 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint128(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count * 16;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            const uint8_t *src = (const uint8_t *)elements + (i * 16);
            uint8_t *dst = out_buf + (i * 16);
            for (size_t j = 0; j < 16; j++)
            {
                dst[j] = src[15 - j];
            }
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of uint256 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint256(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count * 32;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            const uint8_t *src = (const uint8_t *)elements + (i * 32);
            uint8_t *dst = out_buf + (i * 32);
            for (size_t j = 0; j < 32; j++)
            {
                dst[j] = src[31 - j];
            }
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a vector of bool elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_bool(
    const bool *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL || element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bytes = element_count;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    for (size_t i = 0; i < element_count; i++)
    {
        out_buf[i] = (uint8_t)(elements[i]);
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint8 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint8(
    const uint8_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    memcpy(out_buf, elements, total_bytes);
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint16 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint16(
    const uint16_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count * sizeof(uint16_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint16_t val = elements[i];
            out_buf[2 * i] = (uint8_t)(val);
            out_buf[2 * i + 1] = (uint8_t)(val >> 8);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint32 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint32(
    const uint32_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count * sizeof(uint32_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint32_t val = elements[i];
            size_t base = i * 4;
            out_buf[base + 0] = (uint8_t)(val);
            out_buf[base + 1] = (uint8_t)(val >> 8);
            out_buf[base + 2] = (uint8_t)(val >> 16);
            out_buf[base + 3] = (uint8_t)(val >> 24);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint64 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint64(
    const uint64_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count * sizeof(uint64_t);
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            uint64_t val = elements[i];
            size_t base = i * 8;
            out_buf[base + 0] = (uint8_t)(val);
            out_buf[base + 1] = (uint8_t)(val >> 8);
            out_buf[base + 2] = (uint8_t)(val >> 16);
            out_buf[base + 3] = (uint8_t)(val >> 24);
            out_buf[base + 4] = (uint8_t)(val >> 32);
            out_buf[base + 5] = (uint8_t)(val >> 40);
            out_buf[base + 6] = (uint8_t)(val >> 48);
            out_buf[base + 7] = (uint8_t)(val >> 56);
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint128 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint128(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count * 16;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            const uint8_t *src = (const uint8_t *)elements + (i * 16);
            uint8_t *dst = out_buf + (i * 16);
            for (size_t j = 0; j < 16; j++)
            {
                dst[j] = src[15 - j];
            }
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of uint256 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint256(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count * 32;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_buf, elements, total_bytes);
    }
    else
    {
        for (size_t i = 0; i < element_count; i++)
        {
            const uint8_t *src = (const uint8_t *)elements + (i * 32);
            uint8_t *dst = out_buf + (i * 32);
            for (size_t j = 0; j < 32; j++)
            {
                dst[j] = src[31 - j];
            }
        }
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}

/**
 * Serializes a list of bool elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_bool(
    const bool *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size)
{
    if (elements == NULL || out_buf == NULL || out_size == NULL)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    size_t total_bytes = element_count;
    if (*out_size < total_bytes || !check_max_offset(total_bytes))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    for (size_t i = 0; i < element_count; i++)
    {
        out_buf[i] = (uint8_t)(elements[i]);
    }
    *out_size = total_bytes;
    return SSZ_SUCCESS;
}