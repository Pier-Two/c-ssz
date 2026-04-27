#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ssz.h"

typedef struct
{
    ssz_merkle_cache_t cache;
    ssz_merkle_cache_requirements_t requirements;
    ssz_merkle_cache_storage_t storage;
    ssz_merkle_cache_sync_workspace_t workspace;
    ssz_chunk_t *nodes;
    uint64_t *leaf_dirty_bits;
    size_t *leaf_dirty_word_idx;
    uint64_t *parent_dirty_bits[2];
    size_t *parent_dirty_word_idx[2];
    ssz_chunk_t *gather_pairs;
    ssz_chunk_t *gather_hashes;
    size_t *gather_parent_indices;
    uint64_t *token_values;
    uint64_t *token_valid_bits;
    ssz_chunk_t *root_batch_roots;
} fuzz_cache_fixture_t;

typedef struct
{
    const ssz_chunk_t *roots;
    const uint64_t *tokens;
    uint64_t count;
    uint64_t fail_member;
    ssz_error_t fail_err;
} fuzz_composite_ctx_t;

static void *fuzz_cache_alloc(size_t count, size_t element_size)
{
    if (count == 0u)
    {
        return NULL;
    }
    return calloc(count, element_size);
}

static void fuzz_cache_cleanup(fuzz_cache_fixture_t *fixture)
{
    if (fixture == NULL)
    {
        return;
    }

    free(fixture->nodes);
    free(fixture->leaf_dirty_bits);
    free(fixture->leaf_dirty_word_idx);
    free(fixture->parent_dirty_bits[0]);
    free(fixture->parent_dirty_bits[1]);
    free(fixture->parent_dirty_word_idx[0]);
    free(fixture->parent_dirty_word_idx[1]);
    free(fixture->gather_pairs);
    free(fixture->gather_hashes);
    free(fixture->gather_parent_indices);
    free(fixture->token_values);
    free(fixture->token_valid_bits);
    free(fixture->root_batch_roots);
    (void)memset(fixture, 0, sizeof(*fixture));
}

static ssz_merkle_cache_config_t fuzz_cache_config(
    uint64_t initial_leaf_count,
    uint64_t leaf_limit,
    uint64_t reserved_leaf_capacity,
    uint64_t logical_length,
    bool mix_in_length,
    const ssz_hash_fn_t *hash_fn)
{
    ssz_merkle_cache_config_t config;

    (void)memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.initial_leaf_count = initial_leaf_count;
    config.leaf_limit = leaf_limit;
    config.reserved_leaf_capacity = reserved_leaf_capacity;
    config.logical_length = logical_length;
    config.mix_in_length = mix_in_length;
    config.hash_fn = hash_fn;
    return config;
}

