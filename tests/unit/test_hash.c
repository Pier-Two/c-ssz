#if !defined(_WIN32)
#include <pthread.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <openssl/sha.h>

#include "ssz.h"

typedef bool (*test_fn_t)(void);

typedef struct
{
    const char *name;
    test_fn_t fn;
} test_case_t;

#define ASSERT_TRUE(cond)                                                                            \
    do                                                                                               \
    {                                                                                                \
        if (!(cond))                                                                                 \
        {                                                                                            \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_ERR(expr, expected)                                                                   \
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

#define ASSERT_MEM_EQ(actual, expected, len)                                                         \
    do                                                                                               \
    {                                                                                                \
        if (memcmp((actual), (expected), (len)) != 0)                                               \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: memory mismatch (%s vs %s, len=%zu)\n",           \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    #actual,                                                                          \
                    #expected,                                                                        \
                    (size_t)(len));                                                                   \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

static ssz_chunk_t make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

typedef struct
{
    uint8_t fill;
    ssz_error_t err;
} hash_behavior_t;

typedef struct
{
    size_t call_count;
    size_t fail_on_call;
    ssz_error_t fail_err;
} counting_hash_t;

static ssz_error_t mock_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    const hash_behavior_t *behavior = (const hash_behavior_t *)ctx;
    (void)data;
    (void)data_len;
    if (out != NULL)
    {
        memset(out, (behavior != NULL) ? behavior->fill : 0u, 32u);
    }
    if (behavior == NULL)
    {
        return SSZ_SUCCESS;
    }
    return behavior->err;
}

static ssz_error_t mock_hash_2to1(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    const hash_behavior_t *behavior = (const hash_behavior_t *)ctx;
    (void)left;
    (void)right;
    if (out != NULL)
    {
        memset(out->bytes, (behavior != NULL) ? behavior->fill : 0u, SSZ_BYTES_PER_CHUNK);
    }
    if (behavior == NULL)
    {
        return SSZ_SUCCESS;
    }
    return behavior->err;
}

static ssz_error_t mock_hash_2to1_batch(
    const void *ctx,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    const hash_behavior_t *behavior = (const hash_behavior_t *)ctx;
    (void)pairs;
    if ((out != NULL) && (pair_count != 0u))
    {
        for (size_t i = 0u; i < pair_count; i++)
        {
            memset(out[i].bytes, (behavior != NULL) ? behavior->fill : 0u, SSZ_BYTES_PER_CHUNK);
        }
    }
    if (behavior == NULL)
    {
        return SSZ_SUCCESS;
    }
    return behavior->err;
}

static ssz_error_t counting_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    counting_hash_t *state = (counting_hash_t *)ctx;
    (void)data;
    (void)data_len;
    if (out != NULL)
    {
        memset(out, 0u, 32u);
    }
    if (state == NULL)
    {
        return SSZ_SUCCESS;
    }
    state->call_count++;
    if ((state->fail_on_call != 0u) && (state->call_count == state->fail_on_call))
    {
        return state->fail_err;
    }
    return SSZ_SUCCESS;
}

typedef struct
{
    bool sha256_init_should_fail;
#if !defined(_WIN32)
    bool pthread_once_should_fail;
#endif
} internal_hash_hook_state_t;

static internal_hash_hook_state_t g_internal_hash_hooks;

static void reset_internal_hash_hooks(void)
{
    memset(&g_internal_hash_hooks, 0, sizeof(g_internal_hash_hooks));
}

static int hook_sha256_init(SHA256_CTX *ctx)
{
    if (g_internal_hash_hooks.sha256_init_should_fail)
    {
        return 0;
    }
    return SHA256_Init(ctx);
}

#if !defined(_WIN32)
static int hook_pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    if (g_internal_hash_hooks.pthread_once_should_fail)
    {
        return -1;
    }
    return pthread_once(once_control, init_routine);
}
#endif

static const ssz_hash_fn_t *internal_copy_ssz_hash_default(void);

