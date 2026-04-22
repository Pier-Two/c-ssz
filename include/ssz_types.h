#ifndef SSZ_TYPES_H
#define SSZ_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SSZ_BYTES_PER_CHUNK         32u
#define SSZ_BYTES_PER_LENGTH_OFFSET 4u
#define SSZ_BITS_PER_BYTE           8u
#define SSZ_NO_LIMIT                UINT64_MAX

#if defined(_MSC_VER)
#define SSZ_ALIGNAS(n) __declspec(align(n))
#else
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SSZ_ALIGNAS(n) _Alignas(n)
#else
#define SSZ_ALIGNAS(n) __attribute__((aligned(n)))
#endif
#endif

typedef struct
{
    SSZ_ALIGNAS(32) uint8_t bytes[32];
} ssz_chunk_t;

typedef uint64_t ssz_gindex_t;

typedef struct ssz_gindex_type ssz_gindex_type_t;

typedef enum
{
    SSZ_SUCCESS = 0,
    SSZ_ERR_INVALID_ARGUMENT,
    SSZ_ERR_BUFFER_TOO_SMALL,
    SSZ_ERR_OVERFLOW,
    SSZ_ERR_LIMIT_EXCEEDED,
    SSZ_ERR_SCHEMA_INVALID,
    SSZ_ERR_ENCODING_INVALID,
    SSZ_ERR_OFFSET_INVALID,
    SSZ_ERR_TYPE_MISMATCH,
    SSZ_ERR_SELECTOR_INVALID,
    SSZ_ERR_GINDEX_INVALID,
    SSZ_ERR_PROOF_INVALID,
    SSZ_ERR_HASH_FAILURE
} ssz_error_t;

typedef enum
{
    SSZ_GINDEX_LEAF,
    SSZ_GINDEX_ELEMENTS,
    SSZ_GINDEX_CONTAINER
} ssz_gindex_kind_t;

struct ssz_gindex_type
{
    ssz_gindex_kind_t kind;
    uint64_t chunk_count;
    uint8_t item_length;
    bool has_mix_in_length;
    const ssz_gindex_type_t *elem_type;
    const ssz_gindex_type_t *const *field_types;
    uint64_t element_count_or_limit;
};

typedef struct
{
    ssz_error_t (*hash)(const void *ctx, const uint8_t *data, size_t data_len, uint8_t out[32]);
    ssz_error_t (*hash_2to1)(
        const void *ctx,
        const ssz_chunk_t *left,
        const ssz_chunk_t *right,
        ssz_chunk_t *out);
    ssz_error_t (*hash_2to1_batch)(
        const void *ctx,
        const ssz_chunk_t *pairs,
        size_t pair_count,
        ssz_chunk_t *out);
    const void *ctx;
} ssz_hash_fn_t;

typedef struct
{
    void *ctx;
    ssz_error_t (*write)(
        const void *ctx,
        uint64_t member_id,
        uint8_t *out,
        size_t out_cap,
        size_t *out_written);
    ssz_error_t (*read)(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len);
    ssz_error_t (*root)(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root);
} ssz_member_codec_t;

/*
 * Container schema metadata.
 *
 * `field_fixed_sizes` must point to storage that remains valid and immutable
 * for the duration of any operation that uses the schema. Recommended usage is
 * `static const` storage.
 */
typedef struct
{
    const size_t *field_fixed_sizes;
    uint32_t field_count;
} ssz_container_schema_t;

#define SSZ_CONTAINER_SCHEMA_FROM_ARRAY(field_fixed_sizes_array)                                   \
    {                                                                                              \
        .field_fixed_sizes = (field_fixed_sizes_array),                                            \
        .field_count = (uint32_t)(sizeof(field_fixed_sizes_array) /                                \
                                  sizeof((field_fixed_sizes_array)[0]))                            \
    }

typedef uint64_t ssz_path_step_t;
#define SSZ_PATH_STEP_LENGTH UINT64_MAX

static inline ssz_container_schema_t ssz_container_schema_init(
    const size_t *field_fixed_sizes,
    uint32_t field_count)
{
    ssz_container_schema_t schema = {
        .field_fixed_sizes = field_fixed_sizes,
        .field_count = field_count,
    };

    return schema;
}

static inline bool ssz_types_internal_mul_overflow_size(size_t a, size_t b, size_t *out)
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

static inline bool ssz_types_internal_u64_to_size(uint64_t value, size_t *out)
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

static inline bool ssz_types_internal_bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    bool converted = false;

    if (bit_count <= (UINT64_MAX - 7u))
    {
        uint64_t bytes_u64 = (bit_count + 7u) / 8u;
        converted = ssz_types_internal_u64_to_size(bytes_u64, out_bytes);
    }

    return converted;
}

static inline bool ssz_types_internal_bytes_are_zero(const uint8_t *bytes, size_t byte_count)
{
    bool are_zero = true;

    for (size_t i = 0u; i < byte_count; i++)
    {
        if (bytes[i] != 0u)
        {
            are_zero = false;
            break;
        }
    }

    return are_zero;
}

