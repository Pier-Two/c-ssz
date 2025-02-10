#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_utils.h"
#include "ssz_constants.h"

/**
 * Checks if the given memory region is filled with zeros.
 *
 * @param ptr Pointer to the memory region.
 * @param len The length of the memory region in bytes.
 * @return true if all bytes are zero, otherwise false.
 */
bool is_zero(const uint8_t *ptr, size_t len)
{
    size_t i = 0;
    while (len >= 8)
    {
        if (*(const uint64_t *)(ptr + i))
            return false;
        i += 8;
        len -= 8;
    }
    if (len >= 4)
    {
        if (*(const uint32_t *)(ptr + i))
            return false;
        i += 4;
        len -= 4;
    }
    if (len >= 2)
    {
        if (*(const uint16_t *)(ptr + i))
            return false;
        i += 2;
        len -= 2;
    }
    if (len >= 1)
    {
        if (*(ptr + i))
            return false;
    }
    return true;
}

/**
 * Determines whether a given value is a power of two.
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
 * @param value The input value.
 * @return The next power of two for value, or value itself if already a power of two.
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
 * @param offset The offset to validate.
 * @return true if offset is within the allowed range, false otherwise.
 */
bool check_max_offset(size_t offset)
{
    size_t max_offset = ((size_t)1 << (BYTES_PER_LENGTH_OFFSET * BITS_PER_BYTE));
    return (offset < max_offset);
}

