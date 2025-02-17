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
    for (size_t j = 0; j < len; ++j) 
    {
        if (ptr[j] != 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * Computes the next power of two for the given value.
 *
 * @param value The input value.
 * @return The next power of two for value, or value itself if it is already a power of two.
 */
uint64_t next_pow_of_two(uint64_t value)
{
    if (value == 0)
    {
        return 1;
    }
    if ((value & (value - 1)) == 0)
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
    uint64_t max_offset = 1ULL << (SSZ_BYTES_PER_LENGTH_OFFSET * SSZ_BITS_PER_BYTE);
    return offset < max_offset;
}
