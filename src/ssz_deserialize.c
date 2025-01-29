#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "ssz_types.h"
#include "ssz_utils.h"

/**
 * Deserializes an 8-bit unsigned integer from a single byte.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 8-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint8(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 1)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    *(uint8_t *)out_value = buffer[0];
    return SSZ_SUCCESS;
}

/**
 * Deserializes a 16-bit unsigned integer from two bytes.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 16-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint16(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer_size < 2 || buffer == NULL || out_value == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint16_t val = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
    memcpy(out_value, &val, sizeof(val));
    return SSZ_SUCCESS;
}

/**
 * Deserializes a 32-bit unsigned integer from four bytes.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 32-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint32(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 4)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint32_t val = 0;
    val |= (uint32_t)buffer[0];
    val |= (uint32_t)buffer[1] << 8;
    val |= (uint32_t)buffer[2] << 16;
    val |= (uint32_t)buffer[3] << 24;
    *(uint32_t *)out_value = val;
    return SSZ_SUCCESS;
}

/**
 * Deserializes a 64-bit unsigned integer from eight bytes.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 64-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint64(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 8)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint64_t val = 0;
    val |= (uint64_t)buffer[0];
    val |= (uint64_t)buffer[1] << 8;
    val |= (uint64_t)buffer[2] << 16;
    val |= (uint64_t)buffer[3] << 24;
    val |= (uint64_t)buffer[4] << 32;
    val |= (uint64_t)buffer[5] << 40;
    val |= (uint64_t)buffer[6] << 48;
    val |= (uint64_t)buffer[7] << 56;
    *(uint64_t *)out_value = val;
    return SSZ_SUCCESS;
}

/**
 * Deserializes a 128-bit unsigned integer from sixteen bytes.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 128-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint128(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 16)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t *endian_check = (const uint8_t *)&test_value;
    if (endian_check[0] == 0x01)
    {
        memcpy(out_value, buffer, 16);
    }
    else
    {
        uint8_t *dest = (uint8_t *)out_value;
        for (size_t i = 0; i < 16; i++)
        {
            dest[i] = buffer[15 - i];
        }
    }
    return SSZ_SUCCESS;
}

/**
 * Deserializes a 256-bit unsigned integer from thirty-two bytes.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param out_value     Pointer to store the deserialized 256-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint256(const uint8_t *buffer, size_t buffer_size, void *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 32)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    static const uint32_t test_value = 1;
    const uint8_t endian_check = *(const uint8_t *)&test_value;
    if (endian_check == 0x01)
    {
        memcpy(out_value, buffer, 32);
    }
    else
    {
        uint8_t *dest = (uint8_t *)out_value;
        for (size_t i = 0; i < 32; i++)
        {
            dest[i] = buffer[31 - i];
        }
    }
    return SSZ_SUCCESS;
}

/**
 * Deserializes a boolean value (0x00 => false, 0x01 => true).
 *
 * This function reads a single byte from the buffer and interprets it as a boolean value.
 * Any value other than 0x00 or 0x01 is considered invalid.
 *
 * @param buffer      The input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer.
 * @param out_value   Pointer to store the deserialized boolean value.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_boolean(
    const uint8_t *buffer,
    size_t buffer_size,
    bool *out_value)
{
    if (buffer == NULL || out_value == NULL || buffer_size < 1)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    if (buffer[0] == 0x00)
    {
        *out_value = false;
    }
    else if (buffer[0] == 0x01)
    {
        *out_value = true;
    }
    else
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    return SSZ_SUCCESS;
}

/**
 * Deserializes a bitvector of exactly num_bits length.
 *
 * This function reads a fixed-length bitvector from the buffer. The number of bits
 * is specified by num_bits, and the function ensures that the buffer size matches
 * the required number of bytes.
 *
 * @param buffer      The input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer.
 * @param num_bits    The number of bits in the bitvector.
 * @param out_bits    Pointer to store the deserialized bitvector.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t num_bits,
    bool *out_bits)
{
    if (buffer == NULL || out_bits == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    const size_t needed = (num_bits + 7) / 8;
    if (needed != buffer_size)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    const size_t full_bytes = num_bits / 8;
    const size_t remainder_bits = num_bits % 8;
    bool *out_ptr = out_bits;

    for (size_t i = 0; i < full_bytes; ++i)
    {
        const uint8_t byte = buffer[i];
        *out_ptr++ = (byte & 0x01) != 0;
        *out_ptr++ = (byte & 0x02) != 0;
        *out_ptr++ = (byte & 0x04) != 0;
        *out_ptr++ = (byte & 0x08) != 0;
        *out_ptr++ = (byte & 0x10) != 0;
        *out_ptr++ = (byte & 0x20) != 0;
        *out_ptr++ = (byte & 0x40) != 0;
        *out_ptr++ = (byte & 0x80) != 0;
    }

    if (remainder_bits > 0)
    {
        const uint8_t byte = buffer[full_bytes];
        for (size_t bit = 0; bit < remainder_bits; ++bit)
        {
            *out_ptr++ = (byte & (1 << bit)) != 0;
        }
    }

    return SSZ_SUCCESS;
}

/**
 * Deserializes a bitlist with up to max_bits.
 *
 * This function reads a bitlist from the buffer, ensuring that the highest set bit
 * (the boundary bit) is within max_bits + 1. All bits above the boundary must be zero.
 * The bits up to boundary - 1 are considered the data bits.
 *
 * @param buffer           The input buffer containing the serialized data.
 * @param buffer_size      The size of the input buffer.
 * @param max_bits         The maximum number of bits in the bitlist.
 * @param out_bits         Pointer to store the deserialized bitlist.
 * @param out_actual_bits  Pointer to store the actual number of bits in the bitlist.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_bits,
    bool *out_bits,
    size_t *out_actual_bits)
{
    if (buffer == NULL || out_bits == NULL || out_actual_bits == NULL || buffer_size == 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    const size_t max_bytes = (max_bits + 8) / 8;
    if (buffer_size > max_bytes)
    {
        if (!is_all_zero(buffer + max_bytes, buffer_size - max_bytes))
        {
            return SSZ_ERROR_DESERIALIZATION;
        }
        buffer_size = max_bytes;
    }

    ssize_t boundary = -1;
    for (ssize_t byte_i = (ssize_t)buffer_size - 1; byte_i >= 0; byte_i--)
    {
        const uint8_t val = buffer[byte_i];
        if (val != 0)
        {
            const int bit = highest_bit_table[val];
            boundary = (byte_i * 8) + bit;
            break;
        }
    }

    if (boundary < 0 || (size_t)boundary > max_bits)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    const size_t boundary_byte = (boundary / 8) + 1;
    if (boundary_byte < buffer_size)
    {
        if (!is_all_zero(buffer + boundary_byte, buffer_size - boundary_byte))
        {
            return SSZ_ERROR_DESERIALIZATION;
        }
    }

    const uint8_t boundary_mask = (uint8_t)((1U << ((boundary % 8) + 1)) - 1);
    if ((buffer[boundary / 8] & ~boundary_mask) != 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    const size_t data_bits = (size_t)boundary;
    *out_actual_bits = data_bits;

    if (data_bits < max_bits)
    {
        memset(out_bits + data_bits, 0, (max_bits - data_bits) * sizeof(bool));
    }

    const size_t full_bytes = data_bits / 8;
    const size_t rem_bits = data_bits % 8;
    size_t i = 0;
    for (; i < full_bytes; i++)
    {
        const uint8_t val = buffer[i];
        out_bits[i * 8 + 0] = val & 0x01;
        out_bits[i * 8 + 1] = val & 0x02;
        out_bits[i * 8 + 2] = val & 0x04;
        out_bits[i * 8 + 3] = val & 0x08;
        out_bits[i * 8 + 4] = val & 0x10;
        out_bits[i * 8 + 5] = val & 0x20;
        out_bits[i * 8 + 6] = val & 0x40;
        out_bits[i * 8 + 7] = val & 0x80;
    }
    if (rem_bits > 0)
    {
        const uint8_t val = buffer[full_bytes];
        for (size_t bit = 0; bit < rem_bits; bit++)
        {
            out_bits[i * 8 + bit] = val & (1 << bit);
        }
    }

    return SSZ_SUCCESS;
}

/**
 * Deserializes a union by reading the first byte as a selector.
 *
 * This function interprets the first byte of the buffer as a selector value. If the selector
 * is 0, the union's data is set to NULL. Otherwise, the function uses the provided
 * deserialize_fn to parse the union's data.
 *
 * @param buffer      The input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer.
 * @param out_union   Pointer to store the deserialized union.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_union(
    const uint8_t *buffer,
    size_t buffer_size,
    ssz_union_t *out_union)
{
    if (buffer == NULL || out_union == NULL || buffer_size < 1)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint8_t selector = buffer[0];
    if (selector > 127)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    out_union->selector = selector;
    if (selector == 0)
    {
        out_union->data = NULL;
        return SSZ_SUCCESS;
    }
    if (out_union->deserialize_fn == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    const uint8_t *subtype_buf = &buffer[1];
    size_t subtype_size = buffer_size - 1;
    return out_union->deserialize_fn(subtype_buf, subtype_size, &out_union->data);
}

/**
 * Deserializes a fixed-size vector of elements.
 *
 * This function deserializes a fixed-sized vector of elements from the buffer.
 *
 * @param buffer        The input buffer containing the serialized data.
 * @param buffer_size   The size of the input buffer.
 * @param element_count The number of elements in the vector.
 * @param field_sizes   Array of sizes for each field.
 * @param out_elements  Pointer to store the deserialized vector elements.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_vector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    void *out_elements)
{
    if (buffer == NULL || field_sizes == NULL || out_elements == NULL || element_count == 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    const size_t element_size = field_sizes[0];
    const size_t needed = element_count * element_size;
    if (needed != buffer_size)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    memcpy(out_elements, buffer, needed);
    return SSZ_SUCCESS;
}

/**
 * Deserializes a list of elements.
 *
 * This function deserializes a variable-sized list of elements from the buffer.
 * It reads and validates offsets, then copies the slices into the output elements.
 * The function also tracks the actual number of successfully deserialized elements.
 *
 * @param buffer            The input buffer containing the serialized data.
 * @param buffer_size       The size of the input buffer.
 * @param element_count     The number of elements in the list.
 * @param field_sizes       Array of sizes for each field.
 * @param out_elements      Pointer to store the deserialized list elements.
 * @param out_actual_count  Pointer to store the actual number of elements deserialized.
 *
 * @return SSZ_SUCCESS on success, or an appropriate error code on failure.
 */
