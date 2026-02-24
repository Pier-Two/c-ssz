#include <stdint.h>

#include "Hacl_Hash_SHA2.h"

#include "ssz_hash.h"
#include "ssz_internal.h"

static ssz_error_t ssz_internal_normalize_hash_error(ssz_error_t err)
{
    if (err == SSZ_SUCCESS)
    {
        return SSZ_SUCCESS;
    }
    return SSZ_ERR_HASH_FAILURE;
}

ssz_error_t ssz_hash_sha256(const uint8_t *data, size_t data_len, uint8_t out[32])
{
    if (out == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((data == NULL) && (data_len != 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (data_len > UINT32_MAX)
    {
        return SSZ_ERR_OVERFLOW;
    }

    /* Provide a valid pointer for zero-length input to avoid passing NULL into
       HACL*, which triggers a pedantic UBSan finding (NULL + 0 is technically
       undefined in C even though no memory is accessed). */
    uint8_t empty = 0;
    const uint8_t *src = (data != NULL) ? data : &empty;
    Hacl_Hash_SHA2_hash_256(out, (uint8_t *)(uintptr_t)src, (uint32_t)data_len);
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_default_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    return ssz_hash_sha256(data, data_len, out);
}

static const ssz_hash_fn_t ssz_internal_default_hash_fn = {
    .hash = ssz_internal_default_hash,
    .hash_2to1 = NULL,
    .hash_2to1_batch = NULL,
    .ctx = NULL,
};

const ssz_hash_fn_t *ssz_hash_default(void)
{
    return &ssz_internal_default_hash_fn;
}

ssz_error_t ssz_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);

    if ((resolved == NULL) || (left == NULL) || (right == NULL) || (out == NULL) ||
        (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (resolved->hash_2to1 != NULL)
    {
        ssz_error_t err = resolved->hash_2to1(resolved->ctx, left, right, out);
        return ssz_internal_normalize_hash_error(err);
    }

    uint8_t pair_data[64];
    memcpy(pair_data, left->bytes, SSZ_BYTES_PER_CHUNK);
    memcpy(pair_data + SSZ_BYTES_PER_CHUNK, right->bytes, SSZ_BYTES_PER_CHUNK);

    ssz_error_t err = resolved->hash(resolved->ctx, pair_data, sizeof(pair_data), out->bytes);
    return ssz_internal_normalize_hash_error(err);
}

ssz_error_t ssz_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);

    if ((resolved == NULL) || (out == NULL) || (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((pair_count != 0u) && (pairs == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (resolved->hash_2to1_batch != NULL)
    {
        ssz_error_t err = resolved->hash_2to1_batch(resolved->ctx, pairs, pair_count, out);
        return ssz_internal_normalize_hash_error(err);
    }

    if (pair_count > (SIZE_MAX / 2u))
    {
        return SSZ_ERR_OVERFLOW;
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        ssz_error_t err = ssz_hash_2to1(resolved, &pairs[i * 2u], &pairs[i * 2u + 1u], &out[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}
