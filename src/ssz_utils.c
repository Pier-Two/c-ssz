#include <stddef.h>
#include <string.h>
#include "../include/ssz_utils.h"
#include "../include/ssz_constants.h"

/**
 * Returns true if the given value is a power of two, and false otherwise.
 * A neat trick is to use (x & (x-1)) == 0 if and only if x is a power of two (and x is not zero).
 * We define zero as not a power of two for this purpose. 
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
 * Returns the next power of two for the given value. If the value is already a power of two,
 * it returns the same value. If the value is zero, we define it to return one.
 * Otherwise, we repeatedly shift until we've passed the highest set bit in 'value'.
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
 * Checks whether the provided offset is within the maximum allowed SSZ offset range.
 * This is determined by BYTES_PER_LENGTH_OFFSET, which for SSZ is typically 4 bytes.
 * We compute max_offset as (1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE)).
 * Returns true if offset < max_offset, false otherwise.
 */
bool check_max_offset(size_t offset)
{
    size_t max_offset = ((size_t)1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE));
    return (offset < max_offset);
}

/**
 * Writes a 4-byte little-endian representation of the given 32-bit offset into 'out'.
 * Each byte is shifted appropriately: lowest-order byte first, highest-order byte last.
 * This is used in various serialization routines to place offset markers in a buffer.
 */
void write_offset_le(uint32_t offset, uint8_t *out)
{
    out[0] = (uint8_t)(offset & 0xFF);
    out[1] = (uint8_t)((offset >> 8) & 0xFF);
    out[2] = (uint8_t)((offset >> 16) & 0xFF);
    out[3] = (uint8_t)((offset >> 24) & 0xFF);
}

/**
 * Reads a 4-byte little-endian offset from src, starting at 'offset_index'.
 * If there's insufficient space or if pointers are invalid, we return false.
 * Otherwise, out_offset is set to the 32-bit offset value read from the buffer.
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
