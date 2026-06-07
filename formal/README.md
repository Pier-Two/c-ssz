# c-ssz formal verification

This directory holds the VST/Coq formal verification scaffolding for the C
functions in [../src/](../src/). The goal here is not full verification of the
library. The goal is bug hunting: writing a specification and attempting a
proof forces the gap between the C and the SSZ contract to surface as a failed
proof, which is then triaged as either a real defect in the C, a mistake in
the specification, or a proof gap. Only the first category produces a code
change in [../src/](../src/). The shipped binary still compiles with the
regular C compiler, not CompCert.

## Prerequisites

An opam switch named `vst` with CompCert and VST installed. If it does not
exist yet:

```sh
opam switch create vst ocaml-base-compiler.4.14.2
opam switch set vst
opam install coq-compcert coq-vst
eval "$(opam env --switch=vst --set-switch)"
```

## Running the proofs

From this directory:

```sh
cd c-ssz/formal
./scripts/verify.sh
```

This regenerates the Clight translation of every `../src/*.c` listed in
[_CoqProject](_CoqProject), compiles the models, specifications, and proofs,
and exits zero only if every theorem closes without proof stubs or extra
assumptions.

## Layout

| Path | Purpose |
| --- | --- |
| [clight/](clight/) | CompCert `clightgen` output, one `.v` per `../src/*.c` |
| [model/](model/) | Pure Coq reference models for the SSZ contract |
| [spec/](spec/) | VST function specifications |
| [verif/](verif/) | VST proofs, one file per implementation file |
| [scripts/clightgen.sh](scripts/clightgen.sh) | Regenerates Clight from `../src/` |
| [scripts/verify.sh](scripts/verify.sh) | Single end to end entry point |
| [scripts/triage.sh](scripts/triage.sh) | Helper for classifying proof failures |

## Coverage

The **Spec** column has a ✅ when a VST function specification exists in
[spec/](spec/) and references a Coq reference model in [model/](model/). The
**Verification** column has a ✅ when a `semax_body` proof in [verif/](verif/)
closes without proof stubs or extra assumptions.

### ssz_deserialize.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_deserialize_variable_sequence |   |   |
| ssz_internal_validate_boolean_bytes |   |   |
| ssz_deserialize_uint8 |   |   |
| ssz_deserialize_uint16 |   |   |
| ssz_deserialize_uint32 |   |   |
| ssz_deserialize_uint64 |   |   |
| ssz_deserialize_uint128 |   |   |
| ssz_deserialize_uint256 |   |   |
| ssz_deserialize_boolean |   |   |
| ssz_deserialize_bitvector |   |   |
| ssz_deserialize_bitlist |   |   |
| ssz_deserialize_vector_fixed |   |   |
| ssz_deserialize_vector_boolean |   |   |
| ssz_deserialize_vector_variable |   |   |
| ssz_deserialize_list_fixed |   |   |
| ssz_deserialize_list_boolean |   |   |
| ssz_deserialize_list_variable |   |   |
| ssz_deserialize_container |   |   |
| ssz_deserialize_union |   |   |
| ssz_deserialize_compatible_union |   |   |

### ssz_endian.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_write_u16_le | ✅ | ✅ |
| ssz_internal_write_u32_le | ✅ | ✅ |
| ssz_internal_write_u64_le | ✅ | ✅ |
| ssz_internal_read_u16_le | ✅ | ✅ |
| ssz_internal_read_u32_le | ✅ | ✅ |
| ssz_internal_read_u64_le | ✅ | ✅ |

### ssz_hash.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_store_be32 |   |   |
| ssz_internal_sha256_ctx_to_chunk |   |   |
| ssz_internal_sha256_64_batch_default |   |   |
| ssz_internal_normalize_hash_error |   |   |
| ssz_internal_pointer_address |   |   |
| ssz_internal_byte_ranges_overlap |   |   |
| ssz_hash_sha256 |   |   |
| ssz_internal_default_hash |   |   |
| ssz_internal_init_default_zero_hashes |   |   |
| ssz_internal_init_default_zero_hashes_once |   |   |
| ssz_hash_default |   |   |
| ssz_hash_default_zero_hashes |   |   |
| ssz_hash_2to1_batch_raw |   |   |
| ssz_hash_2to1_batch_inplace |   |   |
| ssz_hash_2to1 |   |   |
| ssz_hash_2to1_batch |   |   |

