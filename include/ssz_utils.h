#ifndef SSZ_UTILS_H
#define SSZ_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Checks if the given memory region is filled with zeros.
 * @param ptr Pointer to the memory region.
 * @param len The length of the memory region in bytes.
 * @return true if all bytes are zero, otherwise false.
 */
bool is_all_zero(const uint8_t *ptr, size_t len);

/**
 * Computes the next power of two for the given value.
 * If the input is already a power of two, the same value is returned.
 * If the input is zero, the result is 1.
 *
 * @param value The input value.
 * @return The next power of two.
 */
uint64_t next_pow_of_two(uint64_t value);

/**
 * Determines whether a given value is a power of two.
 *
 * @param value The input value.
 * @return true if the value is a power of two, false otherwise.
 */
bool is_power_of_two(uint64_t value);

/**
 * Validates whether the provided offset is within the maximum allowed SSZ offset range.
 * This check is based on SSZ's fixed offset size of BYTES_PER_LENGTH_OFFSET bytes.
 *
 * @param offset The offset to validate.
 * @return true if the offset is valid, false otherwise.
 */
bool check_max_offset(size_t offset);

/**
 * Computes the number of chunks required for a basic SSZ type.
 *
 * @return The number of chunks.
 */
size_t chunk_count_basic(void);

/**
 * Computes the number of chunks required for a bitlist with a given maximum number of bits.
 *
 * @param max_bits The maximum number of bits in the bitlist.
 * @return The number of chunks required.
 */
size_t chunk_count_bitlist(size_t max_bits);

/**
 * Computes the number of chunks required for a bitvector with a given number of bits.
 *
 * @param num_bits The number of bits in the bitvector.
 * @return The number of chunks required.
 */
size_t chunk_count_bitvector(size_t num_bits);

/**
 * Computes the number of chunks required for a list of basic SSZ elements.
 *
 * @param max_elements The maximum number of elements in the list.
 * @param basic_type_size The size of each basic type element in bytes.
 * @return The number of chunks required.
 */
size_t chunk_count_list_basic(size_t max_elements, size_t basic_type_size);

/**
 * Computes the number of chunks required for a vector of basic SSZ elements.
 *
 * @param num_elements The number of elements in the vector.
 * @param basic_type_size The size of each basic type element in bytes.
 * @return The number of chunks required.
 */
size_t chunk_count_vector_basic(size_t num_elements, size_t basic_type_size);

#endif /* SSZ_UTILS_H */
