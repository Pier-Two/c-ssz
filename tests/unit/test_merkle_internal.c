#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssz.h"
#include "ssz_internal.h"

typedef bool (*internal_test_fn_t)(void);

typedef struct
{
    const char *name;
    internal_test_fn_t fn;
} internal_test_case_t;

#define IASSERT_TRUE(cond)                                                                           \
    do                                                                                               \
    {                                                                                                \
        if (!(cond))                                                                                 \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define IASSERT_ERR(expr, expected)                                                                  \
    do                                                                                               \
    {                                                                                                \
        ssz_error_t _actual = (expr);                                                                \
        if (_actual != (expected))                                                                   \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: %s returned %s (%d), expected %s (%d)\n",         \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    #expr,                                                                            \
                    ssz_error_string(_actual),                                                        \
                    (int)_actual,                                                                     \
                    ssz_error_string((expected)),                                                     \
                    (int)(expected));                                                                 \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define IASSERT_CHUNK_EQ(actual, expected)                                                           \
    do                                                                                               \
    {                                                                                                \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) != 0)                    \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: chunk mismatch\n", __FILE__, __LINE__);   \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

typedef struct
{
    size_t malloc_calls;
    size_t malloc_fail_at;
    size_t u64_to_size_calls;
    size_t u64_to_size_fail_at;
    size_t add_overflow_size_calls;
    size_t add_overflow_size_fail_at;
    size_t mul_overflow_size_calls;
    size_t mul_overflow_size_fail_at;
    size_t add_overflow_u64_calls;
    size_t add_overflow_u64_fail_at;
    size_t mul_overflow_u64_calls;
    size_t mul_overflow_u64_fail_at;
    size_t hash_2to1_calls;
    size_t hash_2to1_fail_at;
    ssz_error_t hash_2to1_fail_err;
    size_t hash_2to1_batch_calls;
    size_t hash_2to1_batch_fail_at;
    ssz_error_t hash_2to1_batch_fail_err;
    size_t hash_2to1_batch_raw_calls;
    size_t hash_2to1_batch_raw_fail_at;
    ssz_error_t hash_2to1_batch_raw_fail_err;
    size_t hash_2to1_batch_inplace_calls;
    size_t hash_2to1_batch_inplace_fail_at;
    ssz_error_t hash_2to1_batch_inplace_fail_err;
    bool return_null_zero_hashes;
} hook_state_t;

static hook_state_t g_hooks;

static void reset_hooks(void)
{
    memset(&g_hooks, 0, sizeof(g_hooks));
    g_hooks.hash_2to1_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_raw_fail_err = SSZ_ERR_HASH_FAILURE;
    g_hooks.hash_2to1_batch_inplace_fail_err = SSZ_ERR_HASH_FAILURE;
}

static bool hook_should_fail(size_t *counter, size_t fail_at)
{
    (*counter)++;
    return (fail_at != 0u) && (*counter == fail_at);
}

static bool hook_u64_to_size(uint64_t value, size_t *out)
{
    if (hook_should_fail(&g_hooks.u64_to_size_calls, g_hooks.u64_to_size_fail_at))
    {
        return false;
    }
    return ssz_internal_u64_to_size(value, out);
}

static bool hook_add_overflow_size(size_t a, size_t b, size_t *out)
{
    if (hook_should_fail(&g_hooks.add_overflow_size_calls, g_hooks.add_overflow_size_fail_at))
    {
        return true;
    }
    return ssz_internal_add_overflow_size(a, b, out);
}

static bool hook_mul_overflow_size(size_t a, size_t b, size_t *out)
{
    if (hook_should_fail(&g_hooks.mul_overflow_size_calls, g_hooks.mul_overflow_size_fail_at))
    {
        return true;
    }
    return ssz_internal_mul_overflow_size(a, b, out);
}

static bool hook_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (hook_should_fail(&g_hooks.add_overflow_u64_calls, g_hooks.add_overflow_u64_fail_at))
    {
        return true;
    }
    return ssz_internal_add_overflow_u64(a, b, out);
}

static bool hook_mul_overflow_u64(uint64_t a, uint64_t b, uint64_t *out)
{
    if (hook_should_fail(&g_hooks.mul_overflow_u64_calls, g_hooks.mul_overflow_u64_fail_at))
    {
        return true;
    }
    return ssz_internal_mul_overflow_u64(a, b, out);
}

static ssz_error_t hook_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    if (hook_should_fail(&g_hooks.hash_2to1_calls, g_hooks.hash_2to1_fail_at))
    {
        return g_hooks.hash_2to1_fail_err;
    }
    return ssz_hash_2to1(hash_fn, left, right, out);
}