### ssz_merkle.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_read_bytes_leaf |   |   |
| ssz_internal_read_codec_leaf |   |   |
| ssz_internal_validate_scratch |   |   |
| ssz_internal_scratch_is_invalid |   |   |
| ssz_internal_get_scratch_chunks |   |   |
| ssz_internal_read_leaf |   |   |
| ssz_internal_validate_leaf_range |   |   |
| ssz_internal_init_chunk_source |   |   |
| ssz_internal_init_bytes_source |   |   |
| ssz_internal_init_codec_source |   |   |
| ssz_internal_log2_u64 |   |   |
| ssz_internal_count_fits_size |   |   |
| ssz_internal_build_zero_hashes |   |   |
| ssz_internal_merkleize_reader_fast |   |   |
| ssz_internal_merkleize_full_range_iter |   |   |
| ssz_internal_merkleize_subtree_iter |   |   |
| ssz_internal_merkleize_reader |   |   |
| ssz_internal_byte_len_to_chunk_count |   |   |
| ssz_internal_merkleize_packed_bytes |   |   |
| ssz_hash_tree_root_uint8 |   |   |
| ssz_hash_tree_root_uint16 |   |   |
| ssz_hash_tree_root_uint32 |   |   |
| ssz_hash_tree_root_uint64 |   |   |
| ssz_hash_tree_root_uint128 |   |   |
| ssz_hash_tree_root_uint256 |   |   |
| ssz_hash_tree_root_boolean |   |   |
| ssz_hash_tree_root_bitvector |   |   |
| ssz_hash_tree_root_bitlist |   |   |
| ssz_hash_tree_root_vector_fixed |   |   |
| ssz_hash_tree_root_vector_composite |   |   |
| ssz_hash_tree_root_vector_roots |   |   |
| ssz_hash_tree_root_list_fixed |   |   |
| ssz_hash_tree_root_list_composite |   |   |
| ssz_hash_tree_root_list_roots |   |   |
| ssz_hash_tree_root_union |   |   |
| ssz_merkleize |   |   |
| ssz_mix_in_length |   |   |
| ssz_mix_in_length_u64 |   |   |
| ssz_mix_in_selector |   |   |
| ssz_mix_in_active_fields |   |   |

### ssz_merkle_cache.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_merkle_cache_internal_struct_size_valid |   |   |
| ssz_merkle_cache_internal_pointer_available |   |   |
| ssz_merkle_cache_internal_chunk_buffer_aligned |   |   |
| ssz_merkle_cache_internal_cache_is_bound |   |   |
| ssz_merkle_cache_internal_storage_has_tokens |   |   |
| ssz_merkle_cache_internal_is_power_of_two_u64 |   |   |
| ssz_merkle_cache_internal_log2_u64 |   |   |
| ssz_merkle_cache_internal_ctz_u64 |   |   |
| ssz_merkle_cache_internal_sort_size_t_asc |   |   |
| ssz_merkle_cache_internal_leaf_words_for_capacity |   |   |
| ssz_merkle_cache_internal_parent_dirty_words_for_capacity |   |   |
| ssz_merkle_cache_internal_gather_pair_capacity_for_capacity |   |   |
| ssz_merkle_cache_internal_build_zero_hashes |   |   |
| ssz_merkle_cache_internal_compute_level_offsets |   |   |
| ssz_merkle_cache_internal_fill_zero_tree |   |   |
| ssz_merkle_cache_internal_bind_dirty_set |   |   |
| ssz_merkle_cache_internal_clear_dirty |   |   |
| ssz_merkle_cache_internal_dirty_set_clear |   |   |
| ssz_merkle_cache_internal_dirty_mark_bit |   |   |
| ssz_merkle_cache_internal_dirty_set_mark |   |   |
| ssz_merkle_cache_internal_invalidate_data_root |   |   |
| ssz_merkle_cache_internal_invalidate_final_root |   |   |
| ssz_merkle_cache_internal_token_valid_get |   |   |
| ssz_merkle_cache_internal_token_valid_set |   |   |
| ssz_merkle_cache_internal_token_valid_clear_range |   |   |
| ssz_merkle_cache_internal_mark_leaf_dirty |   |   |
| ssz_merkle_cache_internal_set_leaf |   |   |
| ssz_merkle_cache_internal_byte_len_to_chunk_count |   |   |
| ssz_merkle_cache_internal_effective_tree_depth |   |   |
| ssz_merkle_cache_internal_refresh_cached_data_root |   |   |
| ssz_merkle_cache_internal_flush_contiguous_run |   |   |
| ssz_merkle_cache_internal_hash_dirty_parents_exact |   |   |
| ssz_merkle_cache_internal_build_parent_dirty_set |   |   |
| ssz_merkle_cache_internal_recompute_data_root |   |   |
| ssz_merkle_cache_internal_validate_config |   |   |
| ssz_merkle_cache_internal_compute_requirements |   |   |
| ssz_merkle_cache_internal_validate_storage |   |   |
| ssz_merkle_cache_internal_clear_u64_array |   |   |
| ssz_merkle_cache_internal_clear_bound_working_state |   |   |
| ssz_merkle_cache_internal_prepare_bound_cache |   |   |
| ssz_merkle_cache_internal_initialize_bound_cache |   |   |
| ssz_merkle_cache_internal_migrate_nodes |   |   |
| ssz_merkle_cache_internal_ensure_capacity_for_count |   |   |
| ssz_merkle_cache_internal_compute_data_root_if_needed |   |   |
| ssz_merkle_cache_internal_limit_matches |   |   |
| ssz_merkle_cache_internal_validate_sync_opts |   |   |
| ssz_merkle_cache_requirements |   |   |
| ssz_merkle_cache_bind |   |   |
| ssz_merkle_cache_migrate_into |   |   |
| ssz_merkle_cache_reset |   |   |
| ssz_merkle_cache_data_root |   |   |
| ssz_merkle_cache_root |   |   |
| ssz_merkle_cache_update_root_range |   |   |
| ssz_merkle_cache_zero_range |   |   |
| ssz_merkle_cache_set_logical_length |   |   |
| ssz_merkle_cache_sync_packed_bytes |   |   |
| ssz_merkle_cache_sync_packed_vector_fixed |   |   |
| ssz_merkle_cache_sync_packed_list_fixed |   |   |
| ssz_merkle_cache_sync_bitvector |   |   |
| ssz_merkle_cache_sync_bitlist |   |   |
| ssz_merkle_cache_internal_sync_composite_run |   |   |
| ssz_merkle_cache_internal_sync_composite_fallback |   |   |
| ssz_merkle_cache_sync_composite |   |   |
| ssz_merkle_cache_needs_resync |   |   |