ssz_error_t ssz_deserialize_list(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    void *out_elements,
    size_t *out_actual_count)
{
    if (!buffer || !field_sizes || !out_elements || !out_actual_count)
        return SSZ_ERROR_DESERIALIZATION;

    *out_actual_count = 0;
    if (element_count == 0)
        return SSZ_SUCCESS;

    const size_t max_possible = buffer_size / BYTES_PER_LENGTH_OFFSET;
    if (max_possible < element_count)
        element_count = max_possible;
    if (element_count == 0)
        return SSZ_SUCCESS;

    const uint8_t *offset_ptr = buffer;
    uint32_t prev_offset = 0;
    uint8_t *write_ptr = (uint8_t *)out_elements;
    size_t total_parsed = 0;

    for (size_t i = 0; i < element_count; ++i, offset_ptr += BYTES_PER_LENGTH_OFFSET)
    {
        if (buffer_size < (size_t)((offset_ptr - buffer) + BYTES_PER_LENGTH_OFFSET))
            break;

        const uint32_t current_offset = *(const uint32_t *)offset_ptr;
        if (current_offset > buffer_size)
            break;

        if (i > 0)
        {
            if (current_offset <= prev_offset)
                return SSZ_ERROR_DESERIALIZATION;

            const size_t slice_len = current_offset - prev_offset;
            if (slice_len != field_sizes[i - 1])
                break;

            memcpy(write_ptr, buffer + prev_offset, slice_len);
            write_ptr += slice_len;
            ++total_parsed;
        }

        prev_offset = current_offset;
    }

    if (total_parsed < element_count)
    {
        const size_t slice_len = buffer_size - prev_offset;
        if (slice_len == field_sizes[total_parsed])
        {
            memcpy(write_ptr, buffer + prev_offset, slice_len);
            ++total_parsed;
        }
    }

    *out_actual_count = total_parsed;
    return (total_parsed > 0) ? SSZ_SUCCESS : SSZ_ERROR_DESERIALIZATION;
}