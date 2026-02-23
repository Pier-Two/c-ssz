#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

    ASSERT_ERR(ssz_hash_2to1(&custom_success, &left, &right, &out), SSZ_SUCCESS);
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
    ASSERT_ERR(ssz_hash_2to1(&custom_fail, &left, &right, &out), SSZ_ERR_HASH_FAILURE);

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

    ASSERT_ERR(ssz_hash_2to1_batch(ssz_hash_default(), pairs_one, (SIZE_MAX / 2u) + 1u, out),
               SSZ_ERR_OVERFLOW);

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

int main(void)
{
    const test_case_t tests[] = {
        {"hash_sha256_known_vectors", test_hash_sha256_known_vectors},
        {"hash_2to1_matches_concat_sha256", test_hash_2to1_matches_concat_sha256},
        {"hash_default_provider", test_hash_default_provider},
        {"hash_2to1_batch_cases", test_hash_2to1_batch_cases},
        {"hash_2to1_null_hash_fn_fallback", test_hash_2to1_null_hash_fn_fallback},
        {"hash_sha256_error_paths", test_hash_sha256_error_paths},
        {"hash_2to1_custom_provider_and_error_normalization", test_hash_2to1_custom_provider_and_error_normalization},
        {"hash_2to1_batch_error_paths", test_hash_2to1_batch_error_paths},
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