### ssz_proof.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_compare_gindex_asc |   |   |
| ssz_internal_compare_gindex_desc |   |   |
| ssz_internal_compare_gindex_prefix |   |   |
| ssz_internal_swap_gindex |   |   |
| ssz_internal_sift_down_gindex |   |   |
| ssz_internal_sort_gindex_with_compare |   |   |
| ssz_internal_sort_gindex_asc |   |   |
| ssz_internal_sort_gindex_desc |   |   |
| ssz_internal_sort_gindex_prefix |   |   |
| ssz_internal_dedup_sorted |   |   |
| ssz_internal_gindex_covers |   |   |
| ssz_internal_gindex_overlaps |   |   |
| ssz_internal_validate_sorted_multiproof_indices |   |   |
| ssz_internal_validate_multiproof_indices |   |   |
| ssz_internal_validate_multiproof_indices_with_scratch |   |   |
| ssz_internal_compute_helper_indices |   |   |
| ssz_internal_swap_pairs |   |   |
| ssz_internal_sift_down_pairs |   |   |
| ssz_internal_sort_pairs_desc |   |   |
| ssz_internal_gindex_are_siblings |   |   |
| ssz_internal_reduce_multi_merkle_round |   |   |
| ssz_internal_reduce_multi_merkle_nodes |   |   |
| ssz_get_generalized_index |   |   |
| ssz_get_branch_indices |   |   |
| ssz_get_path_indices |   |   |
| ssz_get_helper_indices |   |   |
| ssz_calculate_merkle_root |   |   |
| ssz_calculate_multi_merkle_root |   |   |
| ssz_verify_merkle_proof |   |   |
| ssz_verify_merkle_multiproof |   |   |

### ssz_serialize.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_internal_prepare_output | ✅ | ✅ |
| ssz_serialize_uint8 | ✅ | ✅ |
| ssz_serialize_uint16 | ✅ | ✅ |
| ssz_serialize_uint32 | ✅ | ✅ |
| ssz_serialize_uint64 | ✅ | ✅ |
| ssz_serialize_uint128 | ✅ | ✅ |
| ssz_serialize_uint256 | ✅ | ✅ |
| ssz_serialize_boolean | ✅ | ✅ |
| ssz_serialize_bitvector | ✅ | ✅ |
| ssz_serialize_bitlist |   |   |
| ssz_serialize_vector_fixed |   |   |
| ssz_serialize_vector_variable |   |   |
| ssz_serialize_list_fixed |   |   |
| ssz_serialize_list_variable |   |   |
| ssz_serialize_container |   |   |
| ssz_serialize_union |   |   |
| ssz_serialize_compatible_union |   |   |

### ssz_types.c

| Function | Spec | Verification |
| --- | :---: | :---: |
| ssz_types_internal_measure_member |   |   |
| ssz_types_internal_capture_member |   |   |
| ssz_types_internal_default_member |   |   |
| ssz_types_internal_restore_member |   |   |
| ssz_types_internal_member_is_default |   |   |
| ssz_types_internal_default_members |   |   |
| ssz_default_container |   |   |
| ssz_default_union |   |   |
| ssz_is_zero_vector_composite |   |   |
| ssz_is_zero_container |   |   |
| ssz_is_zero_union |   |   |
| ssz_error_string |   |   |