static bool fuzz_cache_init(
    fuzz_cache_fixture_t *fixture,
    const ssz_merkle_cache_config_t *config,
    bool with_tokens)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((fixture == NULL) || (config == NULL))
    {
        return false;
    }

    (void)memset(fixture, 0, sizeof(*fixture));
    err = ssz_merkle_cache_requirements(config, &fixture->requirements);
    if (err != SSZ_SUCCESS)
    {
        return false;
    }

    fixture->nodes = fuzz_cache_alloc(fixture->requirements.nodes_count, sizeof(*fixture->nodes));
    fixture->leaf_dirty_bits =
        fuzz_cache_alloc(fixture->requirements.leaf_dirty_words, sizeof(*fixture->leaf_dirty_bits));
    fixture->leaf_dirty_word_idx = fuzz_cache_alloc(
        fixture->requirements.leaf_dirty_words,
        sizeof(*fixture->leaf_dirty_word_idx));
    fixture->parent_dirty_bits[0] = fuzz_cache_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_bits[0]));
    fixture->parent_dirty_bits[1] = fuzz_cache_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_bits[1]));
    fixture->parent_dirty_word_idx[0] = fuzz_cache_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_word_idx[0]));
    fixture->parent_dirty_word_idx[1] = fuzz_cache_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_word_idx[1]));
    fixture->gather_pairs =
        fuzz_cache_alloc(fixture->requirements.gather_pairs_count, sizeof(*fixture->gather_pairs));
    fixture->gather_hashes =
        fuzz_cache_alloc(fixture->requirements.gather_hashes_count, sizeof(*fixture->gather_hashes));
    fixture->gather_parent_indices = fuzz_cache_alloc(
        fixture->requirements.gather_parent_indices_count,
        sizeof(*fixture->gather_parent_indices));
    fixture->root_batch_roots = fuzz_cache_alloc(
        fixture->requirements.root_batch_roots_count,
        sizeof(*fixture->root_batch_roots));

    if (with_tokens)
    {
        fixture->token_values =
            fuzz_cache_alloc(fixture->requirements.token_values_count, sizeof(*fixture->token_values));
        fixture->token_valid_bits = fuzz_cache_alloc(
            fixture->requirements.token_valid_words,
            sizeof(*fixture->token_valid_bits));
    }

    fixture->storage.struct_size = sizeof(fixture->storage);
    fixture->storage.nodes = fixture->nodes;
    fixture->storage.nodes_count = fixture->requirements.nodes_count;
    fixture->storage.leaf_dirty_bits = fixture->leaf_dirty_bits;
    fixture->storage.leaf_dirty_words = fixture->requirements.leaf_dirty_words;
    fixture->storage.leaf_dirty_word_idx = fixture->leaf_dirty_word_idx;
    fixture->storage.leaf_dirty_word_idx_count = fixture->requirements.leaf_dirty_words;
    fixture->storage.parent_dirty_bits[0] = fixture->parent_dirty_bits[0];
    fixture->storage.parent_dirty_bits[1] = fixture->parent_dirty_bits[1];
    fixture->storage.parent_dirty_words = fixture->requirements.parent_dirty_words;
    fixture->storage.parent_dirty_word_idx[0] = fixture->parent_dirty_word_idx[0];
    fixture->storage.parent_dirty_word_idx[1] = fixture->parent_dirty_word_idx[1];
    fixture->storage.parent_dirty_word_idx_count = fixture->requirements.parent_dirty_words;
    fixture->storage.gather_pairs = fixture->gather_pairs;
    fixture->storage.gather_pairs_count = fixture->requirements.gather_pairs_count;
    fixture->storage.gather_hashes = fixture->gather_hashes;
    fixture->storage.gather_hashes_count = fixture->requirements.gather_hashes_count;
    fixture->storage.gather_parent_indices = fixture->gather_parent_indices;
    fixture->storage.gather_parent_indices_count =
        fixture->requirements.gather_parent_indices_count;
    fixture->storage.token_values = fixture->token_values;
    fixture->storage.token_values_count =
        with_tokens ? fixture->requirements.token_values_count : 0u;
    fixture->storage.token_valid_bits = fixture->token_valid_bits;
    fixture->storage.token_valid_words =
        with_tokens ? fixture->requirements.token_valid_words : 0u;

    fixture->workspace.struct_size = sizeof(fixture->workspace);
    fixture->workspace.root_batch_roots = fixture->root_batch_roots;
    fixture->workspace.root_batch_roots_count = fixture->requirements.root_batch_roots_count;

    err = ssz_merkle_cache_bind(config, &fixture->storage, &fixture->cache);
    if (err != SSZ_SUCCESS)
    {
        fuzz_cache_cleanup(fixture);
        return false;
    }

    return true;
}

static ssz_chunk_t fuzz_cache_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_error_t fuzz_cache_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    return ssz_hash_sha256(data, data_len, out);
}

