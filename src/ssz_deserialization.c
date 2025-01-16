#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/ssz_deserialization.h"
#include "../include/ssz_constants.h"
#include "../include/ssz_types.h"
#include "../include/ssz_utils.h"

/*
 * A helper that reads 'count' offsets from the buffer’s “fixed region,” checking each offset
 * for validity and ensuring they are strictly ascending. If successful, it stores them in an
 * allocated array (*out_offsets). The caller must free that array. For vectors, 'strict_count_match'
 * ensures we do not accept leftover bytes in the buffer. For lists, we may parse as many as
 * we can if the function design allows it.
 */
static ssz_error_t read_variable_offsets_and_check_ascending(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t count,
    uint32_t **out_offsets,
    bool strict_count_match)
{
    size_t offset_region = count * BYTES_PER_LENGTH_OFFSET;
    if (offset_region > buffer_size)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    uint32_t *offsets = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (!offsets)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    for (size_t i = 0; i < count; i++)
    {
        size_t idx = i * BYTES_PER_LENGTH_OFFSET;
        if (!read_offset_le(buffer, buffer_size, idx, &offsets[i]) ||
            !check_max_offset(offsets[i]) ||
            (offsets[i] > buffer_size))
        {
            free(offsets);
            return SSZ_ERROR_DESERIALIZATION;
        }
    }
    for (size_t i = 1; i < count; i++)
    {
        if (offsets[i] <= offsets[i - 1])
        {
            free(offsets);
            return SSZ_ERROR_DESERIALIZATION;
        }
    }
    /*
     * If this is for a vector, we often need to ensure the data region
     * ends exactly at buffer_size. That means the last offset plus that
     * field’s size must match the buffer boundary. We won’t do that just yet;
     * we let the caller handle verifying each element’s length plus offset
     * matches up to buffer_size. If strict_count_match is set, we can do
     * an extra check to ensure the final offset points to the end of the buffer
     * after reading that last slice, but we’ll let the caller do the final
     * checks element-by-element. 
     */
    *out_offsets = offsets;
    return SSZ_SUCCESS;
}

/*
 * Helper function to copy variable-sized slices from the buffer to out_elements, using the
 * provided offsets, ensuring each slice matches field_sizes[i]. If this is for a vector,
 * we confirm that slice_len == field_sizes[i]. If for a list, we can stop once we run
 * out of offsets or buffer.
 */
static ssz_error_t copy_variable_sized_slices(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t count,
    const size_t *field_sizes,
    const uint32_t *offsets,
    void *out_elements)
{
    uint8_t *write_ptr = (uint8_t *)out_elements;
    size_t write_offset = 0;
    for (size_t i = 0; i < count; i++)
    {
        uint32_t start_off = offsets[i];
        uint32_t end_off = (i + 1 < count) ? offsets[i + 1] : (uint32_t)buffer_size;
        if (end_off > buffer_size || end_off < start_off)
        {
            return SSZ_ERROR_DESERIALIZATION;
        }
        size_t slice_len = end_off - start_off;
        if (slice_len != field_sizes[i])
        {
            return SSZ_ERROR_DESERIALIZATION;
        }
        memcpy(write_ptr + write_offset, &buffer[start_off], slice_len);
        write_offset += slice_len;
    }
    return SSZ_SUCCESS;
}

/*
 * Deserializes a uintN with bit_size in [8,16,32,64,128,256].
 */
ssz_error_t ssz_deserialize_uintN(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t bit_size,
    void *out_value)
{
    if (!buffer || !out_value)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    if (bit_size != 8 && bit_size != 16 && bit_size != 32 &&
        bit_size != 64 && bit_size != 128 && bit_size != 256)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    size_t byte_size = bit_size / 8;
    if (buffer_size < byte_size)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    if (bit_size <= 64)
    {
        uint64_t *out_u64 = (uint64_t *)out_value;
        uint64_t result = 0;
        for (size_t i = 0; i < byte_size; i++)
        {
            result |= ((uint64_t)buffer[i] << (8 * i));
        }
        *out_u64 = result;
    }
    else
    {
        memcpy(out_value, buffer, byte_size);
    }
    return SSZ_SUCCESS;
}

