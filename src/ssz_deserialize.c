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
    if (buffer == NULL || out_value == NULL || buffer_size < 2)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint16_t val = 0;
    val |= (uint16_t)buffer[0];
    val |= (uint16_t)buffer[1] << 8;
    *(uint16_t *)out_value = val;
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
    size_t needed = (num_bits + 7) / 8;
    if (needed != buffer_size)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    memset(out_bits, 0, num_bits * sizeof(bool));
    for (size_t i = 0; i < num_bits; i++)
    {
        bool set = (buffer[i / 8] & (1 << (i % 8))) != 0;
        out_bits[i] = set;
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
    if (buffer == NULL || out_bits == NULL || out_actual_bits == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    if (buffer_size == 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    size_t max_bytes = (max_bits + 1 + 7) / 8;
    if (buffer_size > max_bytes)
    {
        for (size_t i = max_bytes; i < buffer_size; i++)
        {
            if (buffer[i] != 0)
            {
                return SSZ_ERROR_DESERIALIZATION;
            }
        }

        buffer_size = max_bytes;
    }

    ssize_t boundary = -1;
    for (ssize_t byte_i = (ssize_t)buffer_size - 1; byte_i >= 0 && boundary < 0; byte_i--)
    {
        uint8_t val = buffer[byte_i];
        if (val != 0)
        {
            for (int bit_i = 7; bit_i >= 0; bit_i--)
            {
                if (val & (1 << bit_i))
                {
                    boundary = (byte_i * 8) + bit_i;
                    break;
                }
            }
        }
    }

    if (boundary < 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    if ((size_t)boundary > max_bits)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    uint8_t boundary_mask = (uint8_t)((1 << ((boundary % 8) + 1)) - 1);
    uint8_t masked_val = buffer[boundary / 8] & ~boundary_mask;
    if (masked_val != 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    for (size_t i = (boundary / 8) + 1; i < buffer_size; i++)
    {
        if (buffer[i] != 0)
        {
            return SSZ_ERROR_DESERIALIZATION;
        }
    }

    size_t data_bits = (size_t)boundary;
    *out_actual_bits = data_bits;

    memset(out_bits, 0, max_bits * sizeof(bool));
    for (size_t i = 0; i < data_bits; i++)
    {
        bool is_set = (buffer[i / 8] & (1 << (i % 8))) != 0;
        out_bits[i] = is_set;
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

    size_t needed = 0;
    for (size_t i = 0; i < element_count; i++)
    {
        needed += field_sizes[i];
    }

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
    if (buffer == NULL || field_sizes == NULL || out_elements == NULL || out_actual_count == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    *out_actual_count = 0;
    if (element_count == 0)
    {
        return SSZ_SUCCESS;
    }
    size_t max_possible = buffer_size / BYTES_PER_LENGTH_OFFSET;
    if (max_possible < element_count)
    {
        element_count = max_possible;
    }
    uint32_t *offsets = (uint32_t *)malloc(element_count * sizeof(uint32_t));
    if (offsets == NULL)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    size_t valid_offsets = 0;
    for (size_t i = 0; i < element_count; i++)
    {
        size_t idx = i * BYTES_PER_LENGTH_OFFSET;
        uint32_t off = 0;
        if (!read_offset_le(buffer, buffer_size, idx, &off) ||
            !check_max_offset(off) ||
            off > buffer_size)
        {
            break;
        }
        offsets[i] = off;
        if (i > 0 && offsets[i] <= offsets[i - 1])
        {
            free(offsets);
            return SSZ_ERROR_DESERIALIZATION;
        }
        valid_offsets++;
    }
    uint8_t *write_ptr = (uint8_t *)out_elements;
    size_t total_parsed = 0;
    for (size_t i = 0; i < valid_offsets; i++)
    {
        uint32_t start_off = offsets[i];
        uint32_t end_off = (i + 1 < valid_offsets) ? offsets[i + 1] : (uint32_t)buffer_size;
        if (end_off < start_off || end_off > buffer_size)
        {
            break;
        }
        size_t slice_len = end_off - start_off;
        if (slice_len != field_sizes[i])
        {
            break;
        }
        memcpy(write_ptr, &buffer[start_off], slice_len);
        write_ptr += slice_len;
        total_parsed++;
    }
    free(offsets);
    *out_actual_count = total_parsed;
    return SSZ_SUCCESS;
}