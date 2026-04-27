#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssz.h"

#define FUZZ_STATEFUL_MAX_LEAVES 16u
#define FUZZ_STATEFUL_MAX_STEPS  96u

typedef struct
{
    const uint8_t *data;
    size_t size;
    size_t offset;
} fuzz_input_t;

typedef struct
{
    ssz_merkle_cache_t cache;
    ssz_merkle_cache_requirements_t requirements;
    ssz_merkle_cache_storage_t storage;
    ssz_chunk_t *nodes;
    uint64_t *leaf_dirty_bits;
    size_t *leaf_dirty_word_idx;
    uint64_t *parent_dirty_bits[2];
    size_t *parent_dirty_word_idx[2];
    ssz_chunk_t *gather_pairs;
    ssz_chunk_t *gather_hashes;
    size_t *gather_parent_indices;
} fuzz_fixture_t;

typedef struct
{
    ssz_chunk_t leaves[FUZZ_STATEFUL_MAX_LEAVES];
    uint64_t leaf_count;
    uint64_t leaf_limit;
    uint64_t reserved_leaf_capacity;
    bool mix_in_length;
} fuzz_model_t;

static void fuzz_fail(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    abort();
}

static uint8_t fuzz_take_u8(fuzz_input_t *input)
{
    uint8_t value = 0u;

    if ((input != NULL) && (input->offset < input->size))
    {
        value = input->data[input->offset];
        input->offset++;
    }

    return value;
}

static uint64_t fuzz_take_u64(fuzz_input_t *input)
{
    uint64_t value = 0u;

    for (size_t i = 0u; i < sizeof(value); i++)
    {
        value |= ((uint64_t)fuzz_take_u8(input)) << (8u * i);
    }

    return value;
}

static uint64_t fuzz_take_u64_bounded(fuzz_input_t *input, uint64_t max_inclusive)
{
    uint64_t value = fuzz_take_u64(input);

    if (max_inclusive != 0u)
    {
        value %= (max_inclusive + 1u);
    }
    else
    {
        value = 0u;
    }

    return value;
}

static void fuzz_fill_chunk(fuzz_input_t *input, ssz_chunk_t *out)
{
    if (out == NULL)
    {
        fuzz_fail("null chunk");
    }

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        out->bytes[i] = fuzz_take_u8(input);
    }
}

static ssz_chunk_t fuzz_seed_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }

    return chunk;
}

static void *fuzz_alloc(size_t count, size_t element_size)
{
    if (count == 0u)
    {
        return NULL;
    }

    return calloc(count, element_size);
}

static void fuzz_fixture_cleanup(fuzz_fixture_t *fixture)
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
    (void)memset(fixture, 0, sizeof(*fixture));
}

static ssz_merkle_cache_config_t fuzz_config(
    uint64_t initial_leaf_count,
    uint64_t leaf_limit,
    uint64_t reserved_leaf_capacity,
    bool mix_in_length)
{
    ssz_merkle_cache_config_t config;

    (void)memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.initial_leaf_count = initial_leaf_count;
    config.leaf_limit = leaf_limit;
    config.reserved_leaf_capacity = reserved_leaf_capacity;
    config.logical_length = initial_leaf_count;
    config.mix_in_length = mix_in_length;
    config.hash_fn = NULL;

    return config;
}