static ssz_error_t hook_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    if (hook_should_fail(&g_hooks.hash_2to1_batch_calls, g_hooks.hash_2to1_batch_fail_at))
    {
        return g_hooks.hash_2to1_batch_fail_err;
    }
    return ssz_hash_2to1_batch(hash_fn, pairs, pair_count, out);
}

static ssz_error_t hook_hash_2to1_batch_raw(
    const ssz_hash_fn_t *hash_fn,
    const uint8_t *pairs64,
    size_t pair_count,
    ssz_chunk_t *out)
{
    if (hook_should_fail(&g_hooks.hash_2to1_batch_raw_calls, g_hooks.hash_2to1_batch_raw_fail_at))
    {
        return g_hooks.hash_2to1_batch_raw_fail_err;
    }
    return ssz_hash_2to1_batch_raw(hash_fn, pairs64, pair_count, out);
}

static ssz_error_t hook_hash_2to1_batch_inplace(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *nodes,
    size_t pair_count)
{
    if (hook_should_fail(&g_hooks.hash_2to1_batch_inplace_calls, g_hooks.hash_2to1_batch_inplace_fail_at))
    {
        return g_hooks.hash_2to1_batch_inplace_fail_err;
    }
    return ssz_hash_2to1_batch_inplace(hash_fn, nodes, pair_count);
}

static const ssz_chunk_t *hook_default_zero_hashes(void)
{
    if (g_hooks.return_null_zero_hashes)
    {
        return NULL;
    }
    return ssz_hash_default_zero_hashes();
}

#define ssz_internal_u64_to_size hook_u64_to_size
#define ssz_internal_add_overflow_size hook_add_overflow_size
#define ssz_internal_mul_overflow_size hook_mul_overflow_size
#define ssz_internal_add_overflow_u64 hook_add_overflow_u64
#define ssz_internal_mul_overflow_u64 hook_mul_overflow_u64
#define ssz_hash_2to1 hook_hash_2to1
#define ssz_hash_2to1_batch hook_hash_2to1_batch
#define ssz_hash_2to1_batch_raw hook_hash_2to1_batch_raw
#define ssz_hash_2to1_batch_inplace hook_hash_2to1_batch_inplace
#define ssz_hash_default_zero_hashes hook_default_zero_hashes
#include "ssz_merkle.c"
#undef ssz_internal_u64_to_size
#undef ssz_internal_add_overflow_size
#undef ssz_internal_mul_overflow_size
#undef ssz_internal_add_overflow_u64
#undef ssz_internal_mul_overflow_u64
#undef ssz_hash_2to1
#undef ssz_hash_2to1_batch
#undef ssz_hash_2to1_batch_raw
#undef ssz_hash_2to1_batch_inplace
#undef ssz_hash_default_zero_hashes

#define main merkle_public_main
#include "test_merkle.c"
#undef main

typedef struct
{
    uint64_t fail_index;
    ssz_error_t fail_err;
    bool fail_enabled;
} reader_ctx_t;

