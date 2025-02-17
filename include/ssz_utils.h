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
bool is_zero(const uint8_t *ptr, size_t len);

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
 * Validates whether the provided offset is within the maximum allowed SSZ offset range.
 * This check is based on SSZ's fixed offset size of SSZ_BYTES_PER_LENGTH_OFFSET bytes.
 *
 * @param offset The offset to validate.
 * @return true if the offset is valid, false otherwise.
 */
bool check_max_offset(size_t offset);

#endif /* SSZ_UTILS_H */