static bool fuzz_fixture_alloc(
    fuzz_fixture_t *fixture,
    const ssz_merkle_cache_config_t *config,
    bool bind_cache)
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

    fixture->nodes = fuzz_alloc(fixture->requirements.nodes_count, sizeof(*fixture->nodes));
    fixture->leaf_dirty_bits =
        fuzz_alloc(fixture->requirements.leaf_dirty_words, sizeof(*fixture->leaf_dirty_bits));
    fixture->leaf_dirty_word_idx = fuzz_alloc(
        fixture->requirements.leaf_dirty_words,
        sizeof(*fixture->leaf_dirty_word_idx));
    fixture->parent_dirty_bits[0] = fuzz_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_bits[0]));
    fixture->parent_dirty_bits[1] = fuzz_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_bits[1]));
    fixture->parent_dirty_word_idx[0] = fuzz_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_word_idx[0]));
    fixture->parent_dirty_word_idx[1] = fuzz_alloc(
        fixture->requirements.parent_dirty_words,
        sizeof(*fixture->parent_dirty_word_idx[1]));
    fixture->gather_pairs =
        fuzz_alloc(fixture->requirements.gather_pairs_count, sizeof(*fixture->gather_pairs));
    fixture->gather_hashes =
        fuzz_alloc(fixture->requirements.gather_hashes_count, sizeof(*fixture->gather_hashes));
    fixture->gather_parent_indices = fuzz_alloc(
        fixture->requirements.gather_parent_indices_count,
        sizeof(*fixture->gather_parent_indices));

    if (((fixture->requirements.nodes_count != 0u) && (fixture->nodes == NULL)) ||
        ((fixture->requirements.leaf_dirty_words != 0u) &&
         ((fixture->leaf_dirty_bits == NULL) || (fixture->leaf_dirty_word_idx == NULL))) ||
        ((fixture->requirements.parent_dirty_words != 0u) &&
         ((fixture->parent_dirty_bits[0] == NULL) || (fixture->parent_dirty_bits[1] == NULL) ||
          (fixture->parent_dirty_word_idx[0] == NULL) ||
          (fixture->parent_dirty_word_idx[1] == NULL))) ||
        ((fixture->requirements.gather_pairs_count != 0u) &&
         ((fixture->gather_pairs == NULL) || (fixture->gather_parent_indices == NULL))) ||
        ((fixture->requirements.gather_hashes_count != 0u) && (fixture->gather_hashes == NULL)))
    {
        fuzz_fixture_cleanup(fixture);
        return false;
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
    fixture->storage.gather_parent_indices_count = fixture->requirements.gather_parent_indices_count;

    if (bind_cache)
    {
        err = ssz_merkle_cache_bind(config, &fixture->storage, &fixture->cache);
        if (err != SSZ_SUCCESS)
        {
            fuzz_fixture_cleanup(fixture);
            return false;
        }
    }

    return true;
}

static uint64_t fuzz_model_capacity(const fuzz_model_t *model)
{
    if (model->leaf_limit == SSZ_NO_LIMIT)
    {
        return model->reserved_leaf_capacity;
    }

    return model->leaf_limit;
}

static void fuzz_sync_logical_length(fuzz_fixture_t *fixture, const fuzz_model_t *model)
{
    ssz_error_t err = ssz_merkle_cache_set_logical_length(&fixture->cache, model->leaf_count);

    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("set_logical_length failed");
    }
}

static void fuzz_assert_model(fuzz_fixture_t *fixture, const fuzz_model_t *model)
{
    ssz_chunk_t expected_data_root;
    ssz_chunk_t expected_root;
    ssz_chunk_t actual_data_root;
    ssz_chunk_t actual_root;
    ssz_error_t err = SSZ_SUCCESS;

    err = ssz_merkleize(
        model->leaves,
        (size_t)model->leaf_count,
        model->leaf_limit,
        NULL,
        NULL,
        &expected_data_root);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("shadow merkleize failed");
    }

    if (model->mix_in_length)
    {
        err = ssz_mix_in_length_u64(&expected_data_root, model->leaf_count, NULL, &expected_root);
        if (err != SSZ_SUCCESS)
        {
            fuzz_fail("shadow mix_in_length failed");
        }
    }
    else
    {
        expected_root = expected_data_root;
    }

    err = ssz_merkle_cache_data_root(&fixture->cache, &actual_data_root);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("cache data root failed");
    }
    if (memcmp(&expected_data_root, &actual_data_root, sizeof(expected_data_root)) != 0)
    {
        fuzz_fail("cache data root mismatch");
    }

    err = ssz_merkle_cache_root(&fixture->cache, &actual_root);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("cache root failed");
    }
    if (memcmp(&expected_root, &actual_root, sizeof(expected_root)) != 0)
    {
        fuzz_fail("cache root mismatch");
    }

    err = ssz_merkle_cache_root(&fixture->cache, &actual_root);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("cached root failed");
    }
    if (memcmp(&expected_root, &actual_root, sizeof(expected_root)) != 0)
    {
        fuzz_fail("cached root mismatch");
    }
}

