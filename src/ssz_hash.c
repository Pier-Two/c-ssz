#include <stdint.h>

#include <openssl/sha.h>

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
       SHA256, which triggers a pedantic UBSan finding (NULL + 0 is technically
       undefined in C even though no memory is accessed). */
    uint8_t empty = 0;
    const uint8_t *src = (data != NULL) ? data : &empty;
    SHA256(src, data_len, out);
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

    if (resolved == &ssz_internal_default_hash_fn)
    {
        const uint8_t *left_bytes = left->bytes;
        const uint8_t *right_bytes = right->bytes;
        uint8_t *out_bytes = out->bytes;

        if ((right_bytes == left_bytes + SSZ_BYTES_PER_CHUNK) &&
            ((out_bytes + SSZ_BYTES_PER_CHUNK <= left_bytes) ||
             (out_bytes >= left_bytes + (SSZ_BYTES_PER_CHUNK * 2u))))
        {
            SHA256(left_bytes, SSZ_BYTES_PER_CHUNK * 2u, out_bytes);
            return SSZ_SUCCESS;
        }

        uint8_t pair_data[64];
        memcpy(pair_data, left_bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair_data + SSZ_BYTES_PER_CHUNK, right_bytes, SSZ_BYTES_PER_CHUNK);
        SHA256(pair_data, sizeof(pair_data), out_bytes);
        return SSZ_SUCCESS;
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

    if ((resolved->hash_2to1 == NULL) && (pair_count != 0u))
    {
        bool use_default_sha256 = (resolved == &ssz_internal_default_hash_fn);
        size_t pairs_bytes_len = 0u;
        size_t out_bytes_len = 0u;
        bool overlap = true;

        if (!ssz_internal_mul_overflow_size(pair_count, SSZ_BYTES_PER_CHUNK * 2u, &pairs_bytes_len) &&
            !ssz_internal_mul_overflow_size(pair_count, SSZ_BYTES_PER_CHUNK, &out_bytes_len))
        {
            const uint8_t *pairs_begin = (const uint8_t *)pairs;
            const uint8_t *pairs_end = pairs_begin + pairs_bytes_len;
            uint8_t *out_begin = (uint8_t *)out;
            uint8_t *out_end = out_begin + out_bytes_len;
            overlap = (out_begin < pairs_end) && (pairs_begin < out_end);
        }

        if (!overlap)
        {
            if (use_default_sha256)
            {
                for (size_t i = 0u; i < pair_count; i++)
                {
                    const uint8_t *pair_bytes = (const uint8_t *)&pairs[i * 2u];
                    SHA256(pair_bytes, SSZ_BYTES_PER_CHUNK * 2u, out[i].bytes);
                }
            }
            else
            {
                for (size_t i = 0u; i < pair_count; i++)
                {
                    const uint8_t *pair_bytes = (const uint8_t *)&pairs[i * 2u];
                    ssz_error_t err =
                        resolved->hash(resolved->ctx, pair_bytes, SSZ_BYTES_PER_CHUNK * 2u, out[i].bytes);
                    if (err != SSZ_SUCCESS)
                    {
                        return ssz_internal_normalize_hash_error(err);
                    }
                }
            }
            return SSZ_SUCCESS;
        }
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
