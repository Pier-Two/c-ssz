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

/**
 * Checks whether the provided offset is within the maximum allowed SSZ offset range.
 * This relies on SSZ's fixed offset size of BYTES_PER_LENGTH_OFFSET bytes. Returns true if valid, false otherwise.
 */
bool check_max_offset(size_t offset);

/**
 * Writes a 4-byte little-endian offset into the output buffer.
 * This takes a 32-bit offset and splits it into four bytes, each shifted accordingly.
 */
void write_offset_le(uint32_t offset, uint8_t *out);

/**
 * Reads a 4-byte little-endian offset from the source buffer at the given offset index.
 * Returns false on failure (e.g., if there's not enough room or pointers are invalid).
 */
bool read_offset_le(const uint8_t *src, size_t src_size, size_t offset_index, uint32_t *out_offset);

#endif /* SSZ_UTILS_H */
