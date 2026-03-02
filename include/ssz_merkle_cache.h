#ifndef SSZ_MERKLE_CACHE_H
#define SSZ_MERKLE_CACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ssz_merkle_cache ssz_merkle_cache_t;

typedef struct
{
    uint64_t initial_leaf_count;
    uint64_t leaf_limit;
    uint64_t logical_length;
    bool mix_in_length;
    const ssz_hash_fn_t *hash_fn;
} ssz_merkle_cache_config_t;

typedef ssz_error_t (*ssz_merkle_cache_token_fn_t)(
    const void *ctx,
    uint64_t member_id,
    uint64_t *out_token);

typedef ssz_error_t (*ssz_merkle_cache_root_batch_fn_t)(
    const void *ctx,
    uint64_t start_index,
    uint64_t count,
    ssz_chunk_t *out_roots);

typedef struct
{
    const void *ctx;
    ssz_merkle_cache_token_fn_t token;
    ssz_merkle_cache_root_batch_fn_t root_batch;
} ssz_merkle_cache_sync_composite_opts_t;

ssz_error_t ssz_merkle_cache_create(
    const ssz_merkle_cache_config_t *config,
    ssz_merkle_cache_t **out_cache);

void ssz_merkle_cache_destroy(ssz_merkle_cache_t *cache);

ssz_error_t ssz_merkle_cache_reset(ssz_merkle_cache_t *cache);

ssz_error_t ssz_merkle_cache_data_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root);

ssz_error_t ssz_merkle_cache_root(ssz_merkle_cache_t *cache, ssz_chunk_t *out_root);

ssz_error_t ssz_merkle_cache_update_root_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    const ssz_chunk_t *roots,
    uint64_t root_count);

ssz_error_t ssz_merkle_cache_zero_range(
    ssz_merkle_cache_t *cache,
    uint64_t start_index,
    uint64_t zero_count);

ssz_error_t ssz_merkle_cache_set_logical_length(
    ssz_merkle_cache_t *cache,
    uint64_t logical_length);

ssz_error_t ssz_merkle_cache_sync_packed_bytes(
    ssz_merkle_cache_t *cache,
    const uint8_t *bytes,
    size_t bytes_len,
    uint64_t logical_length);

ssz_error_t ssz_merkle_cache_sync_packed_vector_fixed(
    ssz_merkle_cache_t *cache,
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size);

ssz_error_t ssz_merkle_cache_sync_packed_list_fixed(
    ssz_merkle_cache_t *cache,
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size);

ssz_error_t ssz_merkle_cache_sync_bitvector(
    ssz_merkle_cache_t *cache,
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count);

ssz_error_t ssz_merkle_cache_sync_bitlist(
    ssz_merkle_cache_t *cache,
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit);

ssz_error_t ssz_merkle_cache_sync_composite(
    ssz_merkle_cache_t *cache,
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    const ssz_merkle_cache_sync_composite_opts_t *opts);

bool ssz_merkle_cache_needs_resync(const ssz_merkle_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif
