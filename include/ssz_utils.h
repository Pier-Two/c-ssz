#ifndef SSZ_UTILS_H
#define SSZ_UTILS_H

#include <stdint.h>
#include <stdbool.h>

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
 * Writes a 4-byte little-endian offset into the output buffer.
 * The 32-bit offset is split into four bytes and written in little-endian order.
 * 
 * @param offset The 32-bit offset to write.
 * @param out Pointer to the output buffer.
 */
void write_offset_le(uint32_t offset, uint8_t *out);

/**
 * Reads a 4-byte little-endian offset from the source buffer at the specified index.
 * 
 * @param src Pointer to the source buffer.
 * @param src_size The size of the source buffer in bytes.
 * @param offset_index The index in the buffer to read the offset from.
 * @param out_offset Pointer to store the read 32-bit offset.
 * @return true on success, false on failure (e.g., insufficient buffer size or invalid pointers).
 */
bool read_offset_le(const uint8_t *src, size_t src_size, size_t offset_index, uint32_t *out_offset);

#endif /* SSZ_UTILS_H */