#define SHA256_Init hook_sha256_init
#if !defined(_WIN32)
#define pthread_once hook_pthread_once
#endif
#define ssz_hash_sha256 internal_copy_ssz_hash_sha256
#define ssz_hash_2to1 internal_copy_ssz_hash_2to1
#define ssz_hash_default internal_copy_ssz_hash_default
#define ssz_hash_2to1_batch internal_copy_ssz_hash_2to1_batch
#define ssz_hash_2to1_batch_raw internal_copy_ssz_hash_2to1_batch_raw
#define ssz_hash_2to1_batch_inplace internal_copy_ssz_hash_2to1_batch_inplace
#define ssz_hash_default_zero_hashes internal_copy_ssz_hash_default_zero_hashes
/* Include the source file directly to exercise the renamed portable fallback
   entry points while keeping coverage attributed to src/ssz_hash.c. */
#include "ssz_hash.c"
#undef SHA256_Init
#if !defined(_WIN32)
#undef pthread_once
#endif
#undef ssz_hash_sha256
#undef ssz_hash_2to1
#undef ssz_hash_default
#undef ssz_hash_2to1_batch
#undef ssz_hash_2to1_batch_raw
#undef ssz_hash_2to1_batch_inplace
#undef ssz_hash_default_zero_hashes

/* Force calls through the renamed entry points to stay out-of-line in
   coverage builds. Otherwise the O3 test build can inline the included copy
   and leave gcov with no function hits for src/ssz_hash.c. */
static ssz_error_t call_internal_copy_ssz_hash_sha256(
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    ssz_error_t (*volatile fn)(const uint8_t *, size_t, uint8_t[32]) = internal_copy_ssz_hash_sha256;
    return fn(data, data_len, out);
}

static const ssz_hash_fn_t *call_internal_copy_ssz_hash_default(void)
{
    const ssz_hash_fn_t *(*volatile fn)(void) = internal_copy_ssz_hash_default;
    return fn();
}

static ssz_error_t call_internal_copy_ssz_hash_2to1(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    ssz_error_t (*volatile fn)(
        const ssz_hash_fn_t *,
        const ssz_chunk_t *,
        const ssz_chunk_t *,
        ssz_chunk_t *) = internal_copy_ssz_hash_2to1;
    return fn(hash_fn, left, right, out);
}

static ssz_error_t call_internal_copy_ssz_hash_2to1_batch_raw(
    const ssz_hash_fn_t *hash_fn,
    const uint8_t *pairs64,
    size_t pair_count,
    ssz_chunk_t *out)
{
    ssz_error_t (*volatile fn)(
        const ssz_hash_fn_t *,
        const uint8_t *,
        size_t,
        ssz_chunk_t *) = internal_copy_ssz_hash_2to1_batch_raw;
    return fn(hash_fn, pairs64, pair_count, out);
}

static ssz_error_t call_internal_copy_ssz_hash_2to1_batch_inplace(
    const ssz_hash_fn_t *hash_fn,
    ssz_chunk_t *nodes,
    size_t pair_count)
{
    ssz_error_t (*volatile fn)(const ssz_hash_fn_t *, ssz_chunk_t *, size_t) =
        internal_copy_ssz_hash_2to1_batch_inplace;
    return fn(hash_fn, nodes, pair_count);
}

static ssz_error_t call_internal_copy_ssz_hash_2to1_batch(
    const ssz_hash_fn_t *hash_fn,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    ssz_error_t (*volatile fn)(
        const ssz_hash_fn_t *,
        const ssz_chunk_t *,
        size_t,
        ssz_chunk_t *) = internal_copy_ssz_hash_2to1_batch;
    return fn(hash_fn, pairs, pair_count, out);
}

static const ssz_chunk_t *call_internal_copy_ssz_hash_default_zero_hashes(void)
{
    const ssz_chunk_t *(*volatile fn)(void) = internal_copy_ssz_hash_default_zero_hashes;
    return fn();
}

