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
    bool overflow = false;

    if (a > (SIZE_MAX - b))
    {
        overflow = true;
    }
    else if (out != NULL)
    {
        *out = a + b;
    }
    else
    {
        /* intentionally empty */
    }

    return overflow;
}

static inline bool ssz_internal_mul_overflow_size(size_t a, size_t b, size_t *out)
{
    bool overflow = false;

    if ((a != 0u) && (b > (SIZE_MAX / a)))
    {
        overflow = true;
    }
    else if (out != NULL)
    {
        *out = a * b;
    }
    else
    {
        /* intentionally empty */
    }

    return overflow;
}

static inline bool ssz_internal_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    bool overflow = false;

    if (a > (UINT64_MAX - b))
    {
        overflow = true;
    }
    else if (out != NULL)
    {
        *out = a + b;
    }
    else
    {
        /* intentionally empty */
    }

    return overflow;
}

static inline bool ssz_internal_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    bool overflow = false;

    if ((a != 0u) && (b > (UINT64_MAX / a)))
    {
        overflow = true;
    }
    else if (out != NULL)
    {
        *out = a * b;
    }
    else
    {
        /* intentionally empty */
    }

    return overflow;
}

static inline bool ssz_internal_u64_to_size(uint64_t value, size_t *out)
{
    bool converted = false;

    if (value <= (uint64_t)SIZE_MAX)
    {
        if (out != NULL)
        {
            *out = (size_t)value;
        }
        converted = true;
    }

    return converted;
}

static inline bool ssz_internal_bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    uint64_t bytes_u64 = 0u;
    bool converted = false;

    if (!ssz_internal_add_overflow_u64(bit_count, 7u, &bytes_u64))
    {
        bytes_u64 /= 8u;
        converted = ssz_internal_u64_to_size(bytes_u64, out_bytes);
    }

    return converted;
}

static inline bool ssz_internal_get_bit_le(const uint8_t *bits_le, uint64_t bit_index)
{
    size_t byte_index = (size_t)(bit_index / 8u);
    uint8_t bit_mask = (uint8_t)(1u << (bit_index % 8u));
    return (bits_le[byte_index] & bit_mask) != 0u;
}

static inline bool ssz_internal_pointer_is_aligned(const void *ptr, size_t alignment);
static inline ssz_error_t ssz_internal_validate_chunk_pointer(const ssz_chunk_t *chunk);
static inline ssz_error_t ssz_internal_validate_chunk_array(
    const ssz_chunk_t *chunks,
    size_t chunk_count);

static inline bool ssz_internal_pointer_is_aligned(const void *ptr, size_t alignment)
{
    bool aligned = false;

    if (ptr == NULL)
    {
        aligned = true;
    }
    else if (alignment == 0u)
    {
        aligned = false;
    }
    else
    {
        aligned = (((uintptr_t)ptr % (uintptr_t)alignment) == 0u);
    }

    return aligned;
}

static inline ssz_error_t ssz_internal_validate_chunk_pointer(const ssz_chunk_t *chunk)
{
    if (chunk == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!ssz_internal_pointer_is_aligned(chunk, SSZ_CHUNK_ALIGNMENT))
    {
        return SSZ_ERR_ALIGNMENT_INVALID;
    }
    return SSZ_SUCCESS;
}

static inline ssz_error_t ssz_internal_validate_chunk_array(
    const ssz_chunk_t *chunks,
    size_t chunk_count)
{
    if ((chunk_count != 0u) && (chunks == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((chunk_count != 0u) && !ssz_internal_pointer_is_aligned(chunks, SSZ_CHUNK_ALIGNMENT))
    {
        return SSZ_ERR_ALIGNMENT_INVALID;
    }
    return SSZ_SUCCESS;
}

static inline const ssz_hash_fn_t *ssz_internal_resolve_hash_fn(const ssz_hash_fn_t *hash_fn)
{
    const ssz_hash_fn_t *resolved_hash_fn = ssz_hash_default();

    if (hash_fn != NULL)
    {
        resolved_hash_fn = hash_fn;
    }

    return resolved_hash_fn;
}

static inline bool ssz_internal_selector_allowed(
    uint8_t selector,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count)
{
    bool allowed = false;

    for (uint32_t i = 0u; i < allowed_selector_count; i++)
    {
        if (allowed_selectors[i] == selector)
        {
            allowed = true;
            break;
        }
    }

    return allowed;
}

static inline ssz_error_t ssz_internal_validate_compatible_union_schema(
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((allowed_selectors == NULL) || (allowed_selector_count == 0u))
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        for (uint32_t i = 0u; i < allowed_selector_count; i++)
        {
            if ((allowed_selectors[i] == 0u) || (allowed_selectors[i] > 127u))
            {
                err = SSZ_ERR_SCHEMA_INVALID;
                break;
            }
        }
    }

    return err;
}

#ifdef __cplusplus
}
#endif

#endif
