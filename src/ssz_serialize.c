#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ssz_serialize.h"
#include "ssz_constants.h"
#include "ssz_types.h"
#include "ssz_utils.h"

/**
 * Prepares the serialization of a variable-sized array by calculating the total size
 * required for fixed-size elements and offsets for variable-size data. This function
 * writes the offsets into the output buffer and computes the total size used.
 * 
 * @param element_count The number of elements in the array.
 * @param element_sizes Array of sizes for each element.
 * @param out_buf The output buffer to write the offsets.
 * @param out_size Pointer to the size of the output buffer. Updated with the total size used.
 * @param fixed_region_size_out Pointer to store the size of the fixed region.
 * @param variable_offset_out Pointer to store the size of the variable region.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
static ssz_error_t prepare_variable_sized_array(
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

/**
 * Copies variable-sized elements into the output buffer after the offsets have been written.
 * This function performs the data copying in a single pass to improve memory locality
 * and reduce redundant pointer arithmetic.
 * 
 * @param elements Pointer to the input elements.
 * @param element_count The number of elements in the array.
 * @param element_sizes Array of sizes for each element.
 * @param out_buf The output buffer to write the serialized data.
 * @param fixed_region_size The size of the fixed region containing offsets.
 * @return SSZ_SUCCESS on success, or an error code on failure.
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

/**
 * Serializes a variable-sized array by writing offsets and copying data into the output buffer.
 * This function performs the serialization in two phases: offset writing and data copying.
 * 
 * @param element_count The number of elements in the array.
 * @param element_sizes Array of sizes for each element.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the total size used.
 * @return SSZ_SUCCESS on success, or an error code on failure.
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

/**
 * Serializes an unsigned integer of a specified bit size into a buffer.
 * Supports bit sizes of 8, 16, 32, 64, 128, and 256. The serialized value
 * is written in little-endian format.
 * 
 * @param value Pointer to the integer value to serialize.
 * @param bit_size The size of the integer in bits.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
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
    if (!out_buf || !out_size || *out_size < 1)
    {
        return SSZ_ERROR_SERIALIZATION;
    }
    out_buf[0] = (value ? 0x01 : 0x00);
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
ssz_error_t ssz_serialize_bitvector(const bool *bits, size_t num_bits, uint8_t *out_buf, size_t *out_size)
{
    if (!bits || !out_buf || !out_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }

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

/**
 * Serializes a vector, which is a fixed-length collection of elements.
 * If the elements are variable-sized, offsets are included in the serialization.
 * 
 * @param elements Pointer to the input elements.
 * @param element_count The number of elements in the vector.
 * @param element_sizes Array of sizes for each element.
 * @param is_variable_size Whether the elements are variable-sized.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
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

/**
 * Serializes a list, which is a variable-length collection of elements.
 * If the elements are variable-sized, offsets are included in the serialization.
 * 
 * @param elements Pointer to the input elements.
 * @param element_count The number of elements in the list.
 * @param element_sizes Array of sizes for each element.
 * @param is_variable_size Whether the elements are variable-sized.
 * @param out_buf The output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
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