static bool test_hash_sha256_known_vectors(void)
{
    uint8_t out[32] = {0u};

    ASSERT_ERR(ssz_hash_sha256(NULL, 0u, out), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out,
                  ((const uint8_t[32]){
                      0xE3u, 0xB0u, 0xC4u, 0x42u, 0x98u, 0xFCu, 0x1Cu, 0x14u,
                      0x9Au, 0xFBu, 0xF4u, 0xC8u, 0x99u, 0x6Fu, 0xB9u, 0x24u,
                      0x27u, 0xAEu, 0x41u, 0xE4u, 0x64u, 0x9Bu, 0x93u, 0x4Cu,
                      0xA4u, 0x95u, 0x99u, 0x1Bu, 0x78u, 0x52u, 0xB8u, 0x55u,
                  }),
                  32u);

    const uint8_t abc[] = {'a', 'b', 'c'};
    ASSERT_ERR(ssz_hash_sha256(abc, sizeof(abc), out), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out,
                  ((const uint8_t[32]){
                      0xBAu, 0x78u, 0x16u, 0xBFu, 0x8Fu, 0x01u, 0xCFu, 0xEAu,
                      0x41u, 0x41u, 0x40u, 0xDEu, 0x5Du, 0xAEu, 0x22u, 0x23u,
                      0xB0u, 0x03u, 0x61u, 0xA3u, 0x96u, 0x17u, 0x7Au, 0x9Cu,
                      0xB4u, 0x10u, 0xFFu, 0x61u, 0xF2u, 0x00u, 0x15u, 0xADu,
                  }),
                  32u);

    return true;
}

static bool test_hash_2to1_matches_concat_sha256(void)
{
    const ssz_chunk_t left = make_chunk(0x00u);
    const ssz_chunk_t right = make_chunk(0x20u);
    ssz_chunk_t out;
    uint8_t expected_bytes[32] = {0u};
    uint8_t concat[64] = {0u};

    memcpy(concat, left.bytes, SSZ_BYTES_PER_CHUNK);
    memcpy(concat + SSZ_BYTES_PER_CHUNK, right.bytes, SSZ_BYTES_PER_CHUNK);

    ASSERT_ERR(ssz_hash_sha256(concat, sizeof(concat), expected_bytes), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &left, &right, &out), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out.bytes, expected_bytes, sizeof(expected_bytes));

    return true;
}

static bool test_hash_default_provider(void)
{
    const ssz_hash_fn_t *provider = ssz_hash_default();
    ASSERT_TRUE(provider != NULL);
    ASSERT_TRUE(provider->hash != NULL);

    uint8_t out1[32] = {0u};
    uint8_t out2[32] = {0u};
    const uint8_t msg[] = {0x01u, 0x02u, 0x03u};

    ASSERT_ERR(provider->hash(provider->ctx, msg, sizeof(msg), out1), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_sha256(msg, sizeof(msg), out2), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out1, out2, sizeof(out1));

    return true;
}

