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
bool is_all_zero(const uint8_t *ptr, size_t len)
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


/**
 * Returns the number of chunks required for a basic type.
 *
 * @return The number of chunks required for a basic type.
 */
size_t chunk_count_basic(void)
{
    return 1;
}

/**
 * Computes the number of chunks required to store a bitlist.
 *
 * @param max_bits The maximum number of bits in the bitlist.
 * @return The number of chunks required.
 */
size_t chunk_count_bitlist(size_t max_bits)
{
    return (max_bits + 255) / 256;
}

/**
 * Computes the number of chunks required to store a bitvector.
 *
 * @param num_bits The number of bits in the bitvector.
 * @return The number of chunks required.
 */
size_t chunk_count_bitvector(size_t num_bits)
{
    return (num_bits + 255) / 256;
}

/**
 * Computes the number of chunks required to store a list of basic types.
 *
 * @param max_elements The maximum number of elements in the list.
 * @param basic_type_size The size of each element in bytes.
 * @return The number of chunks required.
 */
size_t chunk_count_list_basic(size_t max_elements, size_t basic_type_size)
{
    return (max_elements * basic_type_size + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK;
}

/**
 * Computes the number of chunks required to store a vector of basic types.
 *
 * @param num_elements The number of elements in the vector.
 * @param basic_type_size The size of each element in bytes.
 * @return The number of chunks required.
 */
size_t chunk_count_vector_basic(size_t num_elements, size_t basic_type_size)
{
    return (num_elements * basic_type_size + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK;
}