/*
 * Deserializes a boolean (0x00 => false, 0x01 => true).
 */
ssz_error_t ssz_deserialize_boolean(
    const uint8_t *buffer,
    size_t buffer_size,
    bool *out_value)
{
    if (!buffer || !out_value || buffer_size < 1)
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

/*
 * Deserializes a bitvector of exactly num_bits length.
 * Reads (num_bits+7)//8 bytes and populates out_bits accordingly.
 */
ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t num_bits,
    bool *out_bits)
{
    if (!buffer || !out_bits)
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

/*
 * Deserializes a bitlist with up to max_bits. It looks for the highest set bit as the "boundary bit."
 * That boundary must fall within max_bits+1. All bits above the boundary must be zero.
 * Then the bits up to boundary-1 become the data bits. 
 */
ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_bits,
    bool *out_bits,
    size_t *out_actual_bits)
{
    if (!buffer || !out_bits || !out_actual_bits)
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
    {
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

/*
 * Deserializes a union by reading the first byte as selector (<= 127).
 * If selector == 0, data is NULL, else we parse using out_union->deserialize_fn.
 */
ssz_error_t ssz_deserialize_union(
    const uint8_t *buffer,
    size_t buffer_size,
    ssz_union_t *out_union)
{
    if (!buffer || !out_union || buffer_size < 1)
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
    if (!out_union->deserialize_fn)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    const uint8_t *subtype_buf = &buffer[1];
    size_t subtype_size = buffer_size - 1;
    return out_union->deserialize_fn(subtype_buf, subtype_size, &out_union->data);
}

/*
 * Deserializes a vector of elements. For variable-size, we read offsets in one pass
 * (ensuring strict ascending order), then copy all slices in a second pass.
 */
ssz_error_t ssz_deserialize_vector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    bool is_variable_size,
    void *out_elements)
{
    if (!buffer || !field_sizes || !out_elements || element_count == 0)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    if (!is_variable_size)
    {
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
    else
    {
        uint32_t *offsets = NULL;
        ssz_error_t ret = read_variable_offsets_and_check_ascending(
            buffer, buffer_size,
            element_count, &offsets,
            true /* for a vector we generally expect strict matching, but we'll do final checks below */
        );
        if (ret != SSZ_SUCCESS)
        {
            return ret;
        }
        ret = copy_variable_sized_slices(buffer, buffer_size, element_count, field_sizes, offsets, out_elements);
        free(offsets);
        return ret;
    }
}

/*
 * Deserializes a list of elements. For variable-size, we read offsets in one pass,
 * then copy slices. We allow the possibility that not all offsets are valid if the
 * buffer is truncated; we parse as many as we can. out_actual_count is how many
 * elements we actually read successfully.
 */
ssz_error_t ssz_deserialize_list(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    bool is_variable_size,
    void *out_elements,
    size_t *out_actual_count)
{
    if (!buffer || !field_sizes || !out_elements || !out_actual_count)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    *out_actual_count = 0;
    if (element_count == 0)
    {
        return SSZ_SUCCESS; 
    }
    if (!is_variable_size)
    {
        size_t offset_in_buf = 0;
        uint8_t *write_ptr = (uint8_t *)out_elements;
        for (size_t i = 0; i < element_count; i++)
        {
            size_t fs = field_sizes[i];
            if (offset_in_buf + fs > buffer_size)
            {
                break;
            }
            memcpy(write_ptr, &buffer[offset_in_buf], fs);
            write_ptr += fs;
            offset_in_buf += fs;
            (*out_actual_count)++;
        }
        return SSZ_SUCCESS;
    }
    else
    {
        /*
         * We need to figure out how many offsets we can read. We might not read all 'element_count'
         * if the buffer is too small. 
         */
        size_t max_possible = buffer_size / BYTES_PER_LENGTH_OFFSET;
        if (max_possible < element_count)
        {
            element_count = max_possible;
        }
        uint32_t *offsets = (uint32_t *)malloc(element_count * sizeof(uint32_t));
        if (!offsets)
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
        /*
         * Now copy slices for however many offsets we validated. We treat each field_sizes[i]
         * individually up to valid_offsets. 
         */
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
}