static bool test_hash_default_zero_hashes_match_runtime_computation(void)
{
    const ssz_chunk_t *static_zero_hashes = ssz_hash_default_zero_hashes();
    const ssz_chunk_t *cached_zero_hashes = ssz_hash_default_zero_hashes();
    ssz_chunk_t runtime_zero_hashes[64];
    uint8_t pair[SSZ_BYTES_PER_CHUNK * 2u];

    ASSERT_TRUE(static_zero_hashes != NULL);
    ASSERT_TRUE(cached_zero_hashes == static_zero_hashes);

    memset(runtime_zero_hashes[0].bytes, 0u, SSZ_BYTES_PER_CHUNK);
    for (size_t depth = 1u; depth < 64u; depth++)
    {
        memcpy(pair, runtime_zero_hashes[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair + SSZ_BYTES_PER_CHUNK, runtime_zero_hashes[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        ASSERT_ERR(ssz_hash_sha256(pair, sizeof(pair), runtime_zero_hashes[depth].bytes), SSZ_SUCCESS);
    }

    for (size_t depth = 0u; depth < 64u; depth++)
    {
        ASSERT_MEM_EQ(static_zero_hashes[depth].bytes,
                      runtime_zero_hashes[depth].bytes,
                      SSZ_BYTES_PER_CHUNK);
    }

    return true;
}

static bool test_hash_internal_copy_sha256_and_zero_hashes_match_public(void)
{
    const uint8_t msg[] = {0xDEu, 0xADu, 0xBEu, 0xEFu, 0x55u};
    uint8_t copy_out[32] = {0u};
    uint8_t public_out[32] = {0u};
    const ssz_chunk_t *copy_zero_hashes = NULL;
    const ssz_chunk_t *copy_zero_hashes_cached = NULL;
    const ssz_chunk_t *public_zero_hashes = NULL;

    reset_internal_hash_hooks();

    ASSERT_ERR(call_internal_copy_ssz_hash_sha256(msg, sizeof(msg), copy_out), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_sha256(msg, sizeof(msg), public_out), SSZ_SUCCESS);
    ASSERT_MEM_EQ(copy_out, public_out, sizeof(copy_out));

    copy_zero_hashes = call_internal_copy_ssz_hash_default_zero_hashes();
    copy_zero_hashes_cached = call_internal_copy_ssz_hash_default_zero_hashes();
    public_zero_hashes = ssz_hash_default_zero_hashes();

    ASSERT_TRUE(copy_zero_hashes != NULL);
    ASSERT_TRUE(copy_zero_hashes_cached == copy_zero_hashes);
    ASSERT_TRUE(public_zero_hashes != NULL);

    for (size_t depth = 0u; depth < 64u; depth++)
    {
        ASSERT_MEM_EQ(copy_zero_hashes[depth].bytes, public_zero_hashes[depth].bytes, SSZ_BYTES_PER_CHUNK);
    }

    return true;
}

static bool test_hash_2to1_batch_cases(void)
{
    const ssz_chunk_t pairs_one[2] = {make_chunk(0x10u), make_chunk(0x20u)};
    ssz_chunk_t out_one[1];
    ssz_chunk_t expected_one;

    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), pairs_one, 1u, out_one), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs_one[0], &pairs_one[1], &expected_one), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out_one[0].bytes, expected_one.bytes, SSZ_BYTES_PER_CHUNK);

    const ssz_chunk_t pairs_many[6] = {
        make_chunk(0x01u), make_chunk(0x11u),
        make_chunk(0x21u), make_chunk(0x31u),
        make_chunk(0x41u), make_chunk(0x51u),
    };
    ssz_chunk_t out_many[3];
    ssz_chunk_t expected_many[3];

    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), pairs_many, 3u, out_many), SSZ_SUCCESS);

    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs_many[0], &pairs_many[1], &expected_many[0]),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs_many[2], &pairs_many[3], &expected_many[1]),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs_many[4], &pairs_many[5], &expected_many[2]),
               SSZ_SUCCESS);

    for (size_t i = 0u; i < 3u; i++)
    {
        ASSERT_MEM_EQ(out_many[i].bytes, expected_many[i].bytes, SSZ_BYTES_PER_CHUNK);
    }

    return true;
}

static bool test_hash_2to1_null_hash_fn_fallback(void)
{
    const ssz_chunk_t left = make_chunk(0x70u);
    const ssz_chunk_t right = make_chunk(0x90u);
    ssz_chunk_t out_null;
    ssz_chunk_t out_explicit;

    ASSERT_ERR(ssz_hash_2to1(NULL, &left, &right, &out_null), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &left, &right, &out_explicit), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out_null.bytes, out_explicit.bytes, SSZ_BYTES_PER_CHUNK);

    return true;
}

static bool test_hash_2to1_default_contiguous_fast_path_with_output_after_pair(void)
{
    ssz_chunk_t storage[3] = {
        make_chunk(0x11u),
        make_chunk(0x31u),
        make_chunk(0x00u),
    };
    ssz_chunk_t expected;

    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &storage[0], &storage[1], &expected), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &storage[0], &storage[1], &storage[2]), SSZ_SUCCESS);
    ASSERT_MEM_EQ(storage[2].bytes, expected.bytes, SSZ_BYTES_PER_CHUNK);

    return true;
}

