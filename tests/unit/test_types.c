#include <limits.h>
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

static bool test_error_string_for_all_codes(void)
{
    for (int code = (int)SSZ_SUCCESS; code <= (int)SSZ_ERR_HASH_FAILURE; code++)
    {
        const char *label = ssz_error_string((ssz_error_t)code);
        ASSERT_TRUE(label != NULL);
        ASSERT_TRUE(strlen(label) > 0u);
    }

    ASSERT_TRUE(strcmp(ssz_error_string((ssz_error_t)9999), "SSZ_ERR_UNKNOWN") == 0);

    return true;
}

static bool test_type_constants(void)
{
    ASSERT_TRUE(SSZ_BYTES_PER_CHUNK == 32u);
    ASSERT_TRUE(SSZ_BYTES_PER_LENGTH_OFFSET == 4u);
    ASSERT_TRUE(SSZ_BITS_PER_BYTE == 8u);
    ASSERT_TRUE(SSZ_NO_LIMIT == UINT64_MAX);
    ASSERT_TRUE(sizeof(ssz_chunk_t) == SSZ_BYTES_PER_CHUNK);
    ASSERT_TRUE(SSZ_CHUNK_ALIGNMENT == sizeof(uintptr_t));
    ASSERT_TRUE(SSZ_CHUNK_ALIGNMENT > 1u);

    ASSERT_TRUE(SSZ_VERSION_MAJOR == 0u);
    ASSERT_TRUE(SSZ_VERSION_MINOR == 1u);
    ASSERT_TRUE(SSZ_VERSION_PATCH == 0u);
    ASSERT_TRUE(strcmp(SSZ_VERSION_STRING, "0.1.0") == 0);

    return true;
}

static bool test_path_step_length_constant(void)
{
    ASSERT_TRUE(SSZ_PATH_STEP_LENGTH == UINT64_MAX);
    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"error_string_for_all_codes", test_error_string_for_all_codes},
        {"type_constants", test_type_constants},
        {"path_step_length_constant", test_path_step_length_constant},
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

    printf("[OK] %zu/%zu types tests passed\n", passed, total);
    return 0;
}
