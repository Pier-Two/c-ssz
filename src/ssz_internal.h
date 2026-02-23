#ifndef SSZ_INTERNAL_H
#define SSZ_INTERNAL_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ssz_hash.h"
#include "ssz_proof.h"
#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

void ssz_internal_write_u16_le(uint8_t out[2], uint16_t value);
void ssz_internal_write_u32_le(uint8_t out[4], uint32_t value);
void ssz_internal_write_u64_le(uint8_t out[8], uint64_t value);

uint16_t ssz_internal_read_u16_le(const uint8_t in[2]);
uint32_t ssz_internal_read_u32_le(const uint8_t in[4]);
uint64_t ssz_internal_read_u64_le(const uint8_t in[8]);

static inline bool ssz_internal_add_overflow_size(size_t a, size_t b, size_t *out)
{
    if (a > (SIZE_MAX - b))
    {
        return true;
    }
    if (out != NULL)
    {
        *out = a + b;
    }
    return false;
}

static inline bool ssz_internal_mul_overflow_size(size_t a, size_t b, size_t *out)
{
    if ((a != 0u) && (b > (SIZE_MAX / a)))
    {
        return true;
    }
    if (out != NULL)
    {
        *out = a * b;
    }
    return false;
}

static inline bool ssz_internal_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (a > (UINT64_MAX - b))
    {
        return true;
    }
    if (out != NULL)
    {
        *out = a + b;
    }
    return false;
}

static inline bool ssz_internal_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if ((a != 0u) && (b > (UINT64_MAX / a)))
    {
        return true;
    }
    if (out != NULL)
    {
        *out = a * b;
    }
    return false;
}

static inline bool ssz_internal_u64_to_size(uint64_t value, size_t *out)
{
    if (value > (uint64_t)SIZE_MAX)
    {
        return false;
    }
    if (out != NULL)
    {
        *out = (size_t)value;
    }
    return true;
}

static inline bool ssz_internal_bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    uint64_t bytes_u64 = 0u;
    if (ssz_internal_add_overflow_u64(bit_count, 7u, &bytes_u64))
    {
        return false;
    }
    bytes_u64 /= 8u;
    return ssz_internal_u64_to_size(bytes_u64, out_bytes);
}

static inline size_t ssz_internal_count_bits_u8(uint8_t value)
{
    size_t count = 0u;
    while (value != 0u)
    {
        count += (size_t)(value & 1u);
        value >>= 1u;
    }
    return count;
}

static inline bool ssz_internal_get_bit_le(const uint8_t *bits_le, uint64_t bit_index)
{
    size_t byte_index = (size_t)(bit_index / 8u);
    uint8_t bit_mask = (uint8_t)(1u << (bit_index % 8u));
    return (bits_le[byte_index] & bit_mask) != 0u;
}

static inline const ssz_hash_fn_t *ssz_internal_resolve_hash_fn(const ssz_hash_fn_t *hash_fn)
{
    if (hash_fn != NULL)
    {
        return hash_fn;
    }
    return ssz_hash_default();
}

#ifdef __cplusplus
}
#endif

#endif
