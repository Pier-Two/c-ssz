#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/ssz_serialization.h"
#include "../include/ssz_constants.h"
#include "../include/ssz_types.h"

/* 
 * Marking these static inline can help eliminate some function call overhead in hot loops.
 * If the compiler decides inlining is appropriate, it will do so.
 */

static inline void write_offset_le(uint32_t offset, uint8_t *out)
{
    out[0] = (uint8_t)(offset & 0xFF);
    out[1] = (uint8_t)((offset >> 8) & 0xFF);
    out[2] = (uint8_t)((offset >> 16) & 0xFF);
    out[3] = (uint8_t)((offset >> 24) & 0xFF);
}

static inline bool check_max_offset(size_t offset)
{
    size_t max_offset = ((size_t)1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE));
    return (offset < max_offset);
}

/* 
 * A helper function that calculates the total size required for serializing fixed-size elements 
 * and also calculates offsets for variable-size data. This is used by both vector and list logic. 
 * It consolidates the offset writing in one pass and returns the final total used on success. 
 * The 'elements' pointer is not actually copied here; we only compute offsets and write them. 
 */
static ssz_error_t prepare_variable_sized_array(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    uint8_t *out_buf,
    size_t *out_size,
    size_t *fixed_region_size_out,
    size_t *variable_offset_out)
{
    size_t fixed_region_size = element_count * BYTES_PER_LENGTH_OFFSET;
    if (*out_size < fixed_region_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    *fixed_region_size_out = fixed_region_size;
    uint8_t *offset_ptr = out_buf;
    size_t variable_offset = 0;
    for (size_t i = 0; i < element_count; i++)
    {
        size_t elem_size = element_sizes[i];
        uint32_t this_offset_le = (uint32_t)(fixed_region_size + variable_offset);
        if (!check_max_offset(this_offset_le))
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        write_offset_le(this_offset_le, &offset_ptr[i * BYTES_PER_LENGTH_OFFSET]);
        if (fixed_region_size + variable_offset + elem_size > *out_size)
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        variable_offset += elem_size;
    }
    *variable_offset_out = variable_offset;
    return SSZ_SUCCESS;
}

/* 
 * This helper does the actual data copying of variable elements after the offsets have been written. 
 * It consolidates the copying into a single pass for better locality and fewer repeated pointer increments.
 */
static ssz_error_t copy_variable_sized_array(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    uint8_t *out_buf,
    size_t fixed_region_size)
{
    const uint8_t *src = (const uint8_t *)elements;
    size_t src_offset = 0;
    uint8_t *variable_ptr = out_buf + fixed_region_size;
    for (size_t i = 0; i < element_count; i++)
    {
        size_t elem_size = element_sizes[i];
        memcpy(variable_ptr, src + src_offset, elem_size);
        src_offset += elem_size;
        variable_ptr += elem_size;
    }
    return SSZ_SUCCESS;
}

/* 
 * A single helper that encapsulates the “variable-size” portion of vectors/lists. 
 * It does the offset writing and the data copying in two phases, which can be 
 * more performant than interleaving them each iteration in a single loop. 
 */
static ssz_error_t serialize_variable_sized_array(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    uint8_t *out_buf,
    size_t *out_size)
{
    size_t fixed_region_size = 0;
    size_t variable_offset = 0;
    ssz_error_t ret = prepare_variable_sized_array(
        elements,
        element_count,
        element_sizes,
        out_buf,
        out_size,
        &fixed_region_size,
        &variable_offset
    );
    if (ret != SSZ_SUCCESS)
    {
        return ret;
    }
    ret = copy_variable_sized_array(elements, element_count, element_sizes, out_buf, fixed_region_size);
    if (ret != SSZ_SUCCESS)
    {
        return ret;
    }
    size_t total_used = fixed_region_size + variable_offset;
    if (!check_max_offset(total_used))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    *out_size = total_used;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_serialize_uintN(
    const void *value,
    size_t bit_size,
    uint8_t *out_buf,
    size_t *out_size)
{
    if (!value || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (bit_size != 8 && bit_size != 16 && bit_size != 32 &&
        bit_size != 64 && bit_size != 128 && bit_size != 256)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t byte_size = bit_size / BITS_PER_BYTE;
    if (*out_size < byte_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (bit_size <= 64)
    {
        uint64_t val = *(const uint64_t *)value;
        for (size_t i = 0; i < byte_size; i++)
        {
            out_buf[i] = (uint8_t)((val >> (8 * i)) & 0xFF);
        }
    }
    else
    {
        memcpy(out_buf, value, byte_size);
    }
    *out_size = byte_size;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_serialize_boolean(bool value, uint8_t *out_buf, size_t *out_size)
{
    if (!out_buf || !out_size || *out_size < 1)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    out_buf[0] = (value ? 0x01 : 0x00);
    *out_size = 1;
    return SSZ_SUCCESS;
}

/* 
 * In principle, you could optimize bitvector serialization using bit tricks or 
 * by grouping bits in bytes at once, but this is already fairly compact. 
 * You can consider unrolling if you need further speed. For now, only small refactoring. 
 */
ssz_error_t ssz_serialize_bitvector(const bool *bits, size_t num_bits, uint8_t *out_buf, size_t *out_size)
{
    if (!bits || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }

    // Explicitly reject zero-length bitvectors:
    if (num_bits == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }

    size_t byte_count = (num_bits + 7) / 8;
    if (*out_size < byte_count)
    {
        return SSZ_ERROR_SERIALIZATION;
    }

    memset(out_buf, 0, byte_count);
    for (size_t i = 0; i < num_bits; i++)
    {
        if (bits[i])
        {
            out_buf[i >> 3] |= (1 << (i & 7));
        }
    }

    *out_size = byte_count;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_serialize_bitlist(const bool *bits, size_t num_bits, uint8_t *out_buf, size_t *out_size)
{
    if (!bits || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    size_t total_bits = num_bits + 1;
    size_t byte_count = (total_bits + 7) / 8;
    if (*out_size < byte_count)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    memset(out_buf, 0, byte_count);
    for (size_t i = 0; i < num_bits; i++)
    {
        if (bits[i])
        {
            out_buf[i >> 3] |= (1 << (i & 7));
        }
    }
    out_buf[num_bits >> 3] |= (1 << (num_bits & 7));
    *out_size = byte_count;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_serialize_union(const ssz_union_t *u, uint8_t *out_buf, size_t *out_size)
{
    if (!u || !out_buf || !out_size || *out_size < 1)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (u->selector > 127)
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

/* 
 * The vector function is now shorter and uses the new serialize_variable_sized_array helper 
 * if the elements are variable-size. 
 */
ssz_error_t ssz_serialize_vector(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size)
{
    if (!elements || !element_sizes || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (!is_variable_size)
    {
        size_t total_bytes = 0;
        for (size_t i = 0; i < element_count; i++)
        {
            total_bytes += element_sizes[i];
        }
        if (*out_size < total_bytes || !check_max_offset(total_bytes))
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        memcpy(out_buf, elements, total_bytes);
        *out_size = total_bytes;
        return SSZ_SUCCESS;
    }
    else
    {
        return serialize_variable_sized_array(elements, element_count, element_sizes, out_buf, out_size);
    }
}

/* 
 * The list function is also simplified thanks to our serialize_variable_sized_array helper. 
 * We allow zero elements. 
 */
ssz_error_t ssz_serialize_list(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size)
{
    if (!elements || !element_sizes || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (element_count == 0)
    {
        *out_size = 0;
        return SSZ_SUCCESS;
    }
    if (!is_variable_size)
    {
        size_t total_bytes = 0;
        for (size_t i = 0; i < element_count; i++)
        {
            total_bytes += element_sizes[i];
        }
        if (*out_size < total_bytes || !check_max_offset(total_bytes))
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        memcpy(out_buf, elements, total_bytes);
        *out_size = total_bytes;
        return SSZ_SUCCESS;
    }
    else
    {
        return serialize_variable_sized_array(elements, element_count, element_sizes, out_buf, out_size);
    }
}

/* 
 * We can also improve container serialization by splitting out the offset calculation 
 * from the copying. We do a single pass that calculates total variable offset usage, writes offsets, 
 * and then a second pass to do the actual copying. 
 */
ssz_error_t ssz_serialize_container(
    const void *container_data,
    size_t field_count,
    const bool *field_is_variable_size,
    const size_t *field_sizes,
    uint8_t *out_buf,
    size_t *out_size)
{
    if (!container_data || !field_is_variable_size || !field_sizes || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    if (field_count == 0)
    {
        *out_size = 0;
        return SSZ_ERROR_SERIALIZATION;
    }

    size_t fixed_region_size = 0;
    for (size_t i = 0; i < field_count; i++)
    {
        if (field_is_variable_size[i])
        {
            fixed_region_size += BYTES_PER_LENGTH_OFFSET;
        }
        else
        {
            fixed_region_size += field_sizes[i];
        }
    }
    if (*out_size < fixed_region_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    uint8_t *buf_fixed = out_buf;
    uint8_t *buf_variable = out_buf + fixed_region_size;
    const uint8_t *src = (const uint8_t *)container_data;
    size_t variable_offset = 0;
    size_t data_offset = 0;

    for (size_t i = 0; i < field_count; i++)
    {
        if (!field_is_variable_size[i])
        {
            size_t fs = field_sizes[i];
            if (buf_fixed + fs > out_buf + fixed_region_size)
            {
                return SSZ_ERROR_SERIALIZATION;
            }
            memcpy(buf_fixed, src + data_offset, fs);
            buf_fixed += fs;
            data_offset += fs;
        }
        else
        {
            uint32_t offset_le = (uint32_t)(fixed_region_size + variable_offset);
            if (!check_max_offset(offset_le))
            {
                return SSZ_ERROR_SERIALIZATION;
            }
            write_offset_le(offset_le, buf_fixed);
            buf_fixed += BYTES_PER_LENGTH_OFFSET;
            size_t fs = field_sizes[i];
            if (fixed_region_size + variable_offset + fs > *out_size)
            {
                return SSZ_ERROR_SERIALIZATION;
            }
            memcpy(buf_variable, src + data_offset, fs);
            buf_variable += fs;
            data_offset += fs;
            variable_offset += fs;
        }
    }

    size_t total_used = fixed_region_size + variable_offset;
    if (!check_max_offset(total_used))
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    *out_size = total_used;
    return SSZ_SUCCESS;
}
