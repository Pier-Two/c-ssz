#ifndef SSZ_HASH_H
#define SSZ_HASH_H

#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

ssz_error_t ssz_hash_sha256(const uint8_t *data, size_t data_len, uint8_t out[32]);

ssz_error_t ssz_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out);

const ssz_hash_fn_t *ssz_hash_default(void);

ssz_error_t ssz_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out);

ssz_error_t ssz_hash_2to1_batch_raw(
    const ssz_hash_fn_t *hash_fn,
    const uint8_t *pairs64,
    size_t pair_count,
    ssz_chunk_t *out);

ssz_error_t ssz_hash_2to1_batch_inplace(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *nodes,
    size_t pair_count);

const ssz_chunk_t *ssz_hash_default_zero_hashes(void);

#ifdef __cplusplus
}
#endif

#endif
