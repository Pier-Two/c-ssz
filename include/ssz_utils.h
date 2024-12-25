#ifndef SSZ_UTILS_H
#define SSZ_UTILS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Returns the next power of two of the given value.
 * If the input is already a power of two, it returns the same value.
 * If the input is zero, it returns 1.
 * This is required for SSZ merkleization steps.
 */
uint64_t next_pow_of_two(uint64_t value);

/**
 * Checks if a given value is a power of two.
 * This function is mainly useful in validating merkleization parameters.
 */
bool is_power_of_two(uint64_t value);

#endif /* SSZ_UTILS_H */
