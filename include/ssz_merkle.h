#ifndef SSZ_MERKLE_H
#define SSZ_MERKLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SSZ_MERKLE_SCRATCH_MAX_CHUNKS 131072u

typedef struct
{
    ssz_chunk_t *chunks;
    size_t chunk_count;
} ssz_merkle_scratch_t;

ssz_error_t ssz_hash_tree_root_uint8(uint8_t value, ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_uint16(uint16_t value, ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_uint32(uint32_t value, ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_uint64(uint64_t value, ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_uint128(const uint8_t value[16], ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_uint256(const uint8_t value[32], ssz_chunk_t *out_root);
ssz_error_t ssz_hash_tree_root_boolean(uint8_t value, ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_vector_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_vector_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_list_composite(
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    uint64_t limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_union(
    uint8_t selector,
    bool has_none,
    const ssz_member_codec_t *codec,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_merkleize(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    uint64_t limit,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_mix_in_length(
    const ssz_chunk_t *root,
    const uint8_t length[32],
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_mix_in_length_u64(
    const ssz_chunk_t *root,
    uint64_t length,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_mix_in_selector(
    const ssz_chunk_t *root,
    uint8_t selector,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_merkleize_progressive(
    const ssz_chunk_t *chunks,
    size_t chunk_count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_mix_in_active_fields(
    const ssz_chunk_t *root,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_container(
    uint32_t field_count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_list_composite(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_container_roots(
    const ssz_chunk_t *roots,
    uint32_t count,
    const uint8_t *active_fields,
    size_t active_fields_len,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_hash_tree_root_progressive_list_roots(
    const ssz_chunk_t *roots,
    uint64_t count,
    const ssz_merkle_scratch_t *scratch,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

#ifdef __cplusplus
}
#endif

#endif