static bool test_hash_internal_copy_portable_entry_points_match_public(void)
{
    const ssz_hash_fn_t *copy_default = call_internal_copy_ssz_hash_default();
    const ssz_hash_fn_t *public_default = ssz_hash_default();
    const ssz_chunk_t pairs[6] = {
        make_chunk(0x03u), make_chunk(0x13u),
        make_chunk(0x23u), make_chunk(0x33u),
        make_chunk(0x43u), make_chunk(0x53u),
    };
    ssz_chunk_t copy_single;
    ssz_chunk_t public_single;
    ssz_chunk_t copy_raw[2];
    ssz_chunk_t public_raw[2];
    ssz_chunk_t copy_batch[3];
    ssz_chunk_t public_batch[3];
    ssz_chunk_t copy_nodes[4] = {
        make_chunk(0x61u), make_chunk(0x71u),
        make_chunk(0x81u), make_chunk(0x91u),
    };
    ssz_chunk_t public_nodes[4] = {
        make_chunk(0x61u), make_chunk(0x71u),
        make_chunk(0x81u), make_chunk(0x91u),
    };

    ASSERT_TRUE(copy_default != NULL);
    ASSERT_TRUE(public_default != NULL);

    ASSERT_ERR(call_internal_copy_ssz_hash_2to1(copy_default, &pairs[0], &pairs[1], &copy_single),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(public_default, &pairs[0], &pairs[1], &public_single), SSZ_SUCCESS);
    ASSERT_MEM_EQ(copy_single.bytes, public_single.bytes, SSZ_BYTES_PER_CHUNK);

    ASSERT_ERR(call_internal_copy_ssz_hash_2to1_batch_raw(copy_default, (const uint8_t *)pairs, 2u, copy_raw),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1_batch_raw(public_default, (const uint8_t *)pairs, 2u, public_raw), SSZ_SUCCESS);
    for (size_t i = 0u; i < 2u; i++)
    {
        ASSERT_MEM_EQ(copy_raw[i].bytes, public_raw[i].bytes, SSZ_BYTES_PER_CHUNK);
    }

    ASSERT_ERR(call_internal_copy_ssz_hash_2to1_batch_inplace(copy_default, copy_nodes, 2u), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1_batch_inplace(public_default, public_nodes, 2u), SSZ_SUCCESS);
    for (size_t i = 0u; i < 2u; i++)
    {
        ASSERT_MEM_EQ(copy_nodes[i].bytes, public_nodes[i].bytes, SSZ_BYTES_PER_CHUNK);
    }

    ASSERT_ERR(call_internal_copy_ssz_hash_2to1_batch(copy_default, pairs, 3u, copy_batch), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1_batch(public_default, pairs, 3u, public_batch), SSZ_SUCCESS);
    for (size_t i = 0u; i < 3u; i++)
    {
        ASSERT_MEM_EQ(copy_batch[i].bytes, public_batch[i].bytes, SSZ_BYTES_PER_CHUNK);
    }

    return true;
}

