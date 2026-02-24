#include <stddef.h>
#include <stdint.h>

#include "ssz.h"

typedef struct
{
    const uint8_t *ptr;
    size_t remaining;
} fuzz_input_t;

static uint8_t fuzz_take_u8(fuzz_input_t *input)
{
    if ((input == NULL) || (input->remaining == 0u))
    {
        return 0u;
    }

    uint8_t value = input->ptr[0];
    input->ptr++;
    input->remaining--;
    return value;
}

static uint64_t fuzz_take_u64(fuzz_input_t *input)
{
    uint64_t value = 0u;

    for (size_t i = 0u; i < 8u; i++)
    {
        value |= ((uint64_t)fuzz_take_u8(input)) << (8u * i);
    }

    return value;
}

static uint64_t fuzz_take_u64_bounded(fuzz_input_t *input, uint64_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    if (max_inclusive == UINT64_MAX)
    {
        return value;
    }

    return value % (max_inclusive + 1u);
}

static size_t fuzz_take_size_bounded(fuzz_input_t *input, size_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    uint64_t bound = (uint64_t)max_inclusive;
    return (size_t)(value % (bound + 1u));
}

static ssz_chunk_t fuzz_take_chunk(fuzz_input_t *input)
{
    ssz_chunk_t chunk = {{0u}};

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = fuzz_take_u8(input);
    }

    return chunk;
}