static void fuzz_apply_update(fuzz_input_t *input, fuzz_fixture_t *fixture, fuzz_model_t *model)
{
    ssz_chunk_t roots[4];
    uint64_t capacity = fuzz_model_capacity(model);
    uint64_t start = fuzz_take_u64_bounded(input, capacity + 2u);
    uint64_t count = fuzz_take_u64_bounded(input, 4u);
    uint64_t end = start + count;
    ssz_error_t err = SSZ_SUCCESS;

    for (size_t i = 0u; i < 4u; i++)
    {
        fuzz_fill_chunk(input, &roots[i]);
    }

    err = ssz_merkle_cache_update_root_range(
        &fixture->cache,
        start,
        (count == 0u) ? NULL : roots,
        count);

    if ((count == 0u) && (err != SSZ_SUCCESS))
    {
        fuzz_fail("empty update failed");
    }
    if ((count != 0u) && (end <= capacity))
    {
        if (err != SSZ_SUCCESS)
        {
            fuzz_fail("valid update failed");
        }
        for (uint64_t i = 0u; i < count; i++)
        {
            model->leaves[start + i] = roots[i];
        }
        if (end > model->leaf_count)
        {
            model->leaf_count = end;
        }
    }
    else if ((count != 0u) && (err == SSZ_SUCCESS))
    {
        fuzz_fail("out-of-capacity update succeeded");
    }

    fuzz_sync_logical_length(fixture, model);
    fuzz_assert_model(fixture, model);
}

static void fuzz_apply_zero_range(fuzz_input_t *input, fuzz_fixture_t *fixture, fuzz_model_t *model)
{
    ssz_chunk_t zero;
    uint64_t capacity = fuzz_model_capacity(model);
    uint64_t start = fuzz_take_u64_bounded(input, capacity + 2u);
    uint64_t count = fuzz_take_u64_bounded(input, 4u);
    uint64_t end = start + count;
    uint64_t old_leaf_count = model->leaf_count;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&zero, 0, sizeof(zero));
    err = ssz_merkle_cache_zero_range(&fixture->cache, start, count);

    if ((count == 0u) && (err != SSZ_SUCCESS))
    {
        fuzz_fail("empty zero_range failed");
    }
    if ((count != 0u) && (end <= capacity))
    {
        if (err != SSZ_SUCCESS)
        {
            fuzz_fail("valid zero_range failed");
        }
        for (uint64_t i = start; i < end; i++)
        {
            model->leaves[i] = zero;
        }
        if ((start < old_leaf_count) && (end >= old_leaf_count))
        {
            model->leaf_count = start;
        }
    }
    else if ((count != 0u) && (err == SSZ_SUCCESS))
    {
        fuzz_fail("out-of-capacity zero_range succeeded");
    }

    fuzz_sync_logical_length(fixture, model);
    fuzz_assert_model(fixture, model);
}

static void fuzz_apply_truncate(fuzz_input_t *input, fuzz_fixture_t *fixture, fuzz_model_t *model)
{
    ssz_chunk_t zero;
    uint64_t new_count = fuzz_take_u64_bounded(input, model->leaf_count);
    uint64_t old_count = model->leaf_count;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&zero, 0, sizeof(zero));
    err = ssz_merkle_cache_zero_range(&fixture->cache, new_count, old_count - new_count);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("truncate zero_range failed");
    }

    for (uint64_t i = new_count; i < old_count; i++)
    {
        model->leaves[i] = zero;
    }
    model->leaf_count = new_count;

    fuzz_sync_logical_length(fixture, model);
    fuzz_assert_model(fixture, model);
}

