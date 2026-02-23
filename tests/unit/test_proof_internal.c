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

static bool test_compare_gindex_desc_all_branches(void)
{
    /* Exercise all three branches of ssz_internal_compare_gindex_desc:
       ia < ib  →  return 1
       ia > ib  →  return -1
       ia == ib →  return 0 */
    volatile ssz_gindex_t a_seed = 5u;
    volatile ssz_gindex_t b_seed = 10u;
    ssz_gindex_t a = a_seed;
    ssz_gindex_t b = b_seed;
    int (*volatile cmp_fn)(const void *, const void *) = ssz_internal_compare_gindex_desc;

    ASSERT_TRUE(cmp_fn(&a, &b) == 1);   /* ia < ib */
    ASSERT_TRUE(cmp_fn(&b, &a) == -1);  /* ia > ib */
    ASSERT_TRUE(cmp_fn(&a, &a) == 0);   /* ia == ib */

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"compare_gindex_desc_all_branches", test_compare_gindex_desc_all_branches},
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
