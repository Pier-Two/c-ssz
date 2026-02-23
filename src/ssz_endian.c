#include "ssz_internal.h"

void ssz_internal_write_u16_le(uint8_t out[2], uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

void ssz_internal_write_u32_le(uint8_t out[4], uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

void ssz_internal_write_u64_le(uint8_t out[8], uint64_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
    out[4] = (uint8_t)((value >> 32u) & 0xFFu);
    out[5] = (uint8_t)((value >> 40u) & 0xFFu);
    out[6] = (uint8_t)((value >> 48u) & 0xFFu);
    out[7] = (uint8_t)((value >> 56u) & 0xFFu);
}

uint16_t ssz_internal_read_u16_le(const uint8_t in[2])
{
    return (uint16_t)in[0] | (uint16_t)((uint16_t)in[1] << 8u);
}

uint32_t ssz_internal_read_u32_le(const uint8_t in[4])
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8u) | ((uint32_t)in[2] << 16u) |
           ((uint32_t)in[3] << 24u);
}

uint64_t ssz_internal_read_u64_le(const uint8_t in[8])
{
    return (uint64_t)in[0] | ((uint64_t)in[1] << 8u) | ((uint64_t)in[2] << 16u) |
           ((uint64_t)in[3] << 24u) | ((uint64_t)in[4] << 32u) | ((uint64_t)in[5] << 40u) |
           ((uint64_t)in[6] << 48u) | ((uint64_t)in[7] << 56u);
}
