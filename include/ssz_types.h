#ifndef SSZ_TYPES_H
#define SSZ_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SSZ_BYTES_PER_CHUNK         32u
#define SSZ_BYTES_PER_LENGTH_OFFSET 4u
#define SSZ_BITS_PER_BYTE           8u
#define SSZ_NO_LIMIT                UINT64_MAX

#if defined(_MSC_VER)
#define SSZ_PACKED_BEGIN __pragma(pack(push, 1))
#define SSZ_PACKED_END   __pragma(pack(pop))
#define SSZ_PACKED_ATTR
#define SSZ_ALIGNAS(n) __declspec(align(n))
#else
#define SSZ_PACKED_BEGIN
#define SSZ_PACKED_END
#define SSZ_PACKED_ATTR __attribute__((packed))
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

typedef uint64_t ssz_path_step_t;
#define SSZ_PATH_STEP_LENGTH UINT64_MAX

const char *ssz_error_string(ssz_error_t error);

#ifdef __cplusplus
}
#endif

#endif