static ssz_chunk_t internal_make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_error_t sequence_reader(const void *ctx, uint64_t index, ssz_chunk_t *out_leaf)
{
    const reader_ctx_t *cfg = (const reader_ctx_t *)ctx;

    if (out_leaf == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((cfg != NULL) && cfg->fail_enabled && (index == cfg->fail_index))
    {
        return cfg->fail_err;
    }

    *out_leaf = internal_make_chunk((uint8_t)index);
    return SSZ_SUCCESS;
}

static ssz_internal_leaf_source_t make_sequence_source(const reader_ctx_t *ctx)
{
    ssz_internal_leaf_source_t source;

    (void)memset(&source, 0, sizeof(source));
    source.kind = SSZ_INTERNAL_LEAF_SOURCE_CUSTOM;
    source.custom_reader = sequence_reader;
    source.custom_ctx = ctx;
    return source;
}

static ssz_internal_leaf_source_t make_chunk_source(const ssz_chunk_t *chunks, uint64_t count)
{
    ssz_internal_leaf_source_t source;

    ssz_internal_init_chunk_source(&source, chunks, count);
    return source;
}

static ssz_internal_leaf_source_t make_bytes_source(const uint8_t *bytes, size_t byte_len, uint64_t chunk_count)
{
    ssz_internal_leaf_source_t source;

    ssz_internal_init_bytes_source(&source, bytes, byte_len, chunk_count);
    return source;
}

static bool test_internal_alloc_and_leaf_readers(void)
{
    ssz_chunk_t leaf = internal_make_chunk(0xA0u);
    ssz_chunk_t out_leaf;
    const ssz_chunk_t chunks[1] = {leaf};
    const uint8_t bytes[3] = {0x11u, 0x22u, 0x33u};
    ssz_chunk_t scratch_chunks[2];
    ssz_chunk_t *scratch_view = NULL;

    reset_hooks();

    IASSERT_TRUE(!ssz_internal_scratch_is_invalid(NULL));
    IASSERT_TRUE(!ssz_internal_scratch_is_invalid(&(const ssz_merkle_scratch_t){
        .chunks = NULL,
        .chunk_count = 0u,
    }));
    IASSERT_TRUE(ssz_internal_scratch_is_invalid(&(const ssz_merkle_scratch_t){
        .chunks = NULL,
        .chunk_count = 1u,
    }));
    IASSERT_ERR(ssz_internal_get_scratch_chunks(NULL, 1u, &scratch_view), SSZ_ERR_BUFFER_TOO_SMALL);
    IASSERT_ERR(ssz_internal_get_scratch_chunks(
                    &(const ssz_merkle_scratch_t){
                        .chunks = scratch_chunks,
                        .chunk_count = 2u,
                    },
                    1u,
                    &scratch_view),
                SSZ_SUCCESS);
    IASSERT_TRUE(scratch_view == scratch_chunks);

    IASSERT_ERR(ssz_internal_read_chunk_leaf(NULL, 0u, &out_leaf), SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_chunk_leaf(&(ssz_internal_chunk_reader_ctx_t){.chunks = chunks, .count = 1u},
                                             0u,
                                             NULL),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_chunk_leaf(&(ssz_internal_chunk_reader_ctx_t){.chunks = NULL, .count = 1u},
                                             0u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_chunk_leaf(&(ssz_internal_chunk_reader_ctx_t){.chunks = chunks, .count = 1u},
                                             1u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_chunk_leaf(&(ssz_internal_chunk_reader_ctx_t){.chunks = chunks, .count = 1u},
                                             0u,
                                             &out_leaf),
                SSZ_SUCCESS);
    IASSERT_CHUNK_EQ(out_leaf, leaf);

    IASSERT_ERR(ssz_internal_read_bytes_leaf(NULL, 0u, &out_leaf), SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_bytes_leaf(&(ssz_internal_bytes_reader_ctx_t){.bytes = bytes, .byte_len = sizeof(bytes), .chunk_count = 1u},
                                             0u,
                                             NULL),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_bytes_leaf(&(ssz_internal_bytes_reader_ctx_t){.bytes = bytes, .byte_len = sizeof(bytes), .chunk_count = 1u},
                                             1u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_bytes_leaf(&(ssz_internal_bytes_reader_ctx_t){.bytes = NULL, .byte_len = sizeof(bytes), .chunk_count = 1u},
                                             0u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_bytes_leaf(&(ssz_internal_bytes_reader_ctx_t){
                                                .bytes = bytes,
                                                .byte_len = 0u,
                                                .chunk_count = UINT64_MAX,
                                            },
                                             (UINT64_MAX / SSZ_BYTES_PER_CHUNK) + 1u,
                                             &out_leaf),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_read_bytes_leaf(&(ssz_internal_bytes_reader_ctx_t){.bytes = bytes, .byte_len = sizeof(bytes), .chunk_count = 1u},
                                             0u,
                                             &out_leaf),
                SSZ_SUCCESS);
    IASSERT_TRUE(out_leaf.bytes[0] == 0x11u);
    IASSERT_TRUE(out_leaf.bytes[1] == 0x22u);
    IASSERT_TRUE(out_leaf.bytes[2] == 0x33u);
    IASSERT_TRUE(out_leaf.bytes[3] == 0u);

    IASSERT_ERR(ssz_internal_read_codec_leaf(NULL, 0u, &out_leaf), SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_codec_leaf(&(ssz_internal_codec_reader_ctx_t){.codec = NULL, .count = 1u},
                                             0u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_codec_leaf(&(ssz_internal_codec_reader_ctx_t){
                                                .codec = &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL},
                                                .count = 1u,
                                            },
                                             0u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_codec_leaf(&(ssz_internal_codec_reader_ctx_t){
                                                .codec = &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = sequence_reader},
                                                .count = 1u,
                                            },
                                             1u,
                                             &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_read_codec_leaf(&(ssz_internal_codec_reader_ctx_t){
                                                .codec = &(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = sequence_reader},
                                                .count = 1u,
                                            },
                                             0u,
                                             &out_leaf),
                SSZ_SUCCESS);

    IASSERT_ERR(ssz_internal_byte_len_to_chunk_count(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_merkleize_packed_bytes(NULL, 1u, SSZ_NO_LIMIT, NULL, NULL, &out_leaf),
                SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_internal_fast_merkleize_paths(void)
{
    ssz_chunk_t out_root;
    const ssz_chunk_t *zero_hashes = ssz_hash_default_zero_hashes();
    ssz_chunk_t chunk_leaves[128];
    ssz_chunk_t scratch_chunks[128];
    uint8_t bytes[128u * SSZ_BYTES_PER_CHUNK];
    const ssz_merkle_scratch_t full_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 128u,
    };
    const ssz_merkle_scratch_t half_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 64u,
    };
    const ssz_merkle_scratch_t short_chunk_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 63u,
    };
    const ssz_merkle_scratch_t short_full_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 127u,
    };
    ssz_internal_leaf_source_t sequence_source = make_sequence_source(NULL);
    ssz_internal_leaf_source_t chunk_source = make_chunk_source(chunk_leaves, 128u);
    ssz_internal_leaf_source_t bytes_source = make_bytes_source(bytes, sizeof(bytes), 128u);
    ssz_internal_leaf_source_t failing_sequence_source = make_sequence_source(&(const reader_ctx_t){
        .fail_index = 1u,
        .fail_err = SSZ_ERR_TYPE_MISMATCH,
        .fail_enabled = true,
    });

    for (size_t i = 0u; i < 128u; i++)
    {
        chunk_leaves[i] = internal_make_chunk((uint8_t)i);
        memcpy(bytes + (i * SSZ_BYTES_PER_CHUNK), chunk_leaves[i].bytes, SSZ_BYTES_PER_CHUNK);
    }

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, 0u, 1u, 1u, NULL, NULL, zero_hashes, &out_root),
                SSZ_ERR_OVERFLOW);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, 0u, 0u, 0u, NULL, NULL, zero_hashes, &out_root),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, UINT64_MAX, 1u, 1u, NULL, NULL, zero_hashes, &out_root),
                SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 3u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, 0u, 1u, 1u, NULL, NULL, zero_hashes, &out_root),
                SSZ_ERR_OVERFLOW);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, 0u, 1u, 2u, NULL, NULL, zero_hashes, &out_root),
                SSZ_SUCCESS);

    reset_hooks();
    g_hooks.hash_2to1_batch_inplace_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(&sequence_source, 0u, 2u, 2u, NULL, NULL, zero_hashes, &out_root),
                SSZ_ERR_HASH_FAILURE);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &chunk_source,
                    0u,
                    128u,
                    128u,
                    &half_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_SUCCESS);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &chunk_source,
                    0u,
                    128u,
                    128u,
                    &short_chunk_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_BUFFER_TOO_SMALL);

    reset_hooks();
    g_hooks.hash_2to1_batch_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &chunk_source,
                    0u,
                    128u,
                    128u,
                    &half_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_HASH_FAILURE);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &chunk_source,
                    128u,
                    128u,
                    128u,
                    &half_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_INVALID_ARGUMENT);

    reset_hooks();
    g_hooks.mul_overflow_size_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &bytes_source,
                    0u,
                    128u,
                    128u,
                    &half_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_BUFFER_TOO_SMALL);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &bytes_source,
                    0u,
                    128u,
                    128u,
                    &short_chunk_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_BUFFER_TOO_SMALL);

    reset_hooks();
    g_hooks.hash_2to1_batch_raw_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &bytes_source,
                    0u,
                    128u,
                    128u,
                    &half_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_HASH_FAILURE);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &sequence_source,
                    0u,
                    128u,
                    128u,
                    &short_full_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_BUFFER_TOO_SMALL);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &failing_sequence_source,
                    0u,
                    128u,
                    128u,
                    &full_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_TYPE_MISMATCH);

    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &sequence_source,
                    0u,
                    127u,
                    128u,
                    &full_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_SUCCESS);

    reset_hooks();
    g_hooks.hash_2to1_batch_inplace_fail_at = 1u;
    IASSERT_ERR(ssz_internal_merkleize_reader_fast(
                    &sequence_source,
                    0u,
                    127u,
                    128u,
                    &full_scratch,
                    NULL,
                    zero_hashes,
                    &out_root),
                SSZ_ERR_HASH_FAILURE);

    return true;
}