static bool test_hash_sha256_error_paths(void)
{
    uint8_t out[32] = {0u};
    const uint8_t byte = 0xABu;

    ASSERT_ERR(ssz_hash_sha256(NULL, 1u, out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_sha256(&byte, 1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(ssz_hash_sha256(&byte, (size_t)UINT32_MAX + 1u, out), SSZ_ERR_OVERFLOW);
#endif

    return true;
}

static bool test_hash_2to1_custom_provider_and_error_normalization(void)
{
    const ssz_chunk_t left = make_chunk(0x01u);
    const ssz_chunk_t right = make_chunk(0x21u);
    ssz_chunk_t out;

    hash_behavior_t success_behavior = {
        .fill = 0x5Au,
        .err = SSZ_SUCCESS,
    };
    const ssz_hash_fn_t custom_success = {
        .hash = mock_hash,
        .hash_2to1 = mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &success_behavior,
    };
    const ssz_hash_fn_t hash_only_success = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &success_behavior,
    };

    ASSERT_ERR(ssz_hash_2to1(&custom_success, &left, &right, &out), SSZ_SUCCESS);
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(out.bytes[i] == 0x5Au);
    }
    ASSERT_ERR(ssz_hash_2to1(&hash_only_success, &left, &right, &out), SSZ_SUCCESS);
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(out.bytes[i] == 0x5Au);
    }

    hash_behavior_t fail_behavior = {
        .fill = 0u,
        .err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t custom_fail = {
        .hash = mock_hash,
        .hash_2to1 = mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &fail_behavior,
    };
    const ssz_hash_fn_t hash_only_fail = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &fail_behavior,
    };
    ASSERT_ERR(ssz_hash_2to1(&custom_fail, &left, &right, &out), SSZ_ERR_HASH_FAILURE);
    ASSERT_ERR(ssz_hash_2to1(&hash_only_fail, &left, &right, &out), SSZ_ERR_HASH_FAILURE);

    const ssz_hash_fn_t missing_hash = {
        .hash = NULL,
        .hash_2to1 = mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &success_behavior,
    };
    ASSERT_ERR(ssz_hash_2to1(&missing_hash, &left, &right, &out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1(&custom_success, NULL, &right, &out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1(&custom_success, &left, NULL, &out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1(&custom_success, &left, &right, NULL), SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_hash_2to1_batch_error_paths(void)
{
    const ssz_chunk_t pairs_one[2] = {make_chunk(0x10u), make_chunk(0x20u)};
    ssz_chunk_t out[2];

    const ssz_hash_fn_t missing_hash = {
        .hash = NULL,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    ASSERT_ERR(ssz_hash_2to1_batch(&missing_hash, pairs_one, 1u, out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), NULL, 1u, out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), pairs_one, 1u, NULL), SSZ_ERR_INVALID_ARGUMENT);

    hash_behavior_t batch_success_behavior = {
        .fill = 0xA5u,
        .err = SSZ_SUCCESS,
    };
    const ssz_hash_fn_t batch_success = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = mock_hash_2to1_batch,
        .ctx = &batch_success_behavior,
    };
    ASSERT_ERR(ssz_hash_2to1_batch(&batch_success, pairs_one, 1u, out), SSZ_SUCCESS);
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(out[0].bytes[i] == 0xA5u);
    }

    hash_behavior_t batch_fail_behavior = {
        .fill = 0u,
        .err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t batch_fail = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = mock_hash_2to1_batch,
        .ctx = &batch_fail_behavior,
    };
    ASSERT_ERR(ssz_hash_2to1_batch(&batch_fail, pairs_one, 1u, out), SSZ_ERR_HASH_FAILURE);

#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), pairs_one, (SIZE_MAX / 2u) + 1u, out),
               SSZ_ERR_OVERFLOW);
#endif

    counting_hash_t state = {
        .call_count = 0u,
        .fail_on_call = 1u,
        .fail_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t fallback_fail = {
        .hash = counting_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &state,
    };
    ASSERT_ERR(ssz_hash_2to1_batch(&fallback_fail, pairs_one, 1u, out), SSZ_ERR_HASH_FAILURE);

    return true;
}

static bool test_hash_2to1_batch_raw_paths(void)
{
    const ssz_chunk_t pairs[4] = {
        make_chunk(0x01u), make_chunk(0x21u),
        make_chunk(0x41u), make_chunk(0x61u),
    };
    ssz_chunk_t out[2];
    ssz_chunk_t expected[2];

    const ssz_hash_fn_t missing_hash = {
        .hash = NULL,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    hash_behavior_t custom_behavior = {
        .fill = 0x7Eu,
        .err = SSZ_SUCCESS,
    };
    const ssz_hash_fn_t custom_hash = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &custom_behavior,
    };

    ASSERT_ERR(ssz_hash_2to1_batch_raw(ssz_hash_default(), NULL, 0u, out), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1_batch_raw(ssz_hash_default(), (const uint8_t *)pairs, 2u, out), SSZ_SUCCESS);

    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs[0], &pairs[1], &expected[0]), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &pairs[2], &pairs[3], &expected[1]), SSZ_SUCCESS);
    ASSERT_MEM_EQ(out[0].bytes, expected[0].bytes, SSZ_BYTES_PER_CHUNK);
    ASSERT_MEM_EQ(out[1].bytes, expected[1].bytes, SSZ_BYTES_PER_CHUNK);

    ASSERT_ERR(ssz_hash_2to1_batch_raw(&custom_hash, (const uint8_t *)pairs, 2u, out), SSZ_SUCCESS);
    for (size_t i = 0u; i < 2u; i++)
    {
        for (size_t j = 0u; j < SSZ_BYTES_PER_CHUNK; j++)
        {
            ASSERT_TRUE(out[i].bytes[j] == 0x7Eu);
        }
    }

    ASSERT_ERR(ssz_hash_2to1_batch_raw(&missing_hash, (const uint8_t *)pairs, 1u, out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch_raw(ssz_hash_default(), NULL, 1u, out), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch_raw(ssz_hash_default(), (const uint8_t *)pairs, 1u, NULL),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch_raw(ssz_hash_default(),
                                       (const uint8_t *)pairs,
                                       (SIZE_MAX / (SSZ_BYTES_PER_CHUNK * 2u)) + 1u,
                                       out),
               SSZ_ERR_OVERFLOW);

    return true;
}

