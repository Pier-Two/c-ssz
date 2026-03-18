#include <stdlib.h>
#include <string.h>

#include "ssz_hash.h"
#include "ssz_internal.h"
#include "ssz_proof.h"

static int ssz_internal_compare_gindex_asc(const void *a, const void *b)
{
    ssz_gindex_t ia = *(const ssz_gindex_t *)a;
    ssz_gindex_t ib = *(const ssz_gindex_t *)b;
    if (ia < ib)
    {
        return -1;
    }
    if (ia > ib)
    {
        return 1;
    }
    return 0;
}

static int ssz_internal_compare_gindex_desc(const void *a, const void *b)
{
    ssz_gindex_t ia = *(const ssz_gindex_t *)a;
    ssz_gindex_t ib = *(const ssz_gindex_t *)b;
    if (ia < ib)
    {
        return 1;
    }
    if (ia > ib)
    {
        return -1;
    }
    return 0;
}

static size_t ssz_internal_dedup_sorted(ssz_gindex_t *values, size_t count)
{
    if (count == 0u)
    {
        return 0u;
    }

    size_t out = 1u;
    for (size_t i = 1u; i < count; i++)
    {
        if (values[i] != values[out - 1u])
        {
            values[out++] = values[i];
        }
    }

    return out;
}