static void fuzz_apply_migrate(fuzz_input_t *input, fuzz_fixture_t *fixture, fuzz_model_t *model)
{
    static const uint64_t unbounded_reserved[] = {1u, 2u, 4u, 8u, 16u};
    fuzz_fixture_t dst;
    ssz_merkle_cache_config_t config;
    uint64_t reserved_leaf_capacity = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&dst, 0, sizeof(dst));
    if (model->leaf_limit == SSZ_NO_LIMIT)
    {
        reserved_leaf_capacity =
            unbounded_reserved[fuzz_take_u64_bounded(input, 4u)];
    }
    else
    {
        reserved_leaf_capacity = fuzz_take_u64_bounded(input, 20u);
    }

    fuzz_sync_logical_length(fixture, model);
    config = fuzz_config(0u, model->leaf_limit, reserved_leaf_capacity, model->mix_in_length);
    if (!fuzz_fixture_alloc(&dst, &config, false))
    {
        fuzz_assert_model(fixture, model);
        return;
    }

    err = ssz_merkle_cache_migrate_into(&fixture->cache, &config, &dst.storage, &dst.cache);
    if (err == SSZ_SUCCESS)
    {
        fuzz_fixture_cleanup(fixture);
        *fixture = dst;
        model->reserved_leaf_capacity = reserved_leaf_capacity;
    }
    else
    {
        fuzz_fixture_cleanup(&dst);
    }

    fuzz_sync_logical_length(fixture, model);
    fuzz_assert_model(fixture, model);
}

static void fuzz_apply_reset(fuzz_fixture_t *fixture, fuzz_model_t *model)
{
    ssz_chunk_t zero;
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&zero, 0, sizeof(zero));
    err = ssz_merkle_cache_reset(&fixture->cache);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("reset failed");
    }

    for (size_t i = 0u; i < FUZZ_STATEFUL_MAX_LEAVES; i++)
    {
        model->leaves[i] = zero;
    }
    model->leaf_count = 0u;
    fuzz_assert_model(fixture, model);
}

static void fuzz_run_random_sequence(fuzz_input_t *input)
{
    static const uint64_t unbounded_reserved[] = {1u, 2u, 4u, 8u, 16u};
    static const uint64_t bounded_limits[] = {1u, 2u, 3u, 4u, 8u, 16u};
    fuzz_fixture_t fixture;
    fuzz_model_t model;
    ssz_merkle_cache_config_t config;
    uint64_t capacity = 0u;
    uint64_t steps = 0u;

    (void)memset(&fixture, 0, sizeof(fixture));
    (void)memset(&model, 0, sizeof(model));

    model.mix_in_length = (fuzz_take_u8(input) & 1u) != 0u;
    if ((fuzz_take_u8(input) & 1u) != 0u)
    {
        model.leaf_limit = SSZ_NO_LIMIT;
        model.reserved_leaf_capacity =
            unbounded_reserved[fuzz_take_u64_bounded(input, 4u)];
    }
    else
    {
        model.leaf_limit = bounded_limits[fuzz_take_u64_bounded(input, 5u)];
        model.reserved_leaf_capacity = fuzz_take_u64_bounded(input, 20u);
    }

    capacity = fuzz_model_capacity(&model);
    model.leaf_count = fuzz_take_u64_bounded(input, capacity);
    config = fuzz_config(
        model.leaf_count,
        model.leaf_limit,
        model.reserved_leaf_capacity,
        model.mix_in_length);
    if (!fuzz_fixture_alloc(&fixture, &config, true))
    {
        return;
    }

    fuzz_assert_model(&fixture, &model);
    steps = 1u + fuzz_take_u64_bounded(input, FUZZ_STATEFUL_MAX_STEPS - 1u);
    for (uint64_t step = 0u; step < steps; step++)
    {
        switch (fuzz_take_u8(input) % 6u)
        {
        case 0u:
            fuzz_apply_update(input, &fixture, &model);
            break;
        case 1u:
            fuzz_apply_zero_range(input, &fixture, &model);
            break;
        case 2u:
            fuzz_apply_migrate(input, &fixture, &model);
            break;
        case 3u:
            fuzz_apply_truncate(input, &fixture, &model);
            break;
        case 4u:
            fuzz_sync_logical_length(&fixture, &model);
            fuzz_assert_model(&fixture, &model);
            break;
        default:
            fuzz_apply_reset(&fixture, &model);
            break;
        }
    }

    fuzz_fixture_cleanup(&fixture);
}

