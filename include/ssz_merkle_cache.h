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

    typedef struct
    {
        size_t struct_size;
        uint64_t initial_leaf_count;
        uint64_t leaf_limit;
        uint64_t reserved_leaf_capacity;
        uint64_t logical_length;
        bool mix_in_length;
        const ssz_hash_fn_t *hash_fn;
    } ssz_merkle_cache_config_t;

    typedef struct
    {
        size_t struct_size;
        uint64_t physical_leaf_capacity;
        uint64_t mutable_leaf_capacity;
        uint32_t depth;
        size_t nodes_count;
        size_t leaf_dirty_words;
        size_t parent_dirty_words;
        size_t gather_pair_capacity;
        size_t gather_pairs_count;
        size_t gather_hashes_count;
        size_t gather_parent_indices_count;
        size_t token_values_count;
        size_t token_valid_words;
        size_t root_batch_roots_count;
    } ssz_merkle_cache_requirements_t;

    typedef struct
    {
        size_t struct_size;
        ssz_chunk_t *nodes;
        size_t nodes_count;
        uint64_t *leaf_dirty_bits;
        size_t leaf_dirty_words;
        size_t *leaf_dirty_word_idx;
        size_t leaf_dirty_word_idx_count;
        uint64_t *parent_dirty_bits[2];
        size_t parent_dirty_words;
        size_t *parent_dirty_word_idx[2];
        size_t parent_dirty_word_idx_count;
        ssz_chunk_t *gather_pairs;
        size_t gather_pairs_count;
        ssz_chunk_t *gather_hashes;
        size_t gather_hashes_count;
        size_t *gather_parent_indices;
        size_t gather_parent_indices_count;
        uint64_t *token_values;
        size_t token_values_count;
        uint64_t *token_valid_bits;
        size_t token_valid_words;
    } ssz_merkle_cache_storage_t;

    typedef struct
    {
        size_t struct_size;
        ssz_chunk_t *root_batch_roots;
        size_t root_batch_roots_count;
    } ssz_merkle_cache_sync_workspace_t;

    typedef ssz_error_t (
        *ssz_merkle_cache_token_fn_t)(const void *ctx, uint64_t member_id, uint64_t *out_token);

    typedef ssz_error_t (*ssz_merkle_cache_root_batch_fn_t)(
        const void *ctx,
        uint64_t start_index,
        uint64_t count,
        ssz_chunk_t *out_roots);

    typedef struct
    {
        size_t struct_size;
        const void *ctx;
        ssz_merkle_cache_token_fn_t token;
        ssz_merkle_cache_root_batch_fn_t root_batch;
        ssz_merkle_cache_sync_workspace_t *workspace;
    } ssz_merkle_cache_sync_composite_opts_t;

    typedef struct
    {
        size_t struct_size;
        const ssz_hash_fn_t *hash_fn;
        const ssz_chunk_t *zero_hashes;
        ssz_chunk_t zero_hashes_buf[64];
        ssz_chunk_t *nodes;
        size_t level_offsets[64];
        uint64_t leaf_capacity;
        uint64_t mutable_leaf_capacity;
        uint32_t depth;
        uint64_t leaf_limit;
        uint64_t leaf_count;
        uint64_t logical_length;
        bool mix_in_length;
        uint64_t *leaf_dirty_bits;
        size_t *leaf_dirty_word_idx;
        size_t leaf_dirty_word_count;
        size_t leaf_dirty_word_capacity;
        uint64_t *parent_dirty_bits[2];
        size_t *parent_dirty_word_idx[2];
        size_t parent_dirty_word_count[2];
        size_t parent_dirty_word_capacity;
        ssz_chunk_t *gather_pairs;
        ssz_chunk_t *gather_hashes;
        size_t *gather_parent_indices;
        size_t gather_pair_capacity;
        uint64_t *token_values;
        uint64_t *token_valid_bits;
        size_t token_capacity;
        size_t token_word_capacity;
        bool token_storage_ready;
        bool use_gather_inplace;
        bool data_root_valid;
        bool final_root_valid;
        bool needs_resync;
        ssz_chunk_t cached_data_root;
        ssz_chunk_t cached_root;
    } ssz_merkle_cache_t;

    ssz_error_t ssz_merkle_cache_requirements(
        const ssz_merkle_cache_config_t *config,
        ssz_merkle_cache_requirements_t *out_requirements);

    ssz_error_t ssz_merkle_cache_bind(
        const ssz_merkle_cache_config_t *config,
        const ssz_merkle_cache_storage_t *storage,
        ssz_merkle_cache_t *out_cache);

    ssz_error_t ssz_merkle_cache_migrate_into(
        const ssz_merkle_cache_t *src,
        const ssz_merkle_cache_config_t *dst_config,
        const ssz_merkle_cache_storage_t *dst_storage,
        ssz_merkle_cache_t *out_cache);

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