static ssz_error_t fuzz_custom_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    (void)data;
    (void)data_len;

    if (out == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < 32u; i++)
    {
        out[i] = 0x22u;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_custom_2to1_err(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    (void)ctx;
    (void)left;
    (void)right;
    (void)out;
    return SSZ_ERR_HASH_FAILURE;
}

static void fuzz_cover_proof_errors(void)
{
    ssz_gindex_t out_index = 0u;
    ssz_path_step_t path0[1] = {0u};

    ssz_chunk_t leaf = {{0u}};
    ssz_chunk_t out_root;
    ssz_chunk_t proof_nodes[2] = {{{0u}}};
    ssz_chunk_t leaves_multi[2] = {{{0u}}};
    ssz_gindex_t indices_multi[2] = {2u, 4u};
    ssz_gindex_t indices_one[1] = {2u};
    ssz_gindex_t helper_input[3] = {10u, 4u, 7u};
    ssz_gindex_t helper_out[64] = {0u};
    size_t helper_len = 0u;
    ssz_gindex_t scratch_indices[128] = {0u};
    ssz_chunk_t scratch_nodes[128] = {{{0u}}};

    ssz_hash_fn_t hash_err_2to1 = {
        .hash = fuzz_custom_hash,
        .hash_2to1 = fuzz_custom_2to1_err,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };

    ssz_gindex_type_t leaf_type = {
        .kind = SSZ_GINDEX_LEAF,
        .chunk_count = 1u,
        .item_length = 32u,
        .has_mix_in_length = false,
        .elem_type = NULL,
        .field_types = NULL,
    };

    (void)ssz_get_generalized_index(NULL, NULL, 0u, &out_index);
    (void)ssz_get_generalized_index(&leaf_type, NULL, 1u, &out_index);
    (void)ssz_get_generalized_index(&leaf_type, NULL, 0u, NULL);

    {
        ssz_path_step_t long_path[64] = {0u};
        ssz_gindex_type_t recursive_length_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = 1u,
            .item_length = 32u,
            .has_mix_in_length = true,
            .elem_type = NULL,
            .field_types = NULL,
        };
        recursive_length_type.elem_type = &recursive_length_type;
        for (size_t i = 0u; i < 63u; i++)
        {
            long_path[i] = 0u;
        }
        long_path[63] = SSZ_PATH_STEP_LENGTH;
        (void)ssz_get_generalized_index(&recursive_length_type, long_path, 64u, &out_index);
    }

    {
        const ssz_gindex_type_t *field_types[1] = {NULL};
        ssz_gindex_type_t bad_container_type = {
            .kind = SSZ_GINDEX_CONTAINER,
            .chunk_count = 1u,
            .item_length = 0u,
            .has_mix_in_length = false,
            .elem_type = NULL,
            .field_types = field_types,
        };
        (void)ssz_get_generalized_index(&bad_container_type, path0, 1u, &out_index);
    }

    {
        ssz_gindex_type_t bad_elements_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = 1u,
            .item_length = 32u,
            .has_mix_in_length = false,
            .elem_type = NULL,
            .field_types = NULL,
        };
        (void)ssz_get_generalized_index(&bad_elements_type, path0, 1u, &out_index);
    }

    {
        ssz_path_step_t overflow_path[1] = {(ssz_path_step_t)UINT64_MAX};
        ssz_gindex_type_t mul_overflow_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = 2u,
            .item_length = 31u,
            .has_mix_in_length = false,
            .elem_type = &leaf_type,
            .field_types = NULL,
        };
        (void)ssz_get_generalized_index(&mul_overflow_type, overflow_path, 1u, &out_index);
    }

    {
        ssz_gindex_type_t width_overflow_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = (UINT64_C(1) << 63u) + 1u,
            .item_length = 32u,
            .has_mix_in_length = false,
            .elem_type = &leaf_type,
            .field_types = NULL,
        };
        (void)ssz_get_generalized_index(&width_overflow_type, path0, 1u, &out_index);
    }

    {
        ssz_gindex_type_t multiplier_overflow_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = UINT64_C(1) << 63u,
            .item_length = 32u,
            .has_mix_in_length = true,
            .elem_type = &leaf_type,
            .field_types = NULL,
        };
        (void)ssz_get_generalized_index(&multiplier_overflow_type, path0, 1u, &out_index);
    }

    {
        ssz_path_step_t root_overflow_path[64] = {0u};
        ssz_gindex_type_t root_overflow_type = {
            .kind = SSZ_GINDEX_ELEMENTS,
            .chunk_count = 2u,
            .item_length = 32u,
            .has_mix_in_length = false,
            .elem_type = NULL,
            .field_types = NULL,
        };
        root_overflow_type.elem_type = &root_overflow_type;
        (void)ssz_get_generalized_index(&root_overflow_type, root_overflow_path, 64u, &out_index);
    }

    (void)ssz_get_branch_indices(2u, NULL, 0u, NULL);
    (void)ssz_get_path_indices(2u, NULL, 0u, NULL);

    (void)ssz_get_helper_indices(helper_input,
                                 3u,
                                 helper_out,
                                 sizeof(helper_out) / sizeof(helper_out[0]),
                                 &helper_len,
                                 scratch_indices,
                                 sizeof(scratch_indices) / sizeof(scratch_indices[0]));

    (void)ssz_calculate_merkle_root(NULL, proof_nodes, 1u, 2u, ssz_hash_default(), &out_root);
    (void)ssz_calculate_merkle_root(&leaf, proof_nodes, 1u, 2u, ssz_hash_default(), NULL);
    (void)ssz_calculate_merkle_root(&leaf, proof_nodes, 1u, 2u, &hash_err_2to1, &out_root);

    (void)ssz_calculate_multi_merkle_root(leaves_multi,
                                          indices_one,
                                          1u,
                                          proof_nodes,
                                          1u,
                                          scratch_indices,
                                          scratch_nodes,
                                          128u,
                                          &hash_err_2to1,
                                          &out_root);
    (void)ssz_calculate_multi_merkle_root(leaves_multi,
                                          indices_multi,
                                          2u,
                                          proof_nodes,
                                          2u,
                                          scratch_indices,
                                          scratch_nodes,
                                          128u,
                                          ssz_hash_default(),
                                          &out_root);

    (void)ssz_verify_merkle_proof(NULL, proof_nodes, 1u, 2u, &out_root, ssz_hash_default());
    (void)ssz_verify_merkle_multiproof(leaves_multi,
                                       indices_one,
                                       1u,
                                       proof_nodes,
                                       1u,
                                       NULL,
                                       scratch_indices,
                                       scratch_nodes,
                                       128u,
                                       ssz_hash_default());
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if ((data == NULL) || (size == 0u))
    {
        return 0;
    }

    fuzz_input_t input = {
        .ptr = data,
        .remaining = size,
    };

    const ssz_hash_fn_t *hash_fn = ssz_hash_default();
    uint8_t api_selector = fuzz_take_u8(&input);

    switch (api_selector % 8u)
    {
        case 0u:
        {
            ssz_gindex_type_t leaf_type = {
                .kind = SSZ_GINDEX_LEAF,
                .chunk_count = 1u,
                .item_length = 32u,
                .has_mix_in_length = false,
                .elem_type = NULL,
                .field_types = NULL,
            };
            ssz_gindex_type_t elements_type = {
                .kind = SSZ_GINDEX_ELEMENTS,
                .chunk_count = fuzz_take_u64_bounded(&input, 8u),
                .item_length = (uint8_t)fuzz_take_u64_bounded(&input, 40u),
                .has_mix_in_length = (fuzz_take_u8(&input) & 1u) != 0u,
                .elem_type = &leaf_type,
                .field_types = NULL,
            };
            const ssz_gindex_type_t *field_types[4] = {
                &leaf_type,
                &elements_type,
                &leaf_type,
                &elements_type,
            };
            ssz_gindex_type_t container_type = {
                .kind = SSZ_GINDEX_CONTAINER,
                .chunk_count = 4u,
                .item_length = 0u,
                .has_mix_in_length = false,
                .elem_type = NULL,
                .field_types = field_types,
            };

            const ssz_gindex_type_t *root_type = (fuzz_take_u8(&input) & 1u) != 0u
                                                     ? &container_type
                                                     : &elements_type;
            size_t path_len = fuzz_take_size_bounded(&input, 8u);
            ssz_path_step_t path[8] = {0u};

            for (size_t i = 0u; i < path_len; i++)
            {
                if ((fuzz_take_u8(&input) & 1u) != 0u)
                {
                    path[i] = SSZ_PATH_STEP_LENGTH;
                }
                else
                {
                    path[i] = (ssz_path_step_t)fuzz_take_u64_bounded(&input, 32u);
                }
            }

            ssz_gindex_t out_index = 0u;
            (void)ssz_get_generalized_index(root_type, path, path_len, &out_index);
            break;
        }

        case 1u:
        {
            ssz_gindex_t tree_index = (ssz_gindex_t)fuzz_take_u64(&input);
            ssz_gindex_t out_indices[64] = {0u};
            size_t out_cap = fuzz_take_size_bounded(&input, 64u);
            size_t out_len = 0u;

            if ((fuzz_take_u8(&input) & 1u) != 0u)
            {
                (void)ssz_get_branch_indices(tree_index, NULL, 0u, &out_len);
            }
            else
            {
                size_t *out_len_ptr = ((fuzz_take_u8(&input) & 1u) != 0u) ? &out_len : NULL;
                (void)ssz_get_branch_indices(tree_index, out_indices, out_cap, out_len_ptr);
            }

            break;
        }

        case 2u:
        {
            ssz_gindex_t tree_index = (ssz_gindex_t)fuzz_take_u64(&input);
            ssz_gindex_t out_indices[64] = {0u};
            size_t out_cap = fuzz_take_size_bounded(&input, 64u);
            size_t out_len = 0u;

            if ((fuzz_take_u8(&input) & 1u) != 0u)
            {
                (void)ssz_get_path_indices(tree_index, NULL, 0u, &out_len);
            }
            else
            {
                size_t *out_len_ptr = ((fuzz_take_u8(&input) & 1u) != 0u) ? &out_len : NULL;
                (void)ssz_get_path_indices(tree_index, out_indices, out_cap, out_len_ptr);
            }

            break;
        }

        case 3u:
        {
            size_t index_count = fuzz_take_size_bounded(&input, 8u);
            ssz_gindex_t indices[8] = {0u};
            for (size_t i = 0u; i < index_count; i++)
            {
                indices[i] = (ssz_gindex_t)fuzz_take_u64(&input);
            }

            ssz_gindex_t out_indices[64] = {0u};
            size_t out_cap = fuzz_take_size_bounded(&input, 64u);
            size_t out_len = 0u;

            ssz_gindex_t scratch[128] = {0u};
            size_t scratch_cap = fuzz_take_size_bounded(&input, 128u);

            bool out_indices_null = (fuzz_take_u8(&input) & 1u) != 0u;
            size_t *out_len_ptr = ((fuzz_take_u8(&input) & 1u) != 0u) ? &out_len : NULL;

            (void)ssz_get_helper_indices(
                indices,
                index_count,
                out_indices_null ? NULL : out_indices,
                out_cap,
                out_len_ptr,
                scratch,
                scratch_cap);
            break;
        }

        case 4u:
        {
            ssz_chunk_t leaf = fuzz_take_chunk(&input);
            size_t proof_len = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t proof[16] = {{{0u}}};

            for (size_t i = 0u; i < proof_len; i++)
            {
                proof[i] = fuzz_take_chunk(&input);
            }

            ssz_gindex_t index = (ssz_gindex_t)fuzz_take_u64(&input);
            ssz_chunk_t out_root;
            (void)ssz_calculate_merkle_root(&leaf, proof, proof_len, index, hash_fn, &out_root);
            break;
        }

        case 5u:
        {
            size_t leaf_count = fuzz_take_size_bounded(&input, 8u);
            ssz_chunk_t leaves[8] = {{{0u}}};
            ssz_gindex_t indices[8] = {0u};

            for (size_t i = 0u; i < leaf_count; i++)
            {
                leaves[i] = fuzz_take_chunk(&input);
                indices[i] = (ssz_gindex_t)fuzz_take_u64(&input);
            }

            size_t proof_count = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t proof[16] = {{{0u}}};
            for (size_t i = 0u; i < proof_count; i++)
            {
                proof[i] = fuzz_take_chunk(&input);
            }

            ssz_gindex_t scratch_indices[128] = {0u};
            ssz_chunk_t scratch_nodes[128] = {{{0u}}};
            size_t scratch_cap = fuzz_take_size_bounded(&input, 128u);

            ssz_chunk_t out_root;
            (void)ssz_calculate_multi_merkle_root(
                leaves,
                indices,
                leaf_count,
                proof,
                proof_count,
                scratch_indices,
                scratch_nodes,
                scratch_cap,
                hash_fn,
                &out_root);
            break;
        }

        case 6u:
        {
            ssz_chunk_t leaf = fuzz_take_chunk(&input);
            size_t proof_len = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t proof[16] = {{{0u}}};

            for (size_t i = 0u; i < proof_len; i++)
            {
                proof[i] = fuzz_take_chunk(&input);
            }

            ssz_gindex_t index = (ssz_gindex_t)fuzz_take_u64(&input);
            ssz_chunk_t expected_root = fuzz_take_chunk(&input);

            (void)ssz_verify_merkle_proof(
                &leaf,
                proof,
                proof_len,
                index,
                &expected_root,
                hash_fn);
            break;
        }

        default:
        {
            size_t leaf_count = fuzz_take_size_bounded(&input, 8u);
            ssz_chunk_t leaves[8] = {{{0u}}};
            ssz_gindex_t indices[8] = {0u};

            for (size_t i = 0u; i < leaf_count; i++)
            {
                leaves[i] = fuzz_take_chunk(&input);
                indices[i] = (ssz_gindex_t)fuzz_take_u64(&input);
            }

            size_t proof_count = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t proof[16] = {{{0u}}};
            for (size_t i = 0u; i < proof_count; i++)
            {
                proof[i] = fuzz_take_chunk(&input);
            }

            ssz_chunk_t expected_root = fuzz_take_chunk(&input);
            ssz_gindex_t scratch_indices[128] = {0u};
            ssz_chunk_t scratch_nodes[128] = {{{0u}}};
            size_t scratch_cap = fuzz_take_size_bounded(&input, 128u);

            (void)ssz_verify_merkle_multiproof(
                leaves,
                indices,
                leaf_count,
                proof,
                proof_count,
                &expected_root,
                scratch_indices,
                scratch_nodes,
                scratch_cap,
                hash_fn);
            break;
        }
    }

    fuzz_cover_proof_errors();

    return 0;
}