static void fuzz_directed_migration_zero_append(void)
{
    fuzz_fixture_t fixture;
    fuzz_model_t model;
    ssz_merkle_cache_config_t config = fuzz_config(0u, SSZ_NO_LIMIT, 2u, false);
    ssz_chunk_t zero;
    ssz_chunk_t roots[2];
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&fixture, 0, sizeof(fixture));
    (void)memset(&model, 0, sizeof(model));
    (void)memset(&zero, 0, sizeof(zero));
    model.leaf_limit = SSZ_NO_LIMIT;
    model.reserved_leaf_capacity = 2u;
    model.mix_in_length = false;

    if (!fuzz_fixture_alloc(&fixture, &config, true))
    {
        fuzz_fail("directed source init failed");
    }

    roots[0] = fuzz_seed_chunk(0x11u);
    roots[1] = fuzz_seed_chunk(0x22u);
    err = ssz_merkle_cache_update_root_range(&fixture.cache, 0u, roots, 2u);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("directed source update failed");
    }
    model.leaves[0] = roots[0];
    model.leaves[1] = roots[1];
    model.leaf_count = 2u;
    fuzz_assert_model(&fixture, &model);

    {
        fuzz_fixture_t dst;
        ssz_merkle_cache_config_t dst_config = fuzz_config(0u, SSZ_NO_LIMIT, 4u, false);

        (void)memset(&dst, 0, sizeof(dst));
        if (!fuzz_fixture_alloc(&dst, &dst_config, false))
        {
            fuzz_fail("directed destination init failed");
        }
        err = ssz_merkle_cache_migrate_into(&fixture.cache, &dst_config, &dst.storage, &dst.cache);
        if (err != SSZ_SUCCESS)
        {
            fuzz_fail("directed migrate failed");
        }
        fuzz_fixture_cleanup(&fixture);
        fixture = dst;
        model.reserved_leaf_capacity = 4u;
    }

    fuzz_assert_model(&fixture, &model);
    err = ssz_merkle_cache_update_root_range(&fixture.cache, 2u, &zero, 1u);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("directed zero append failed");
    }
    model.leaves[2] = zero;
    model.leaf_count = 3u;
    fuzz_assert_model(&fixture, &model);
    fuzz_fixture_cleanup(&fixture);
}

static void fuzz_directed_empty_update(void)
{
    fuzz_fixture_t fixture;
    fuzz_model_t model;
    ssz_merkle_cache_config_t config = fuzz_config(0u, SSZ_NO_LIMIT, 4u, false);
    ssz_chunk_t leaf = fuzz_seed_chunk(0x33u);
    ssz_error_t err = SSZ_SUCCESS;

    (void)memset(&fixture, 0, sizeof(fixture));
    (void)memset(&model, 0, sizeof(model));
    model.leaf_limit = SSZ_NO_LIMIT;
    model.reserved_leaf_capacity = 4u;
    model.mix_in_length = false;

    if (!fuzz_fixture_alloc(&fixture, &config, true))
    {
        fuzz_fail("directed empty-update init failed");
    }

    err = ssz_merkle_cache_update_root_range(&fixture.cache, 0u, &leaf, 1u);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("directed initial update failed");
    }
    model.leaves[0] = leaf;
    model.leaf_count = 1u;
    fuzz_assert_model(&fixture, &model);

    err = ssz_merkle_cache_update_root_range(&fixture.cache, 3u, NULL, 0u);
    if (err != SSZ_SUCCESS)
    {
        fuzz_fail("directed empty update failed");
    }
    fuzz_assert_model(&fixture, &model);
    fuzz_fixture_cleanup(&fixture);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    fuzz_input_t input = {
        .data = data,
        .size = size,
        .offset = 0u,
    };

    fuzz_directed_migration_zero_append();
    fuzz_directed_empty_update();
    fuzz_run_random_sequence(&input);

    return 0;
}