static bool test_hash_2to1_batch_inplace_cases(void)
{
    ssz_chunk_t nodes_default[4] = {
        make_chunk(0x10u), make_chunk(0x20u),
        make_chunk(0x30u), make_chunk(0x40u),
    };
    ssz_chunk_t expected_default[2];

    hash_behavior_t custom_behavior = {
        .fill = 0x3Cu,
        .err = SSZ_SUCCESS,
    };
    const ssz_hash_fn_t custom_hash = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &custom_behavior,
    };
    ssz_chunk_t nodes_custom[4] = {
        make_chunk(0x50u), make_chunk(0x60u),
        make_chunk(0x70u), make_chunk(0x80u),
    };

    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &nodes_default[0], &nodes_default[1], &expected_default[0]),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(ssz_hash_default(), &nodes_default[2], &nodes_default[3], &expected_default[1]),
               SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1_batch_inplace(ssz_hash_default(), nodes_default, 2u), SSZ_SUCCESS);
    ASSERT_MEM_EQ(nodes_default[0].bytes, expected_default[0].bytes, SSZ_BYTES_PER_CHUNK);
    ASSERT_MEM_EQ(nodes_default[1].bytes, expected_default[1].bytes, SSZ_BYTES_PER_CHUNK);

    ASSERT_ERR(ssz_hash_2to1_batch_inplace(&custom_hash, nodes_custom, 2u), SSZ_SUCCESS);
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(nodes_custom[0].bytes[i] == 0x3Cu);
        ASSERT_TRUE(nodes_custom[1].bytes[i] == 0x3Cu);
    }

    return true;
}