static ssz_error_t ssz_internal_compute_helper_indices(
    const ssz_gindex_t *indices,
    size_t index_count,
    ssz_gindex_t *scratch,
    size_t scratch_cap,
    size_t *out_count)
{
    size_t branch_total = 0u;
    size_t path_total = 0u;

    if ((indices == NULL) || (index_count == 0u) || (scratch == NULL) || (out_count == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < index_count; i++)
    {
        if (indices[i] == 0u)
        {
            return SSZ_ERR_GINDEX_INVALID;
        }

        size_t depth = ssz_generalized_index_length(indices[i]);
        if (ssz_internal_add_overflow_size(branch_total, depth, &branch_total) ||
            ssz_internal_add_overflow_size(path_total, depth, &path_total))
        {
            return SSZ_ERR_OVERFLOW;
        }
    }

    size_t required = 0u;
    if (ssz_internal_add_overflow_size(branch_total, path_total, &required))
    {
        return SSZ_ERR_OVERFLOW;
    }
    if (scratch_cap < required)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    ssz_gindex_t *branch_indices = scratch;
    ssz_gindex_t *path_indices = scratch + branch_total;

    size_t branch_pos = 0u;
    size_t path_pos = 0u;

    for (size_t i = 0u; i < index_count; i++)
    {
        ssz_gindex_t cur = indices[i];
        while (cur > 1u)
        {
            branch_indices[branch_pos++] = ssz_generalized_index_sibling(cur);
            path_indices[path_pos++] = cur;
            cur = ssz_generalized_index_parent(cur);
        }
    }

    qsort(branch_indices, branch_pos, sizeof(ssz_gindex_t), ssz_internal_compare_gindex_asc);
    qsort(path_indices, path_pos, sizeof(ssz_gindex_t), ssz_internal_compare_gindex_asc);

    branch_pos = ssz_internal_dedup_sorted(branch_indices, branch_pos);
    path_pos = ssz_internal_dedup_sorted(path_indices, path_pos);

    size_t i = 0u;
    size_t j = 0u;
    size_t diff_count = 0u;

    while (i < branch_pos)
    {
        if ((j >= path_pos) || (branch_indices[i] < path_indices[j]))
        {
            scratch[diff_count++] = branch_indices[i++];
            continue;
        }
        if (branch_indices[i] == path_indices[j])
        {
            i++;
            continue;
        }

        j++;
    }

    qsort(scratch, diff_count, sizeof(ssz_gindex_t), ssz_internal_compare_gindex_desc);
    *out_count = diff_count;
    return SSZ_SUCCESS;
}

static ptrdiff_t ssz_internal_find_node(
    const ssz_gindex_t *indices,
    size_t count,
    ssz_gindex_t index)
{
    for (size_t i = 0u; i < count; i++)
    {
        if (indices[i] == index)
        {
            return (ptrdiff_t)i;
        }
    }
    return -1;
}

static ssz_error_t ssz_internal_insert_node(
    ssz_gindex_t *indices,
    ssz_chunk_t *nodes,
    size_t *count,
    size_t cap,
    ssz_gindex_t index,
    const ssz_chunk_t *node)
{
    ptrdiff_t existing = ssz_internal_find_node(indices, *count, index);
    if (existing >= 0)
    {
        if (memcmp(nodes[(size_t)existing].bytes, node->bytes, SSZ_BYTES_PER_CHUNK) != 0)
        {
            return SSZ_ERR_PROOF_INVALID;
        }
        return SSZ_SUCCESS;
    }

    if (*count >= cap)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    indices[*count] = index;
    nodes[*count] = *node;
    (*count)++;
    return SSZ_SUCCESS;
}

static void ssz_internal_sort_pairs_desc(
    ssz_gindex_t *indices,
    ssz_chunk_t *nodes,
    size_t count)
{
    for (size_t i = 1u; i < count; i++)
    {
        ssz_gindex_t key_index = indices[i];
        ssz_chunk_t key_node = nodes[i];
        size_t j = i;

        while ((j > 0u) && (indices[j - 1u] < key_index))
        {
            indices[j] = indices[j - 1u];
            nodes[j] = nodes[j - 1u];
            j--;
        }

        indices[j] = key_index;
        nodes[j] = key_node;
    }
}

ssz_error_t ssz_get_generalized_index(
    const ssz_gindex_type_t *type,
    const ssz_path_step_t *path,
    size_t path_len,
    ssz_gindex_t *out_index)
{
    static const ssz_gindex_type_t length_type = {
        .kind = SSZ_GINDEX_LEAF,
        .chunk_count = 1u,
        .item_length = 8u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };

    const ssz_gindex_type_t *current = type;
    ssz_gindex_t root = 1u;

    if ((type == NULL) || (out_index == NULL) || ((path_len != 0u) && (path == NULL)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < path_len; i++)
    {
        if (current->kind == SSZ_GINDEX_LEAF)
        {
            return SSZ_ERR_GINDEX_INVALID;
        }

        if (path[i] == SSZ_PATH_STEP_LENGTH)
        {
            if ((current->kind != SSZ_GINDEX_ELEMENTS) || !current->has_mix_in_length)
            {
                return SSZ_ERR_GINDEX_INVALID;
            }

            if (ssz_internal_mul_overflow_u64(root, 2u, &root) ||
                ssz_internal_add_overflow_u64(root, 1u, &root))
            {
                return SSZ_ERR_OVERFLOW;
            }

            current = &length_type;
            continue;
        }

        if (current->chunk_count == 0u)
        {
            return SSZ_ERR_SCHEMA_INVALID;
        }

        uint64_t pos = 0u;
        const ssz_gindex_type_t *next_type = NULL;

        if (current->kind == SSZ_GINDEX_CONTAINER)
        {
            if ((current->field_types == NULL) || (path[i] >= current->chunk_count))
            {
                return SSZ_ERR_GINDEX_INVALID;
            }
            pos = path[i];
            next_type = current->field_types[path[i]];
            if (next_type == NULL)
            {
                return SSZ_ERR_TYPE_MISMATCH;
            }
        }
        else
        {
            if (current->elem_type == NULL)
            {
                return SSZ_ERR_SCHEMA_INVALID;
            }
            if ((current->item_length == 0u) || (current->item_length > 32u))
            {
                return SSZ_ERR_SCHEMA_INVALID;
            }

            if (current->item_length == 32u)
            {
                if (path[i] >= current->chunk_count)
                {
                    return SSZ_ERR_GINDEX_INVALID;
                }
                pos = path[i];
            }
            else
            {
                uint64_t start_bytes = 0u;
                if (ssz_internal_mul_overflow_u64(path[i], current->item_length, &start_bytes))
                {
                    return SSZ_ERR_OVERFLOW;
                }
                pos = start_bytes / 32u;
                if (pos >= current->chunk_count)
                {
                    return SSZ_ERR_GINDEX_INVALID;
                }
            }

            next_type = current->elem_type;
        }

        uint64_t width = ssz_next_pow_of_two(current->chunk_count);
        if (width == 0u)
        {
            return SSZ_ERR_OVERFLOW;
        }

        uint64_t base = current->has_mix_in_length ? 2u : 1u;
        uint64_t multiplier = 0u;
        if (ssz_internal_mul_overflow_u64(base, width, &multiplier))
        {
            return SSZ_ERR_OVERFLOW;
        }

        if (ssz_internal_mul_overflow_u64(root, multiplier, &root) ||
            ssz_internal_add_overflow_u64(root, pos, &root))
        {
            return SSZ_ERR_OVERFLOW;
        }

        current = next_type;
    }

    *out_index = root;
    return SSZ_SUCCESS;
}

ssz_error_t ssz_get_branch_indices(
    ssz_gindex_t tree_index,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len)
{
    if (tree_index == 0u)
    {
        return SSZ_ERR_GINDEX_INVALID;
    }

    size_t required = ssz_generalized_index_length(tree_index);
    if (out_indices == NULL)
    {
        if (out_len == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        *out_len = required;
        return SSZ_SUCCESS;
    }
    if (out_cap < required)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    size_t pos = 0u;
    ssz_gindex_t cur = tree_index;
    while (cur > 1u)
    {
        out_indices[pos++] = ssz_generalized_index_sibling(cur);
        cur = ssz_generalized_index_parent(cur);
    }

    if (out_len != NULL)
    {
        *out_len = required;
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_get_path_indices(
    ssz_gindex_t tree_index,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len)
{
    if (tree_index == 0u)
    {
        return SSZ_ERR_GINDEX_INVALID;
    }

    size_t required = ssz_generalized_index_length(tree_index);
    if (out_indices == NULL)
    {
        if (out_len == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        *out_len = required;
        return SSZ_SUCCESS;
    }
    if (out_cap < required)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    size_t pos = 0u;
    ssz_gindex_t cur = tree_index;
    while (cur > 1u)
    {
        out_indices[pos++] = cur;
        cur = ssz_generalized_index_parent(cur);
    }

    if (out_len != NULL)
    {
        *out_len = required;
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_get_helper_indices(
    const ssz_gindex_t *indices,
    size_t index_count,
    ssz_gindex_t *out_indices,
    size_t out_cap,
    size_t *out_len,
    ssz_gindex_t *scratch,
    size_t scratch_cap)
{
    size_t helper_count = 0u;

    if ((scratch == NULL) || (scratch_cap == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err =
        ssz_internal_compute_helper_indices(indices, index_count, scratch, scratch_cap, &helper_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (out_indices == NULL)
    {
        if (out_len == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        *out_len = helper_count;
        return SSZ_SUCCESS;
    }

    if (out_cap < helper_count)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (helper_count != 0u)
    {
        memcpy(out_indices, scratch, helper_count * sizeof(ssz_gindex_t));
    }
    if (out_len != NULL)
    {
        *out_len = helper_count;
    }

    return SSZ_SUCCESS;
}

ssz_error_t ssz_calculate_merkle_root(
    const ssz_chunk_t *leaf,
    const ssz_chunk_t *proof,
    size_t proof_len,
    ssz_gindex_t index,
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *out_root)
{
    if ((leaf == NULL) || (out_root == NULL) || ((proof_len != 0u) && (proof == NULL)))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (index == 0u)
    {
        return SSZ_ERR_GINDEX_INVALID;
    }

    size_t expected_len = ssz_generalized_index_length(index);
    if (proof_len != expected_len)
    {
        return SSZ_ERR_PROOF_INVALID;
    }

    ssz_chunk_t current = *leaf;
    for (size_t i = 0u; i < proof_len; i++)
    {
        ssz_error_t err = SSZ_SUCCESS;
        if (ssz_generalized_index_bit(index, i))
        {
            err = ssz_hash_2to1(hash_fn, &proof[i], &current, &current);
        }
        else
        {
            err = ssz_hash_2to1(hash_fn, &current, &proof[i], &current);
        }
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    *out_root = current;
    return SSZ_SUCCESS;
}

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
    ssz_chunk_t *out_root)
{
    size_t helper_count = 0u;

    if ((leaf_count == 0u) || (leaves == NULL) || (indices == NULL) ||
        ((proof_count != 0u) && (proof == NULL)) ||
        (scratch_indices == NULL) || (scratch_nodes == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err =
        ssz_internal_compute_helper_indices(indices, leaf_count, scratch_indices, scratch_cap, &helper_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    if (helper_count != proof_count)
    {
        return SSZ_ERR_PROOF_INVALID;
    }

    size_t map_offset = helper_count;
    if (map_offset > scratch_cap)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    ssz_gindex_t *map_indices = scratch_indices + map_offset;
    size_t map_cap = scratch_cap - map_offset;
    if (map_cap == 0u)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    size_t map_count = 0u;
    for (size_t i = 0u; i < leaf_count; i++)
    {
        if (indices[i] == 0u)
        {
            return SSZ_ERR_GINDEX_INVALID;
        }

        err = ssz_internal_insert_node(
            map_indices,
            scratch_nodes,
            &map_count,
            map_cap,
            indices[i],
            &leaves[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    for (size_t i = 0u; i < proof_count; i++)
    {
        err = ssz_internal_insert_node(
            map_indices,
            scratch_nodes,
            &map_count,
            map_cap,
            scratch_indices[i],
            &proof[i]);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
    }

    ssz_internal_sort_pairs_desc(map_indices, scratch_nodes, map_count);

    size_t pos = 0u;
    while (pos < map_count)
    {
        ssz_gindex_t k = map_indices[pos];
        if (k > 1u)
        {
            ssz_gindex_t sibling = ssz_generalized_index_sibling(k);
            ssz_gindex_t parent = ssz_generalized_index_parent(k);

            if ((ssz_internal_find_node(map_indices, map_count, sibling) >= 0) &&
                (ssz_internal_find_node(map_indices, map_count, parent) < 0))
            {
                ssz_gindex_t left_index = (k | 1u) ^ 1u;
                ssz_gindex_t right_index = (k | 1u);

                ptrdiff_t left_pos = ssz_internal_find_node(map_indices, map_count, left_index);
                ptrdiff_t right_pos = ssz_internal_find_node(map_indices, map_count, right_index);
                if ((left_pos < 0) || (right_pos < 0))
                {
                    return SSZ_ERR_PROOF_INVALID;
                }

                ssz_chunk_t parent_node;
                err = ssz_hash_2to1(hash_fn,
                                   &scratch_nodes[(size_t)left_pos],
                                   &scratch_nodes[(size_t)right_pos],
                                   &parent_node);
                if (err != SSZ_SUCCESS)
                {
                    return err;
                }

                err = ssz_internal_insert_node(
                    map_indices,
                    scratch_nodes,
                    &map_count,
                    map_cap,
                    parent,
                    &parent_node);
                if (err != SSZ_SUCCESS)
                {
                    return err;
                }
            }
        }

        pos++;
    }

    ptrdiff_t root_pos = ssz_internal_find_node(map_indices, map_count, 1u);
    if (root_pos < 0)
    {
        return SSZ_ERR_PROOF_INVALID;
    }

    *out_root = scratch_nodes[(size_t)root_pos];
    return SSZ_SUCCESS;
}

ssz_error_t ssz_verify_merkle_proof(
    const ssz_chunk_t *leaf,
    const ssz_chunk_t *proof,
    size_t proof_len,
    ssz_gindex_t index,
    const ssz_chunk_t *expected_root,
    const ssz_hash_fn_t *hash_fn)
{
    ssz_chunk_t computed_root;

    if ((leaf == NULL) || (expected_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err =
        ssz_calculate_merkle_root(leaf, proof, proof_len, index, hash_fn, &computed_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (memcmp(computed_root.bytes, expected_root->bytes, SSZ_BYTES_PER_CHUNK) != 0)
    {
        return SSZ_ERR_PROOF_INVALID;
    }

    return SSZ_SUCCESS;
}

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
    const ssz_hash_fn_t *hash_fn)
{
    ssz_chunk_t computed_root;

    if (expected_root == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ssz_error_t err = ssz_calculate_multi_merkle_root(leaves,
                                                      indices,
                                                      leaf_count,
                                                      proof,
                                                      proof_count,
                                                      scratch_indices,
                                                      scratch_nodes,
                                                      scratch_cap,
                                                      hash_fn,
                                                      &computed_root);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (memcmp(computed_root.bytes, expected_root->bytes, SSZ_BYTES_PER_CHUNK) != 0)
    {
        return SSZ_ERR_PROOF_INVALID;
    }

    return SSZ_SUCCESS;
}