static ssz_error_t fuzz_cache_composite_root(
    const void *ctx,
    uint64_t member_id,
    ssz_chunk_t *out_root)
{
    const fuzz_composite_ctx_t *state = (const fuzz_composite_ctx_t *)ctx;

    if ((state == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == state->fail_member)
    {
        return state->fail_err;
    }
    if (member_id >= state->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_root = state->roots[member_id];
    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_cache_composite_root_batch(
    const void *ctx,
    uint64_t start_index,
    uint64_t count,
    ssz_chunk_t *out_roots)
{
    const fuzz_composite_ctx_t *state = (const fuzz_composite_ctx_t *)ctx;

    if ((state == NULL) || (out_roots == NULL) || ((start_index + count) > state->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((state->fail_member >= start_index) && (state->fail_member < (start_index + count)))
    {
        return state->fail_err;
    }

    for (uint64_t i = 0u; i < count; i++)
    {
        out_roots[i] = state->roots[start_index + i];
    }
    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_cache_composite_token(
    const void *ctx,
    uint64_t member_id,
    uint64_t *out_token)
{
    const fuzz_composite_ctx_t *state = (const fuzz_composite_ctx_t *)ctx;

    if ((state == NULL) || (out_token == NULL) || (state->tokens == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == state->fail_member)
    {
        return state->fail_err;
    }
    if (member_id >= state->count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_token = state->tokens[member_id];
    return SSZ_SUCCESS;
}

static void fuzz_cover_cache_invalid_inputs(void)
{
    ssz_merkle_cache_requirements_t requirements;
    ssz_merkle_cache_config_t invalid_config =
        fuzz_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);
    ssz_merkle_cache_t cache;

    invalid_config.struct_size = 0u;
    (void)ssz_merkle_cache_requirements(NULL, &requirements);
    (void)ssz_merkle_cache_requirements(&invalid_config, &requirements);
    (void)ssz_merkle_cache_bind(NULL, NULL, &cache);
    (void)ssz_merkle_cache_reset(NULL);
    (void)ssz_merkle_cache_data_root(NULL, NULL);
    (void)ssz_merkle_cache_root(NULL, NULL);
    (void)ssz_merkle_cache_update_root_range(NULL, 0u, NULL, 0u);
    (void)ssz_merkle_cache_zero_range(NULL, 0u, 0u);
    (void)ssz_merkle_cache_set_logical_length(NULL, 0u);
    (void)ssz_merkle_cache_sync_packed_bytes(NULL, NULL, 0u, 0u);
    (void)ssz_merkle_cache_sync_packed_vector_fixed(NULL, NULL, 0u, 0u);
    (void)ssz_merkle_cache_sync_packed_list_fixed(NULL, NULL, 0u, 0u, 0u);
    (void)ssz_merkle_cache_sync_bitvector(NULL, NULL, 0u, 0u);
    (void)ssz_merkle_cache_sync_bitlist(NULL, NULL, 0u, 0u, 0u);
    (void)ssz_merkle_cache_sync_composite(NULL, 0u, 0u, NULL, NULL);
    (void)ssz_merkle_cache_needs_resync(NULL);
}

static void fuzz_cover_cache_root_updates(void)
{
    fuzz_cache_fixture_t fixture;
    ssz_chunk_t leaves[4] = {
        fuzz_cache_chunk(0x10u),
        fuzz_cache_chunk(0x20u),
        fuzz_cache_chunk(0x30u),
        fuzz_cache_chunk(0x40u),
    };
    ssz_chunk_t out_root;
    ssz_merkle_cache_config_t config =
        fuzz_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, true, NULL);

    if (!fuzz_cache_init(&fixture, &config, true))
    {
        return;
    }

    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 0u, leaves, 2u);
    (void)ssz_merkle_cache_set_logical_length(&fixture.cache, 2u);
    (void)ssz_merkle_cache_data_root(&fixture.cache, &out_root);
    (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
    (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
    (void)ssz_merkle_cache_zero_range(&fixture.cache, 1u, 1u);
    (void)ssz_merkle_cache_zero_range(&fixture.cache, 0u, 0u);
    (void)ssz_merkle_cache_data_root(&fixture.cache, &out_root);
    (void)ssz_merkle_cache_reset(&fixture.cache);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 2u, &leaves[2], 1u);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 3u, &leaves[3], 1u);
    (void)ssz_merkle_cache_data_root(&fixture.cache, &out_root);

    fuzz_cache_cleanup(&fixture);
}

static void fuzz_cover_cache_packed_sync(void)
{
    uint8_t bytes[80] = {0u};
    uint8_t valid_bits[2] = {0x55u, 0x01u};
    uint8_t invalid_bits[2] = {0x55u, 0xC1u};
    ssz_chunk_t out_root;

    for (size_t i = 0u; i < sizeof(bytes); i++)
    {
        bytes[i] = (uint8_t)(0x80u + (uint8_t)i);
    }

    {
        fuzz_cache_fixture_t fixture;
        ssz_merkle_cache_config_t config =
            fuzz_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, NULL);
        if (fuzz_cache_init(&fixture, &config, false))
        {
            (void)ssz_merkle_cache_sync_packed_bytes(&fixture.cache, bytes, 57u, 57u);
            (void)ssz_merkle_cache_sync_packed_bytes(&fixture.cache, bytes, 10u, 10u);
            (void)ssz_merkle_cache_sync_packed_vector_fixed(&fixture.cache, bytes, 4u, 8u);
            (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
            fuzz_cache_cleanup(&fixture);
        }
    }

    {
        fuzz_cache_fixture_t fixture;
        ssz_merkle_cache_config_t config = fuzz_cache_config(0u, 2u, 0u, 0u, true, NULL);
        if (fuzz_cache_init(&fixture, &config, false))
        {
            (void)ssz_merkle_cache_sync_packed_list_fixed(&fixture.cache, bytes, 8u, 64u, 1u);
            (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
            fuzz_cache_cleanup(&fixture);
        }
    }

    {
        fuzz_cache_fixture_t fixture;
        ssz_merkle_cache_config_t config = fuzz_cache_config(0u, 1u, 0u, 0u, false, NULL);
        if (fuzz_cache_init(&fixture, &config, false))
        {
            (void)ssz_merkle_cache_sync_bitvector(&fixture.cache, valid_bits, sizeof(valid_bits), 10u);
            (void)ssz_merkle_cache_sync_bitvector(&fixture.cache, invalid_bits, sizeof(invalid_bits), 10u);
            (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
            fuzz_cache_cleanup(&fixture);
        }
    }

    {
        fuzz_cache_fixture_t fixture;
        ssz_merkle_cache_config_t config = fuzz_cache_config(0u, 1u, 0u, 0u, true, NULL);
        if (fuzz_cache_init(&fixture, &config, false))
        {
            (void)ssz_merkle_cache_sync_bitlist(&fixture.cache, valid_bits, sizeof(valid_bits), 10u, 10u);
            (void)ssz_merkle_cache_sync_bitlist(&fixture.cache, invalid_bits, sizeof(invalid_bits), 9u, 10u);
            (void)ssz_merkle_cache_root(&fixture.cache, &out_root);
            fuzz_cache_cleanup(&fixture);
        }
    }
}

static void fuzz_cover_cache_custom_hash_and_dirty_words(void)
{
    const ssz_hash_fn_t hash_fn = {
        .hash = fuzz_cache_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    fuzz_cache_fixture_t fixture;
    ssz_merkle_cache_config_t config =
        fuzz_cache_config(0u, SSZ_NO_LIMIT, 256u, 0u, false, &hash_fn);
    ssz_chunk_t leaves[256];
    ssz_chunk_t out_root;

    if (!fuzz_cache_init(&fixture, &config, false))
    {
        return;
    }

    for (size_t i = 0u; i < 256u; i++)
    {
        leaves[i] = fuzz_cache_chunk((uint8_t)i);
    }
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 0u, leaves, 256u);
    (void)ssz_merkle_cache_data_root(&fixture.cache, &out_root);

    leaves[1] = fuzz_cache_chunk(0xA1u);
    leaves[65] = fuzz_cache_chunk(0xA2u);
    leaves[129] = fuzz_cache_chunk(0xA3u);
    leaves[193] = fuzz_cache_chunk(0xA4u);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 1u, &leaves[1], 1u);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 65u, &leaves[65], 1u);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 129u, &leaves[129], 1u);
    (void)ssz_merkle_cache_update_root_range(&fixture.cache, 193u, &leaves[193], 1u);
    (void)ssz_merkle_cache_data_root(&fixture.cache, &out_root);

    fuzz_cache_cleanup(&fixture);
}

static void fuzz_cover_cache_composite_sync(void)
{
    fuzz_cache_fixture_t fixture;
    ssz_chunk_t roots[4] = {
        fuzz_cache_chunk(0x31u),
        fuzz_cache_chunk(0x32u),
        fuzz_cache_chunk(0x33u),
        fuzz_cache_chunk(0x34u),
    };
    uint64_t tokens[4] = {11u, 22u, 33u, 44u};
    fuzz_composite_ctx_t composite = {
        .roots = roots,
        .tokens = tokens,
        .count = 4u,
        .fail_member = UINT64_MAX,
        .fail_err = SSZ_ERR_HASH_FAILURE,
    };
    ssz_member_codec_t codec = {
        .ctx = &composite,
        .write = NULL,
        .read = NULL,
        .root = fuzz_cache_composite_root,
    };
    ssz_merkle_cache_config_t config = fuzz_cache_config(0u, 4u, 0u, 0u, true, NULL);
    ssz_merkle_cache_sync_composite_opts_t opts;
    ssz_chunk_t out_root;

    if (!fuzz_cache_init(&fixture, &config, true))
    {
        return;
    }

    (void)memset(&opts, 0, sizeof(opts));
    opts.struct_size = sizeof(opts);
    opts.ctx = &composite;
    opts.token = fuzz_cache_composite_token;
    opts.root_batch = fuzz_cache_composite_root_batch;
    opts.workspace = &fixture.workspace;

    (void)ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts);
    (void)ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts);
    roots[1] = fuzz_cache_chunk(0xB2u);
    tokens[1] = 55u;
    (void)ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, &opts);
    (void)ssz_merkle_cache_root(&fixture.cache, &out_root);

    opts.token = NULL;
    (void)ssz_merkle_cache_sync_composite(&fixture.cache, 2u, 4u, &codec, &opts);

    composite.fail_member = 1u;
    (void)ssz_merkle_cache_sync_composite(&fixture.cache, 3u, 4u, &codec, NULL);
    (void)ssz_merkle_cache_needs_resync(&fixture.cache);

    fuzz_cache_cleanup(&fixture);
}

static void fuzz_cover_cache_migration(void)
{
    const ssz_hash_fn_t hash_fn = {
        .hash = fuzz_cache_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    fuzz_cache_fixture_t small_fixture;
    fuzz_cache_fixture_t large_fixture;
    ssz_merkle_cache_config_t small_config =
        fuzz_cache_config(0u, SSZ_NO_LIMIT, 4u, 0u, false, &hash_fn);
    ssz_merkle_cache_config_t large_config =
        fuzz_cache_config(0u, SSZ_NO_LIMIT, 8u, 0u, false, &hash_fn);
    ssz_chunk_t leaves[4] = {
        fuzz_cache_chunk(0x41u),
        fuzz_cache_chunk(0x42u),
        fuzz_cache_chunk(0x43u),
        fuzz_cache_chunk(0x44u),
    };
    ssz_chunk_t out_root;

    if (!fuzz_cache_init(&small_fixture, &small_config, false))
    {
        return;
    }
    if (!fuzz_cache_init(&large_fixture, &large_config, false))
    {
        fuzz_cache_cleanup(&small_fixture);
        return;
    }

    (void)ssz_merkle_cache_update_root_range(&small_fixture.cache, 0u, leaves, 4u);
    (void)ssz_merkle_cache_data_root(&small_fixture.cache, &out_root);
    (void)ssz_merkle_cache_migrate_into(
        &small_fixture.cache,
        &large_config,
        &large_fixture.storage,
        &large_fixture.cache);
    (void)ssz_merkle_cache_data_root(&large_fixture.cache, &out_root);

    fuzz_cache_cleanup(&small_fixture);
    fuzz_cache_cleanup(&large_fixture);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static bool covered = false;

    (void)data;
    (void)size;

    if (!covered)
    {
        covered = true;
        fuzz_cover_cache_invalid_inputs();
        fuzz_cover_cache_root_updates();
        fuzz_cover_cache_packed_sync();
        fuzz_cover_cache_custom_hash_and_dirty_words();
        fuzz_cover_cache_composite_sync();
        fuzz_cover_cache_migration();
    }

    return 0;
}
