#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

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

typedef struct
{
#if defined(_WIN32)
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE cond;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    size_t waiting;
    size_t generation;
    size_t participant_count;
} test_barrier_t;

typedef struct
{
    test_barrier_t *barrier;
    const ssz_chunk_t *zero_hashes;
    ssz_chunk_t merkle_root;
    ssz_error_t merkle_err;
} worker_ctx_t;

#if defined(_WIN32)
typedef HANDLE test_thread_t;
#else
typedef pthread_t test_thread_t;
#endif

static bool test_barrier_init(test_barrier_t *barrier, size_t participant_count)
{
    if (barrier == NULL)
    {
        return false;
    }

#if defined(_WIN32)
    InitializeCriticalSection(&barrier->mutex);
    InitializeConditionVariable(&barrier->cond);
#else
    if (pthread_mutex_init(&barrier->mutex, NULL) != 0)
    {
        return false;
    }
    if (pthread_cond_init(&barrier->cond, NULL) != 0)
    {
        (void)pthread_mutex_destroy(&barrier->mutex);
        return false;
    }
#endif

    barrier->waiting = 0u;
    barrier->generation = 0u;
    barrier->participant_count = participant_count;
    return true;
}

static void test_barrier_destroy(test_barrier_t *barrier)
{
    if (barrier == NULL)
    {
        return;
    }

#if defined(_WIN32)
    DeleteCriticalSection(&barrier->mutex);
#else
    (void)pthread_cond_destroy(&barrier->cond);
    (void)pthread_mutex_destroy(&barrier->mutex);
#endif
}

static void test_barrier_wait(test_barrier_t *barrier)
{
    size_t generation = 0u;

    if (barrier == NULL)
    {
        return;
    }

#if defined(_WIN32)
    EnterCriticalSection(&barrier->mutex);
    generation = barrier->generation;
    barrier->waiting++;

    if (barrier->waiting == barrier->participant_count)
    {
        barrier->waiting = 0u;
        barrier->generation++;
        WakeAllConditionVariable(&barrier->cond);
    }
    else
    {
        while (generation == barrier->generation)
        {
            SleepConditionVariableCS(&barrier->cond, &barrier->mutex, INFINITE);
        }
    }
    LeaveCriticalSection(&barrier->mutex);
#else
    (void)pthread_mutex_lock(&barrier->mutex);
    generation = barrier->generation;
    barrier->waiting++;

    if (barrier->waiting == barrier->participant_count)
    {
        barrier->waiting = 0u;
        barrier->generation++;
        (void)pthread_cond_broadcast(&barrier->cond);
    }
    else
    {
        while (generation == barrier->generation)
        {
            (void)pthread_cond_wait(&barrier->cond, &barrier->mutex);
        }
    }

    (void)pthread_mutex_unlock(&barrier->mutex);
#endif
}

static bool compute_expected_zero_hashes(ssz_chunk_t out[64])
{
    uint8_t pair[SSZ_BYTES_PER_CHUNK * 2u];

    if (out == NULL)
    {
        return false;
    }

    memset(out[0].bytes, 0u, SSZ_BYTES_PER_CHUNK);
    for (size_t depth = 1u; depth < 64u; depth++)
    {
        memcpy(pair, out[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        memcpy(pair + SSZ_BYTES_PER_CHUNK, out[depth - 1u].bytes, SSZ_BYTES_PER_CHUNK);
        if (ssz_hash_sha256(pair, sizeof(pair), out[depth].bytes) != SSZ_SUCCESS)
        {
            return false;
        }
    }

    return true;
}

#if defined(_WIN32)
static DWORD WINAPI worker_main(LPVOID opaque)
#else
static void *worker_main(void *opaque)
#endif
{
    worker_ctx_t *ctx = (worker_ctx_t *)opaque;

    if (ctx != NULL)
    {
        test_barrier_wait(ctx->barrier);
        ctx->zero_hashes = ssz_hash_default_zero_hashes();
        ctx->merkle_err = ssz_merkleize(NULL, 0u, SSZ_NO_LIMIT, NULL, NULL, &ctx->merkle_root);
    }

#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}

static bool test_thread_create(test_thread_t *out_thread, worker_ctx_t *ctx)
{
    if ((out_thread == NULL) || (ctx == NULL))
    {
        return false;
    }

#if defined(_WIN32)
    *out_thread = CreateThread(NULL, 0u, worker_main, ctx, 0u, NULL);
    return *out_thread != NULL;
#else
    return pthread_create(out_thread, NULL, worker_main, ctx) == 0;
#endif
}

static bool test_thread_join(test_thread_t thread)
{
#if defined(_WIN32)
    bool ok = WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0;
    ok = ok && (CloseHandle(thread) != 0);
    return ok;
#else
    return pthread_join(thread, NULL) == 0;
#endif
}

static bool test_default_zero_hashes_concurrent_first_use(void)
{
    enum
    {
        worker_count = 8
    };

    test_thread_t threads[worker_count];
    worker_ctx_t workers[worker_count];
    test_barrier_t barrier;
    ssz_chunk_t expected_zero_hashes[64];
    const ssz_chunk_t *shared_zero_hashes = NULL;

    ASSERT_TRUE(compute_expected_zero_hashes(expected_zero_hashes));
    ASSERT_TRUE(test_barrier_init(&barrier, worker_count));

    memset(workers, 0, sizeof(workers));
    for (size_t i = 0u; i < worker_count; i++)
    {
        workers[i].barrier = &barrier;
        if (!test_thread_create(&threads[i], &workers[i]))
        {
            fprintf(stderr, "Failed to create worker thread %zu\n", i);
            test_barrier_destroy(&barrier);
            return false;
        }
    }

    for (size_t i = 0u; i < worker_count; i++)
    {
        ASSERT_TRUE(test_thread_join(threads[i]));
    }

    test_barrier_destroy(&barrier);

    for (size_t i = 0u; i < worker_count; i++)
    {
        ASSERT_TRUE(workers[i].zero_hashes != NULL);
        ASSERT_ERR(workers[i].merkle_err, SSZ_SUCCESS);
        ASSERT_MEM_EQ(workers[i].merkle_root.bytes, expected_zero_hashes[0].bytes, SSZ_BYTES_PER_CHUNK);

        if (shared_zero_hashes == NULL)
        {
            shared_zero_hashes = workers[i].zero_hashes;
        }
        else
        {
            ASSERT_TRUE(workers[i].zero_hashes == shared_zero_hashes);
        }
    }

    ASSERT_TRUE(shared_zero_hashes != NULL);
    for (size_t depth = 0u; depth < 64u; depth++)
    {
        ASSERT_MEM_EQ(shared_zero_hashes[depth].bytes,
                      expected_zero_hashes[depth].bytes,
                      SSZ_BYTES_PER_CHUNK);
    }

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"default_zero_hashes_concurrent_first_use", test_default_zero_hashes_concurrent_first_use},
    };

    size_t failures = 0u;

    for (size_t i = 0u; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        bool ok = tests[i].fn();
        printf("[%s] %s\n", ok ? "PASS" : "FAIL", tests[i].name);
        if (!ok)
        {
            failures++;
        }
    }

    return (failures == 0u) ? 0 : 1;
}
