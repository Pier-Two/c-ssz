#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Include the source file directly to access static functions.
   Use the bare filename (src/ is in the include path via CMake) so that
   gcov attributes coverage to src/ssz_proof.c, not a path through tests/. */
#include "ssz_proof.c"

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

static bool test_sort_gindex_desc(void)
{
    ssz_gindex_t arr[] = {3u, 10u, 1u, 7u, 5u};
    ssz_internal_sort_gindex_desc(arr, 5u);

    ASSERT_TRUE(arr[0] == 10u);
    ASSERT_TRUE(arr[1] == 7u);
    ASSERT_TRUE(arr[2] == 5u);
    ASSERT_TRUE(arr[3] == 3u);
    ASSERT_TRUE(arr[4] == 1u);

    /* Single element */
    ssz_gindex_t single[] = {42u};
    ssz_internal_sort_gindex_desc(single, 1u);
    ASSERT_TRUE(single[0] == 42u);

    /* Empty */
    ssz_internal_sort_gindex_desc(NULL, 0u);

    return true;
}

static bool test_sort_gindex_asc(void)
{
    ssz_gindex_t arr[] = {10u, 3u, 7u, 1u, 5u};
    ssz_internal_sort_gindex_asc(arr, 5u);

    ASSERT_TRUE(arr[0] == 1u);
    ASSERT_TRUE(arr[1] == 3u);
    ASSERT_TRUE(arr[2] == 5u);
    ASSERT_TRUE(arr[3] == 7u);
    ASSERT_TRUE(arr[4] == 10u);

    return true;
}

static bool test_gindex_overlap_helpers(void)
{
    ASSERT_TRUE(ssz_internal_gindex_covers(2u, 4u));
    ASSERT_TRUE(!ssz_internal_gindex_covers(4u, 2u));
    ASSERT_TRUE(ssz_internal_gindex_covers(1u, 7u));

    ASSERT_TRUE(ssz_internal_gindex_overlaps(2u, 4u));
    ASSERT_TRUE(ssz_internal_gindex_overlaps(4u, 2u));
    ASSERT_TRUE(ssz_internal_gindex_overlaps(5u, 5u));
    ASSERT_TRUE(!ssz_internal_gindex_overlaps(5u, 6u));

    return true;
}

static bool test_validate_multiproof_indices(void)
{
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){5u, 6u}, 2u), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){2u, 3u, 4u}, 3u),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){2u, 4u}, 2u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){4u, 2u}, 2u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){4u, 4u}, 2u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){1u, 7u}, 2u),
               SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){0u, 7u}, 2u),
               SSZ_ERR_GINDEX_INVALID);
    ASSERT_ERR(ssz_internal_validate_multiproof_indices((const ssz_gindex_t[]){7u, 0u}, 2u),
               SSZ_ERR_GINDEX_INVALID);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"sort_gindex_desc", test_sort_gindex_desc},
        {"sort_gindex_asc", test_sort_gindex_asc},
        {"gindex_overlap_helpers", test_gindex_overlap_helpers},
        {"validate_multiproof_indices", test_validate_multiproof_indices},
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

    printf("[OK] %zu/%zu proof_internal tests passed\n", passed, total);
    return 0;
}