static bool test_internal_subtree_progressive_and_public_overflows(void)
{
    ssz_chunk_t out_root;
    const ssz_chunk_t *zero_hashes = ssz_hash_default_zero_hashes();
    const uint8_t one_byte = 0x01u;
    const ssz_chunk_t root = internal_make_chunk(0x55u);
    ssz_chunk_t scratch_chunks[128];
    const ssz_merkle_scratch_t scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 128u,
    };
    ssz_internal_leaf_source_t sequence_source = make_sequence_source(NULL);
    ssz_internal_leaf_source_t failing_sequence_source = make_sequence_source(&(const reader_ctx_t){
        .fail_index = 2u,
        .fail_err = SSZ_ERR_TYPE_MISMATCH,
        .fail_enabled = true,
    });
    const ssz_hash_fn_t invalid_hash = {
        .hash = NULL,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };

    IASSERT_ERR(ssz_internal_merkleize_subtree(&sequence_source,
                                               UINT64_MAX,
                                               2u,
                                               1u,
                                               1u,
                                               0u,
                                               &scratch,
                                               NULL,
                                               zero_hashes,
                                               &out_root),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_merkleize_subtree(&sequence_source,
                                               UINT64_MAX,
                                               3u,
                                               1u,
                                               2u,
                                               1u,
                                               &scratch,
                                               NULL,
                                               zero_hashes,
                                               &out_root),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_merkleize_subtree(&sequence_source,
                                               0u,
                                               UINT64_MAX,
                                               UINT64_MAX - 1u,
                                               4u,
                                               2u,
                                               &scratch,
                                               NULL,
                                               zero_hashes,
                                               &out_root),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_merkleize_subtree(&failing_sequence_source,
                                               0u,
                                               3u,
                                               0u,
                                               4u,
                                               2u,
                                               &scratch,
                                               NULL,
                                               zero_hashes,
                                               &out_root),
                SSZ_ERR_TYPE_MISMATCH);

    IASSERT_ERR(ssz_internal_merkleize_reader(NULL, 0u, 0u, 0u, NULL, NULL, &out_root), SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_merkleize_reader(&sequence_source, 0u, 0u, 0u, NULL, &invalid_hash, &out_root),
                SSZ_ERR_INVALID_ARGUMENT);

    reset_hooks();
    g_hooks.return_null_zero_hashes = true;
    IASSERT_ERR(ssz_internal_merkleize_reader(&sequence_source, 0u, 0u, 0u, NULL, NULL, &out_root),
                SSZ_ERR_HASH_FAILURE);

    reset_hooks();
    IASSERT_ERR(ssz_internal_merkleize_progressive_reader(NULL, 0u, 0u, 1u, NULL, NULL, &out_root),
                SSZ_ERR_INVALID_ARGUMENT);
    IASSERT_ERR(ssz_internal_merkleize_progressive_reader(
                    &sequence_source,
                    UINT64_MAX,
                    1u,
                    2u,
                    &scratch,
                    NULL,
                    &out_root),
                SSZ_ERR_OVERFLOW);
    IASSERT_ERR(ssz_internal_merkleize_progressive_reader(&sequence_source,
                                                          0u,
                                                          1u,
                                                          (UINT64_MAX / 4u) + 1u,
                                                          &scratch,
                                                          NULL,
                                                          &out_root),
                SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_vector_fixed(&one_byte, 1u, 1u, NULL, &out_root), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_vector_roots(&root, 1u, NULL, &out_root), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_list_fixed(&one_byte, 1u, SSZ_NO_LIMIT, 1u, NULL, &out_root),
                SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_list_roots(&root, 1u, SSZ_NO_LIMIT, NULL, &out_root), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_progressive_list_fixed(&one_byte, 1u, 1u, NULL, &out_root),
                SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.add_overflow_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_progressive_bitlist(&one_byte, 1u, 1u, NULL, &out_root), SSZ_ERR_OVERFLOW);

    reset_hooks();
    g_hooks.u64_to_size_fail_at = 1u;
    IASSERT_ERR(ssz_hash_tree_root_progressive_list_roots(&root, 1u, NULL, &out_root), SSZ_ERR_OVERFLOW);

    return true;
}

int main(void)
{
    const internal_test_case_t tests[] = {
        {"internal_alloc_and_leaf_readers", test_internal_alloc_and_leaf_readers},
        {"internal_fast_merkleize_paths", test_internal_fast_merkleize_paths},
        {"internal_subtree_progressive_and_public_overflows", test_internal_subtree_progressive_and_public_overflows},
    };

    if (merkle_public_main() != 0)
    {
        return 1;
    }

    size_t passed = 0u;
    const size_t total = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0u; i < total; i++)
    {
        if (!tests[i].fn())
        {
            fprintf(stderr, "[FAIL] %s\n", tests[i].name);
            return 1;
        }
        passed++;
    }

    printf("[OK] %zu/%zu merkle_internal tests passed\n", passed, total);
    return 0;
}
