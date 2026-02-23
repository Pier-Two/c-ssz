#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Include the source file directly to access static functions.
   Use the bare filename — src/ is in the include path via CMake. */
#include "ssz_deserialize.c"

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
            fprintf(stderr, "ASSERT_ERR failed at %s:%d: got %d, expected %d\n",                 \
                    __FILE__, __LINE__, (int)_actual, (int)expected);                                 \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

static bool test_variable_sequence_zero_elements(void)
{
    /* ssz_internal_deserialize_variable_sequence with element_count == 0.
       Both public callers guard against this, but the static function handles
       it as a defensive check. */
    ssz_error_t (*volatile deserialize_variable_sequence)(
        const uint8_t *,
        size_t,
        uint64_t,
        size_t,
        ssz_member_codec_t *) = ssz_internal_deserialize_variable_sequence;
    volatile uint64_t zero_elements = 0u;
    volatile size_t min_element_size = 1u;

    /* Zero elements + zero-length input → success. */
    ASSERT_ERR(deserialize_variable_sequence(NULL, 0u, zero_elements, min_element_size, NULL), SSZ_SUCCESS);

    /* Zero elements + non-zero-length input → offset invalid. */
    const uint8_t dummy[4] = {0u};
    volatile size_t dummy_len = sizeof(dummy);
    ASSERT_ERR(deserialize_variable_sequence(dummy, dummy_len, zero_elements, min_element_size, NULL),
               SSZ_ERR_OFFSET_INVALID);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"variable_sequence_zero_elements", test_variable_sequence_zero_elements},
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

    printf("[OK] %zu/%zu deserialize_internal tests passed\n", passed, total);
    return 0;
}
