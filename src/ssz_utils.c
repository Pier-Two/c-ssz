#include <stddef.h>
#include <string.h>
#include "ssz_utils.h"
#include "ssz_constants.h"

/**
 * Determines whether a given value is a power of two.
 * 
 * A value is considered a power of two if it satisfies the condition
 * (x & (x - 1)) == 0, provided x is not zero. Zero is explicitly
 * defined as not a power of two.
 * 
 * @param value The input value to check.
 * @return true if the value is a power of two, false otherwise.
 */
bool is_power_of_two(uint64_t value)
{
    if (value == 0)
    {
        return false;
    }
    return (value & (value - 1)) == 0;
}

/**
 * Computes the next power of two for the given value.
 * 
 * If the input value is already a power of two, the same value is returned.
 * If the input value is zero, the result is defined as 1. Otherwise, the
 * function calculates the next power of two by repeatedly shifting until
 * the highest set bit in the input value is surpassed.
 * 
 * @param value The input value.
 * @return The next power of two.
 */
uint64_t next_pow_of_two(uint64_t value)
{
    if (value == 0)
    {
        return 1;
    }
    if (is_power_of_two(value))
    {
        return value;
    }
    uint64_t p = 1;
    while (p < value)
    {
        p <<= 1;
    }
    return p;
}

/**
 * Validates whether the provided offset is within the maximum allowed SSZ offset range.
 * 
 * The maximum offset is determined by the constant BYTES_PER_LENGTH_OFFSET, which
 * typically represents 4 bytes in SSZ. The maximum offset is computed as
 * (1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE)). The function returns true
 * if the offset is less than the maximum allowed value, and false otherwise.
 * 
 * @param offset The offset to validate.
 * @return true if the offset is within the allowed range, false otherwise.
 */
bool check_max_offset(size_t offset)
{
    size_t max_offset = ((size_t)1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE));
    return (offset < max_offset);
}

/**
 * Reads a 4-byte little-endian offset from the source buffer at the specified index.
 * 
 * The function reads four bytes starting at the given offset index in the source
 * buffer and combines them into a 32-bit value in little-endian order. If the
 * source buffer is too small or if the pointers are invalid, the function returns
 * false. On success, the 32-bit offset value is stored in out_offset.
 * 
 * @param src Pointer to the source buffer.
 * @param src_size The size of the source buffer in bytes.
 * @param offset_index The index in the buffer to read the offset from.
 * @param out_offset Pointer to store the read 32-bit offset.
 * @return true on success, false on failure.
 */
bool read_offset_le(const uint8_t *src, size_t src_size, size_t offset_index, uint32_t *out_offset)
{
    if (!src || !out_offset || (offset_index + 3 >= src_size))
    {
        return false;
    }
    uint32_t val =
        (uint32_t)src[offset_index]       |
        ((uint32_t)src[offset_index + 1] << 8)  |
        ((uint32_t)src[offset_index + 2] << 16) |
        ((uint32_t)src[offset_index + 3] << 24);
    *out_offset = val;
    return true;
}
