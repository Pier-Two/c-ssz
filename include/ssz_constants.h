#ifndef SSZ_CONSTANTS_H
#define SSZ_CONSTANTS_H

#include <stdint.h>

#define SSZ_BYTES_PER_CHUNK         32
#define SSZ_BYTES_PER_LENGTH_OFFSET 4
#define SSZ_BITS_PER_BYTE           8
#define SSZ_BYTE_SIZE_OF_UINT8      1
#define SSZ_BYTE_SIZE_OF_UINT16     2
#define SSZ_BYTE_SIZE_OF_UINT32     4
#define SSZ_BYTE_SIZE_OF_UINT64     8
#define SSZ_BYTE_SIZE_OF_UINT128    16
#define SSZ_BYTE_SIZE_OF_UINT256    32
#define SSZ_BYTE_SIZE_OF_BOOL       1

/**
 * Provides a lookup table to find the highest set bit for each byte value (0-255).
 * Each index maps to the highest set bit in the corresponding byte, or -1 if the byte is zero.
 */
extern const int8_t highest_bit_table[256];

#endif /* SSZ_CONSTANTS_H */