static bool test_hash_2to1_batch_inplace_error_paths(void)
{
    ssz_chunk_t nodes[2] = {
        make_chunk(0x91u),
        make_chunk(0xB1u),
    };

    const ssz_hash_fn_t missing_hash = {
        .hash = NULL,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    hash_behavior_t fail_behavior = {
        .fill = 0u,
        .err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t failing_hash = {
        .hash = mock_hash,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = &fail_behavior,
    };

    ASSERT_ERR(ssz_hash_2to1_batch_inplace(&missing_hash, nodes, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch_inplace(ssz_hash_default(), NULL, 1u), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_2to1_batch_inplace(ssz_hash_default(), nodes, (SIZE_MAX / 2u) + 1u), SSZ_ERR_OVERFLOW);
    ASSERT_ERR(ssz_hash_2to1_batch_inplace(&failing_hash, nodes, 1u), SSZ_ERR_HASH_FAILURE);

    return true;
}

static bool test_hash_2to1_batch_falls_back_to_hash_2to1_provider(void)
{
    const ssz_chunk_t pairs[4] = {
        make_chunk(0x02u), make_chunk(0x12u),
        make_chunk(0x22u), make_chunk(0x32u),
    };
    ssz_chunk_t out[2];

    hash_behavior_t success_behavior = {
        .fill = 0xC3u,
        .err = SSZ_SUCCESS,
    };
    const ssz_hash_fn_t success_hash = {
        .hash = mock_hash,
        .hash_2to1 = mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &success_behavior,
    };

    hash_behavior_t fail_behavior = {
        .fill = 0u,
        .err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t fail_hash = {
        .hash = mock_hash,
        .hash_2to1 = mock_hash_2to1,
        .hash_2to1_batch = NULL,
        .ctx = &fail_behavior,
    };

    ASSERT_ERR(ssz_hash_2to1_batch(&success_hash, pairs, 2u, out), SSZ_SUCCESS);
    for (size_t i = 0u; i < 2u; i++)
    {
        for (size_t j = 0u; j < SSZ_BYTES_PER_CHUNK; j++)
        {
            ASSERT_TRUE(out[i].bytes[j] == 0xC3u);
        }
    }

    ASSERT_ERR(ssz_hash_2to1_batch(&fail_hash, pairs, 1u, out), SSZ_ERR_HASH_FAILURE);

    return true;
}

static bool test_hash_internal_sha256_batch_default_defensive_paths(void)
{
    const ssz_chunk_t pairs[2] = {
        make_chunk(0xA0u),
        make_chunk(0xC0u),
    };
    ssz_chunk_t out[1];

    reset_internal_hash_hooks();

    ASSERT_ERR(ssz_internal_sha256_64_batch_default(NULL, 0u, NULL), SSZ_SUCCESS);
    ASSERT_ERR(ssz_internal_sha256_64_batch_default(NULL, 1u, out), SSZ_ERR_INVALID_ARGUMENT);

    g_internal_hash_hooks.sha256_init_should_fail = true;
    ASSERT_ERR(ssz_internal_sha256_64_batch_default((const uint8_t *)pairs, 1u, out), SSZ_ERR_HASH_FAILURE);

    reset_internal_hash_hooks();
    return true;
}

#if !defined(_WIN32)
static bool test_hash_default_zero_hashes_pthread_once_failure(void)
{
    reset_internal_hash_hooks();
    g_internal_hash_hooks.pthread_once_should_fail = true;

    const ssz_chunk_t *result = call_internal_copy_ssz_hash_default_zero_hashes();
    ASSERT_TRUE(result == NULL);

    reset_internal_hash_hooks();
    return true;
}
#endif

int main(void)
{
    const test_case_t tests[] = {
        {"hash_sha256_known_vectors", test_hash_sha256_known_vectors},
        {"hash_2to1_matches_concat_sha256", test_hash_2to1_matches_concat_sha256},
        {"hash_default_provider", test_hash_default_provider},
        {"hash_default_zero_hashes_match_runtime_computation",
         test_hash_default_zero_hashes_match_runtime_computation},
        {"hash_internal_copy_sha256_and_zero_hashes_match_public",
         test_hash_internal_copy_sha256_and_zero_hashes_match_public},
        {"hash_2to1_batch_cases", test_hash_2to1_batch_cases},
        {"hash_internal_copy_portable_entry_points_match_public",
         test_hash_internal_copy_portable_entry_points_match_public},
        {"hash_2to1_null_hash_fn_fallback", test_hash_2to1_null_hash_fn_fallback},
        {"hash_2to1_default_contiguous_fast_path_with_output_after_pair",
         test_hash_2to1_default_contiguous_fast_path_with_output_after_pair},
        {"hash_sha256_error_paths", test_hash_sha256_error_paths},
        {"hash_2to1_custom_provider_and_error_normalization", test_hash_2to1_custom_provider_and_error_normalization},
        {"hash_2to1_batch_error_paths", test_hash_2to1_batch_error_paths},
        {"hash_2to1_batch_raw_paths", test_hash_2to1_batch_raw_paths},
        {"hash_2to1_batch_inplace_cases", test_hash_2to1_batch_inplace_cases},
        {"hash_2to1_batch_inplace_error_paths", test_hash_2to1_batch_inplace_error_paths},
        {"hash_2to1_batch_falls_back_to_hash_2to1_provider", test_hash_2to1_batch_falls_back_to_hash_2to1_provider},
        {"hash_internal_sha256_batch_default_defensive_paths", test_hash_internal_sha256_batch_default_defensive_paths},
#if !defined(_WIN32)
        {"hash_default_zero_hashes_pthread_once_failure", test_hash_default_zero_hashes_pthread_once_failure},
#endif
    };

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

    printf("[OK] %zu/%zu hash tests passed\n", passed, total);
    return 0;
}
