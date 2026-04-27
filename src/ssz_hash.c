#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <openssl/sha.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
#include <sanitizer/msan_interface.h>
#define SSZ_MSAN_UNPOISON(ptr, size) __msan_unpoison((ptr), (size))
#endif
#endif
#ifndef SSZ_MSAN_UNPOISON
#define SSZ_MSAN_UNPOISON(ptr, size) ((void)(ptr), (void)(size))
#endif

#include "ssz_hash.h"
#include "ssz_internal.h"

/* SHA-256 padding block for a 64-byte message (length = 512 bits). */
static const uint8_t ssz_internal_sha256_pad_block_for_64[64] = {
    0x80u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x02u, 0x00u,
};

static void ssz_internal_store_be32(uint8_t out[4], uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static void ssz_internal_sha256_ctx_to_chunk(const SHA256_CTX *ctx, ssz_chunk_t *out)
{
    for (size_t i = 0u; i < 8u; i++)
    {
        ssz_internal_store_be32(&out->bytes[i * 4u], ctx->h[i]);
    }
}

static ssz_error_t ssz_internal_sha256_64_batch_default(
    const uint8_t *pairs64,
    size_t pair_count,
    ssz_chunk_t *out)
{
    SHA256_CTX base;

    if (pair_count == 0u)
    {
        return SSZ_SUCCESS;
    }
    if ((pairs64 == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (ssz_internal_validate_chunk_pointer(out) != SSZ_SUCCESS)
    {
        return SSZ_ERR_ALIGNMENT_INVALID;
    }

    if (SHA256_Init(&base) != 1)
    {
        return SSZ_ERR_HASH_FAILURE;
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        SHA256_CTX ctx = base;
        const uint8_t *pair = pairs64 + (i * ((size_t)SSZ_BYTES_PER_CHUNK * 2u));

        SHA256_Transform(&ctx, pair);
        SHA256_Transform(&ctx, ssz_internal_sha256_pad_block_for_64);
        ssz_internal_sha256_ctx_to_chunk(&ctx, &out[i]);
        SSZ_MSAN_UNPOISON(&out[i], sizeof(out[i]));
    }

    return SSZ_SUCCESS;
}

static ssz_error_t ssz_internal_normalize_hash_error(ssz_error_t err)
{
    if (err == SSZ_SUCCESS)
    {
        return SSZ_SUCCESS;
    }
    return SSZ_ERR_HASH_FAILURE;
}

static uintptr_t ssz_internal_pointer_address(const void *ptr)
{
    uintptr_t address = 0u;
    const void *ptr_value = ptr;

    (void)memcpy(&address, (const void *)&ptr_value, sizeof(ptr_value));
    return address;
}

static bool ssz_internal_byte_ranges_overlap(
    const void *left,
    size_t left_len,
    const void *right,
    size_t right_len)
{
    bool overlap = false;

    if ((left_len != 0u) && (right_len != 0u))
    {
        uintptr_t left_begin = ssz_internal_pointer_address(left);
        uintptr_t right_begin = ssz_internal_pointer_address(right);

        if (((uintmax_t)left_len > (uintmax_t)(UINTPTR_MAX - left_begin)) ||
            ((uintmax_t)right_len > (uintmax_t)(UINTPTR_MAX - right_begin)))
        {
            overlap = true;
        }
        else
        {
            uintptr_t left_end = left_begin + (uintptr_t)left_len;
            uintptr_t right_end = right_begin + (uintptr_t)right_len;
            overlap = (left_begin < right_end) && (right_begin < left_end);
        }
    }

    return overlap;
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
    {
        uint8_t empty = 0u;
        const uint8_t *src = (data != NULL) ? data : &empty;
        SHA256(src, data_len, out);
        SSZ_MSAN_UNPOISON(out, SHA256_DIGEST_LENGTH);
    }

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

/* This shared table is reached through public Merkle APIs, so first-use
   initialization must be serialized across threads. */
static ssz_chunk_t ssz_internal_default_zero_hashes[64];

static void ssz_internal_init_default_zero_hashes(void)
{
    ssz_chunk_t computed[64];
    uint8_t pair[(size_t)SSZ_BYTES_PER_CHUNK * 2u];

    memset(computed[0].bytes, 0u, SSZ_BYTES_PER_CHUNK);
    for (size_t depth = 1u; depth < 64u; depth++)
    {
        memcpy(pair, computed[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair + SSZ_BYTES_PER_CHUNK, computed[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        ssz_hash_sha256(pair, sizeof(pair), computed[depth].bytes);
    }

    memcpy(ssz_internal_default_zero_hashes, computed, sizeof(computed));
}

#if defined(_WIN32)
static INIT_ONCE ssz_internal_default_zero_hashes_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK ssz_internal_init_default_zero_hashes_once(
    PINIT_ONCE init_once,
    PVOID parameter,
    PVOID *context)
{
    (void)init_once;
    (void)parameter;
    (void)context;

    ssz_internal_init_default_zero_hashes();
    return TRUE;
}
#else
static pthread_once_t ssz_internal_default_zero_hashes_once = PTHREAD_ONCE_INIT;
#endif

const ssz_hash_fn_t *ssz_hash_default(void)
{
    return &ssz_internal_default_hash_fn;
}

const ssz_chunk_t *ssz_hash_default_zero_hashes(void)
{
#if defined(_WIN32)
    if (InitOnceExecuteOnce(&ssz_internal_default_zero_hashes_once,
                            ssz_internal_init_default_zero_hashes_once,
                            NULL,
                            NULL) == 0)
    {
        return NULL;
    }
#else
    if (pthread_once(&ssz_internal_default_zero_hashes_once, ssz_internal_init_default_zero_hashes) !=
        0)
    {
        return NULL;
    }
#endif

    return ssz_internal_default_zero_hashes;
}

ssz_error_t ssz_hash_2to1_batch_raw(
    const ssz_hash_fn_t *hash_fn,
    const uint8_t *pairs64,
    size_t pair_count,
    ssz_chunk_t *out)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);

    if ((resolved == NULL) || (out == NULL) || (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (ssz_internal_validate_chunk_pointer(out) != SSZ_SUCCESS)
    {
        return SSZ_ERR_ALIGNMENT_INVALID;
    }
    if ((pair_count != 0u) && (pairs64 == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (pair_count > (SIZE_MAX / ((size_t)SSZ_BYTES_PER_CHUNK * 2u)))
    {
        return SSZ_ERR_OVERFLOW;
    }

    if (pair_count != 0u)
    {
        size_t pairs_bytes_len = 0u;
        size_t out_bytes_len = 0u;

        if (ssz_internal_mul_overflow_size(
                pair_count,
                (size_t)SSZ_BYTES_PER_CHUNK * 2u,
                &pairs_bytes_len) ||
            ssz_internal_mul_overflow_size(pair_count, SSZ_BYTES_PER_CHUNK, &out_bytes_len))
        {
            return SSZ_ERR_OVERFLOW;
        }

        if (ssz_internal_byte_ranges_overlap(pairs64, pairs_bytes_len, out, out_bytes_len))
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
    }

    if (resolved == &ssz_internal_default_hash_fn)
    {
        return ssz_internal_sha256_64_batch_default(pairs64, pair_count, out);
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        const uint8_t *pair = pairs64 + (i * ((size_t)SSZ_BYTES_PER_CHUNK * 2u));
        ssz_error_t err =
            resolved->hash(resolved->ctx, pair, (size_t)SSZ_BYTES_PER_CHUNK * 2u, out[i].bytes);
        if (err != SSZ_SUCCESS)
        {
            return ssz_internal_normalize_hash_error(err);
        }
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_2to1_batch_inplace(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *nodes,
    size_t pair_count)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);
    ssz_error_t err = SSZ_SUCCESS;

    if ((resolved == NULL) || (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (pair_count > (SIZE_MAX / 2u))
    {
        return SSZ_ERR_OVERFLOW;
    }
    err = ssz_internal_validate_chunk_array(nodes, pair_count * 2u);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (resolved == &ssz_internal_default_hash_fn)
    {
        return ssz_internal_sha256_64_batch_default((const uint8_t *)nodes, pair_count, nodes);
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        uint8_t pair_data[(size_t)SSZ_BYTES_PER_CHUNK * 2u];
        ssz_error_t err;

        memcpy(pair_data, nodes[i * 2u].bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair_data + SSZ_BYTES_PER_CHUNK, nodes[i * 2u + 1u].bytes, SSZ_BYTES_PER_CHUNK);
        err = resolved->hash(resolved->ctx, pair_data, sizeof(pair_data), nodes[i].bytes);
        if (err != SSZ_SUCCESS)
        {
            return ssz_internal_normalize_hash_error(err);
        }
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);
    ssz_error_t err = SSZ_SUCCESS;

    if ((resolved == NULL) || (left == NULL) || (right == NULL) || (out == NULL) ||
        (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    err = ssz_internal_validate_chunk_pointer(left);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    err = ssz_internal_validate_chunk_pointer(right);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    err = ssz_internal_validate_chunk_pointer(out);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (resolved->hash_2to1 != NULL)
    {
        err = resolved->hash_2to1(resolved->ctx, left, right, out);
        return ssz_internal_normalize_hash_error(err);
    }

    if (resolved == &ssz_internal_default_hash_fn)
    {
        const uint8_t *left_bytes = left->bytes;
        const uint8_t *right_bytes = right->bytes;

        if ((right_bytes == left_bytes + SSZ_BYTES_PER_CHUNK) &&
            !ssz_internal_byte_ranges_overlap(
                left_bytes,
                (size_t)SSZ_BYTES_PER_CHUNK * 2u,
                out->bytes,
                SSZ_BYTES_PER_CHUNK))
        {
            return ssz_internal_sha256_64_batch_default(left_bytes, 1u, out);
        }

        {
            uint8_t pair_data[(size_t)SSZ_BYTES_PER_CHUNK * 2u];
            memcpy(pair_data, left_bytes, SSZ_BYTES_PER_CHUNK);
            memcpy(pair_data + SSZ_BYTES_PER_CHUNK, right_bytes, SSZ_BYTES_PER_CHUNK);
            return ssz_internal_sha256_64_batch_default(pair_data, 1u, out);
        }
    }

    {
        uint8_t pair_data[(size_t)SSZ_BYTES_PER_CHUNK * 2u];
        ssz_error_t err;

        memcpy(pair_data, left->bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair_data + SSZ_BYTES_PER_CHUNK, right->bytes, SSZ_BYTES_PER_CHUNK);

        err = resolved->hash(resolved->ctx, pair_data, sizeof(pair_data), out->bytes);
        return ssz_internal_normalize_hash_error(err);
    }
}

ssz_error_t ssz_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    const ssz_hash_fn_t *resolved = ssz_internal_resolve_hash_fn(hash_fn);
    ssz_error_t err = SSZ_SUCCESS;

    if ((resolved == NULL) || (out == NULL) || (resolved->hash == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    err = ssz_internal_validate_chunk_array(out, pair_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    if (pair_count > (SIZE_MAX / 2u))
    {
        return SSZ_ERR_OVERFLOW;
    }
    err = ssz_internal_validate_chunk_array(pairs, pair_count * 2u);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (pair_count != 0u)
    {
        size_t pairs_bytes_len = 0u;
        size_t out_bytes_len = 0u;

        if (ssz_internal_mul_overflow_size(
                pair_count,
                (size_t)SSZ_BYTES_PER_CHUNK * 2u,
                &pairs_bytes_len) ||
            ssz_internal_mul_overflow_size(pair_count, SSZ_BYTES_PER_CHUNK, &out_bytes_len))
        {
            return SSZ_ERR_OVERFLOW;
        }

        if (ssz_internal_byte_ranges_overlap(pairs, pairs_bytes_len, out, out_bytes_len))
        {
            if (ssz_internal_pointer_address(pairs) == ssz_internal_pointer_address(out))
            {
                return ssz_hash_2to1_batch_inplace(resolved, out, pair_count);
            }
            return SSZ_ERR_INVALID_ARGUMENT;
        }
    }

    if (resolved->hash_2to1_batch != NULL)
    {
        err = resolved->hash_2to1_batch(resolved->ctx, pairs, pair_count, out);
        return ssz_internal_normalize_hash_error(err);
    }

    if ((resolved->hash_2to1 == NULL) && (pair_count != 0u))
    {
        return ssz_hash_2to1_batch_raw(resolved, (const uint8_t *)pairs, pair_count, out);
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        err = ssz_hash_2to1(resolved, &pairs[i * 2u], &pairs[i * 2u + 1u], &out[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    return SSZ_SUCCESS;
}
