#ifndef SSZ_PROOF_H
#define SSZ_PROOF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

ssz_error_t ssz_get_generalized_index(
    const ssz_gindex_type_t *type,
    const ssz_path_step_t *path,
    size_t path_len,
    ssz_gindex_t *out_index);

static inline uint64_t ssz_next_pow_of_two(uint64_t i)
{
    if (i <= 1u)
    {
        return 1u;
    }
    if (i > (UINT64_C(1) << 63u))
    {
        return 0u;
    }
    i--;
    i |= i >> 1u;
    i |= i >> 2u;
    i |= i >> 4u;
    i |= i >> 8u;
    i |= i >> 16u;
    i |= i >> 32u;
    return i + 1u;
}

ssz_error_t ssz_get_branch_indices(
    ssz_gindex_t tree_index,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_get_path_indices(
    ssz_gindex_t tree_index,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len);

static inline size_t ssz_generalized_index_length(ssz_gindex_t index)
{
    size_t length = 0u;
    while (index > 1u)
    {
        index >>= 1u;
        length++;
    }
    return length;
}

static inline bool ssz_generalized_index_bit(ssz_gindex_t index, size_t position)
{
    return (position < 64u) && (((index >> position) & 1u) != 0u);
}

static inline ssz_gindex_t ssz_generalized_index_sibling(ssz_gindex_t index)
{
    return index ^ 1u;
}

static inline ssz_gindex_t ssz_generalized_index_child(ssz_gindex_t index, bool right_side)
{
    if (index >= (UINT64_C(1) << 63u))
    {
        return 0u;
    }
    return (index << 1u) | (right_side ? 1u : 0u);
}

static inline ssz_gindex_t ssz_generalized_index_parent(ssz_gindex_t index)
{
    return index >> 1u;
}

ssz_error_t ssz_get_helper_indices(
    const ssz_gindex_t *indices,
    size_t index_count,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len,
    ssz_gindex_t *scratch,
    size_t scratch_cap);

ssz_error_t ssz_calculate_merkle_root(
    const ssz_chunk_t *leaf,
    const ssz_chunk_t *proof,
    size_t proof_len,
    ssz_gindex_t index,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_calculate_multi_merkle_root(
    const ssz_chunk_t *leaves,
    const ssz_gindex_t *indices,
    size_t leaf_count,
    const ssz_chunk_t *proof,
    size_t proof_count,
    ssz_gindex_t *scratch_indices,
    ssz_chunk_t *scratch_nodes,
    size_t scratch_cap,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root);

ssz_error_t ssz_verify_merkle_proof(
    const ssz_chunk_t *leaf,
    const ssz_chunk_t *proof,
    size_t proof_len,
    ssz_gindex_t index,
    const ssz_chunk_t *expected_root,
    const ssz_hash_fn_t *hash_fn);

ssz_error_t ssz_verify_merkle_multiproof(
    const ssz_chunk_t *leaves,
    const ssz_gindex_t *indices,
    size_t leaf_count,
    const ssz_chunk_t *proof,
    size_t proof_count,
    const ssz_chunk_t *expected_root,
    ssz_gindex_t *scratch_indices,
    ssz_chunk_t *scratch_nodes,
    size_t scratch_cap,
    const ssz_hash_fn_t *hash_fn);

#ifdef __cplusplus
}
#endif

#endif