static inline ssz_error_t ssz_default_uint8(uint8_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_uint16(uint16_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_uint32(uint32_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_uint64(uint64_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_uint128(uint8_t out_value[16])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_value, 0, 16u);
    }

    return err;
}

static inline ssz_error_t ssz_default_uint256(uint8_t out_value[32])
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_value, 0, 32u);
    }

    return err;
}

static inline ssz_error_t ssz_default_boolean(uint8_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_value = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_bitvector(
    uint8_t *out_bits_le,
    size_t out_bits_le_len,
    uint64_t bit_count)
{
    size_t byte_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (bit_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_types_internal_bits_to_bytes(bit_count, &byte_count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((byte_count != 0u) && ((out_bits_le == NULL) || (out_bits_le_len < byte_count)))
    {
        err = SSZ_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        if (byte_count != 0u)
        {
            (void)memset(out_bits_le, 0, byte_count);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static inline ssz_error_t ssz_default_bitlist(uint64_t *out_bit_len)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_bit_len == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_bit_len = 0u;
    }

    return err;
}

static inline ssz_error_t ssz_default_vector_fixed(
    uint8_t *out_elements,
    uint64_t element_count,
    size_t element_size)
{
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_types_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_types_internal_mul_overflow_size((size_t)element_count, element_size, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((required != 0u) && (out_elements == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (required != 0u)
        {
            (void)memset(out_elements, 0, required);
        }
        else
        {
            /* intentionally empty */
        }
    }

    return err;
}

static inline ssz_error_t ssz_default_vector_composite(uint64_t element_count, ssz_member_codec_t *codec)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if ((codec == NULL) || (codec->read == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (uint64_t i = 0u; i < element_count; i++)
        {
            err = codec->read(codec->ctx, i, NULL, 0u);
            if (err != SSZ_SUCCESS)
            {
                break;
            }
        }
    }

    return err;
}

static inline ssz_error_t ssz_default_list(uint64_t *out_element_count)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_element_count == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_element_count = 0u;
    }

    return err;
}

ssz_error_t ssz_default_container(const ssz_container_schema_t *schema, ssz_member_codec_t *codec);

ssz_error_t ssz_default_union(
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *out_selector);

static inline ssz_error_t ssz_is_zero_uint8(uint8_t value, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (value == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_uint16(uint16_t value, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (value == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_uint32(uint32_t value, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (value == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_uint64(uint64_t value, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (value == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_uint128(const uint8_t value[16], bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out_is_zero == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = ssz_types_internal_bytes_are_zero(value, 16u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_uint256(const uint8_t value[32], bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((value == NULL) || (out_is_zero == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = ssz_types_internal_bytes_are_zero(value, 32u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_boolean(uint8_t value, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (value == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    bool *out_is_zero)
{
    size_t byte_count = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (bit_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_types_internal_bits_to_bytes(bit_count, &byte_count))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((byte_count != 0u) && ((bits_le == NULL) || (bits_le_len < byte_count)))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ((bit_count % 8u) != 0u)
        {
            uint8_t mask = (uint8_t)((1u << (bit_count % 8u)) - 1u);
            if ((bits_le[byte_count - 1u] & (uint8_t)(~mask)) != 0u)
            {
                err = SSZ_ERR_ENCODING_INVALID;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            *out_is_zero = (byte_count == 0u) || ssz_types_internal_bytes_are_zero(bits_le, byte_count);
        }
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_bitlist(uint64_t bit_len, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (bit_len == 0u);
    }

    return err;
}

static inline ssz_error_t ssz_is_zero_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    bool *out_is_zero)
{
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (element_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (element_size == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else if (!ssz_types_internal_u64_to_size(element_count, NULL))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if (ssz_types_internal_mul_overflow_size((size_t)element_count, element_size, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else if ((required != 0u) && (elements == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (required == 0u) || ssz_types_internal_bytes_are_zero(elements, required);
    }

    return err;
}

/* Composite zero checks split scratch into two equal halves for current/default snapshots. */
ssz_error_t ssz_is_zero_vector_composite(
    uint64_t element_count,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero);

static inline ssz_error_t ssz_is_zero_list(uint64_t element_count, bool *out_is_zero)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (out_is_zero == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_is_zero = (element_count == 0u);
    }

    return err;
}

ssz_error_t ssz_is_zero_container(
    const ssz_container_schema_t *schema,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero);

ssz_error_t ssz_is_zero_union(
    uint8_t selector,
    uint32_t option_count,
    bool has_none,
    ssz_member_codec_t *codec,
    uint8_t *scratch,
    size_t scratch_len,
    bool *out_is_zero);

const char *ssz_error_string(ssz_error_t error);

#ifdef __cplusplus
}
#endif

#endif
