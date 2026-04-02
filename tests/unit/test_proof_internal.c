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

int main(void)
{
    const test_case_t tests[] = {
        {"sort_gindex_desc", test_sort_gindex_desc},
        {"sort_gindex_asc", test_sort_gindex_asc},
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
