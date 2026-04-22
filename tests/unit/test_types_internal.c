#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ssz.h"

/* Include the source file directly to access static functions.
   Use the bare filename (src/ is in the include path via CMake) so that
   gcov attributes coverage to src/ssz_types.c, not a path through tests/. */
#include "ssz_types.c"

#define CONTAINER_SCHEMA(field_fixed_sizes_value, field_count_value)                              \
    (&(const ssz_container_schema_t){                                                             \
        .field_fixed_sizes = (field_fixed_sizes_value),                                           \
        .field_count = (field_count_value),                                                       \
    })

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

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

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
                    "Assertion failed at %s:%d: memory mismatch (%s vs %s, len=%zu)\n",            \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    #actual,                                                                          \
                    #expected,                                                                        \
                    (size_t)(len));                                                                   \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

#define ASSERT_SIZE_EQ(actual, expected)                                                             \
    do                                                                                               \
    {                                                                                                \
        size_t _actual = (actual);                                                                   \
        size_t _expected = (expected);                                                               \
        if (_actual != _expected)                                                                    \
        {                                                                                            \
            fprintf(stderr,                                                                           \
                    "Assertion failed at %s:%d: %zu != %zu\n",                                      \
                    __FILE__,                                                                         \
                    __LINE__,                                                                         \
                    _actual,                                                                          \
                    _expected);                                                                       \
            return false;                                                                            \
        }                                                                                            \
    } while (0)

typedef struct
{
    uint64_t id;
    uint8_t current[64];
    size_t current_len;
    uint8_t default_bytes[64];
    size_t default_len;
    size_t default_calls;
    size_t restore_calls;
    size_t write_calls;
    size_t write_fail_at;
    ssz_error_t write_fail_err;
    size_t short_write_at;
    size_t default_fail_at;
    ssz_error_t default_fail_err;
    size_t restore_fail_at;
    ssz_error_t restore_fail_err;
} codec_entry_t;

typedef struct
{
    codec_entry_t *entries;
    size_t entry_count;
} codec_ctx_t;

static codec_entry_t *find_entry(codec_ctx_t *ctx, uint64_t member_id)
{
    if (ctx == NULL)
    {
        return NULL;
    }

    for (size_t i = 0u; i < ctx->entry_count; i++)
    {
        if (ctx->entries[i].id == member_id)
        {
            return &ctx->entries[i];
        }
    }

    return NULL;
}

static const codec_entry_t *find_entry_const(const codec_ctx_t *ctx, uint64_t member_id)
{
    return find_entry((codec_ctx_t *)ctx, member_id);
}

static ssz_error_t codec_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const codec_ctx_t *codec_ctx = (const codec_ctx_t *)ctx;
    const codec_entry_t *entry = find_entry_const(codec_ctx, member_id);
    size_t reported_len = 0u;

    if ((entry == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    ((codec_entry_t *)entry)->write_calls++;
    if ((entry->write_fail_at != 0u) && (entry->write_calls == entry->write_fail_at))
    {
        return entry->write_fail_err;
    }

    reported_len = entry->current_len;
    if ((entry->short_write_at != 0u) && (entry->write_calls == entry->short_write_at) && (reported_len != 0u))
    {
        reported_len--;
    }

    *out_written = reported_len;
    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < reported_len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(out, entry->current, reported_len);
    return SSZ_SUCCESS;
}

static ssz_error_t codec_read(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    codec_ctx_t *codec_ctx = (codec_ctx_t *)ctx;
    codec_entry_t *entry = find_entry(codec_ctx, member_id);

    if (entry == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((data == NULL) && (data_len == 0u))
    {
        entry->default_calls++;
        if ((entry->default_fail_at != 0u) && (entry->default_calls == entry->default_fail_at))
        {
            return entry->default_fail_err;
        }

        entry->current_len = entry->default_len;
        memcpy(entry->current, entry->default_bytes, entry->default_len);
        return SSZ_SUCCESS;
    }

    if (data == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    entry->restore_calls++;
    if ((entry->restore_fail_at != 0u) && (entry->restore_calls == entry->restore_fail_at))
    {
        return entry->restore_fail_err;
    }
    if (data_len > sizeof(entry->current))
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    entry->current_len = data_len;
    memcpy(entry->current, data, data_len);
    return SSZ_SUCCESS;
}

static ssz_member_codec_t make_codec(codec_ctx_t *ctx)
{
    ssz_member_codec_t codec = {
        .ctx = ctx,
        .write = codec_write,
        .read = codec_read,
        .root = NULL,
    };
    return codec;
}

static bool test_capture_member_error_paths(void)
{
    ssz_error_t (*volatile measure_fn)(ssz_member_codec_t *, uint64_t, size_t *) =
        ssz_types_internal_measure_member;
    ssz_error_t (*volatile capture_fn)(ssz_member_codec_t *, uint64_t, uint8_t *, size_t, size_t) =
        ssz_types_internal_capture_member;
    size_t byte_len = 0u;

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0xAAu, 0xBBu},
            .current_len = 2u,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);
        uint8_t bytes[1];

        ASSERT_ERR(measure_fn(&codec, 0u, &byte_len), SSZ_SUCCESS);
        ASSERT_ERR(capture_fn(&codec, 0u, bytes, sizeof(bytes), byte_len), SSZ_ERR_BUFFER_TOO_SMALL);
        ASSERT_SIZE_EQ(entry.write_calls, 1u);
    }

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0x10u, 0x11u},
            .current_len = 2u,
            .write_fail_at = 2u,
            .write_fail_err = SSZ_ERR_HASH_FAILURE,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);
        uint8_t bytes[2];

        ASSERT_ERR(measure_fn(&codec, 0u, &byte_len), SSZ_SUCCESS);
        ASSERT_ERR(capture_fn(&codec, 0u, bytes, sizeof(bytes), byte_len), SSZ_ERR_HASH_FAILURE);
        ASSERT_SIZE_EQ(entry.write_calls, 2u);
    }

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0x20u, 0x21u},
            .current_len = 2u,
            .short_write_at = 2u,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);
        uint8_t bytes[2];

        ASSERT_ERR(measure_fn(&codec, 0u, &byte_len), SSZ_SUCCESS);
        ASSERT_ERR(capture_fn(&codec, 0u, bytes, sizeof(bytes), byte_len), SSZ_ERR_TYPE_MISMATCH);
        ASSERT_SIZE_EQ(entry.write_calls, 2u);
    }

    return true;
}

static bool test_member_is_default_error_paths(void)
{
    ssz_error_t (*volatile member_is_default_fn)(
        ssz_member_codec_t *,
        uint64_t,
        uint8_t *,
        size_t,
        bool *) =
        ssz_types_internal_member_is_default;
    bool is_default = false;
    uint8_t scratch[3u] = {0u};

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0xA0u, 0xA1u},
            .current_len = 2u,
            .default_bytes = {0x01u},
            .default_len = 1u,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);
        uint8_t small_scratch[2u] = {0u};

        ASSERT_ERR(
            member_is_default_fn(&codec, 0u, small_scratch, sizeof(small_scratch), &is_default),
            SSZ_ERR_BUFFER_TOO_SMALL);
        ASSERT_SIZE_EQ(entry.write_calls, 3u);
        ASSERT_SIZE_EQ(entry.default_calls, 1u);
        ASSERT_SIZE_EQ(entry.restore_calls, 1u);
        ASSERT_MEM_EQ(entry.current, ((const uint8_t[]){0xA0u, 0xA1u}), 2u);
    }

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0xA0u, 0xA1u},
            .current_len = 2u,
            .default_bytes = {0x01u},
            .default_len = 1u,
            .default_fail_at = 1u,
            .default_fail_err = SSZ_ERR_SELECTOR_INVALID,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        ASSERT_ERR(member_is_default_fn(&codec, 0u, scratch, sizeof(scratch), &is_default),
                   SSZ_ERR_SELECTOR_INVALID);
        ASSERT_SIZE_EQ(entry.write_calls, 2u);
        ASSERT_SIZE_EQ(entry.default_calls, 1u);
        ASSERT_SIZE_EQ(entry.restore_calls, 0u);
        ASSERT_MEM_EQ(entry.current, ((const uint8_t[]){0xA0u, 0xA1u}), 2u);
    }

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0xB0u, 0xB1u},
            .current_len = 2u,
            .default_bytes = {0x09u},
            .default_len = 1u,
            .write_fail_at = 4u,
            .write_fail_err = SSZ_ERR_HASH_FAILURE,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        ASSERT_ERR(member_is_default_fn(&codec, 0u, scratch, sizeof(scratch), &is_default),
                   SSZ_ERR_HASH_FAILURE);
        ASSERT_SIZE_EQ(entry.write_calls, 4u);
        ASSERT_SIZE_EQ(entry.default_calls, 1u);
        ASSERT_SIZE_EQ(entry.restore_calls, 1u);
        ASSERT_MEM_EQ(entry.current, ((const uint8_t[]){0xB0u, 0xB1u}), 2u);
    }

    {
        codec_entry_t entry = {
            .id = 0u,
            .current = {0xC0u, 0xC1u},
            .current_len = 2u,
            .default_bytes = {0x0Au},
            .default_len = 1u,
            .write_fail_at = 4u,
            .write_fail_err = SSZ_ERR_HASH_FAILURE,
            .restore_fail_at = 1u,
            .restore_fail_err = SSZ_ERR_PROOF_INVALID,
        };
        codec_ctx_t ctx = {
            .entries = &entry,
            .entry_count = 1u,
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        ASSERT_ERR(member_is_default_fn(&codec, 0u, scratch, sizeof(scratch), &is_default),
                   SSZ_ERR_PROOF_INVALID);
        ASSERT_SIZE_EQ(entry.write_calls, 4u);
        ASSERT_SIZE_EQ(entry.default_calls, 1u);
        ASSERT_SIZE_EQ(entry.restore_calls, 1u);
        ASSERT_TRUE(entry.current_len == 1u);
        ASSERT_MEM_EQ(entry.current, entry.default_bytes, entry.default_len);
    }

    return true;
}

static bool test_container_error_and_early_return_paths(void)
{
    const size_t field_fixed_sizes[3] = {1u, 1u, 1u};
    bool is_zero = true;
    uint8_t scratch[2u] = {0u};

    {
        codec_entry_t entries[] = {
            {
                .id = 0u,
                .current = {0x11u},
                .current_len = 1u,
                .default_bytes = {0x01u},
                .default_len = 1u,
            },
            {
                .id = 1u,
                .current = {0x22u},
                .current_len = 1u,
                .default_bytes = {0x02u},
                .default_len = 1u,
                .default_fail_at = 1u,
                .default_fail_err = SSZ_ERR_HASH_FAILURE,
            },
            {
                .id = 2u,
                .current = {0x33u},
                .current_len = 1u,
                .default_bytes = {0x03u},
                .default_len = 1u,
            },
        };
        codec_ctx_t ctx = {
            .entries = entries,
            .entry_count = sizeof(entries) / sizeof(entries[0]),
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        ASSERT_ERR(ssz_default_container(CONTAINER_SCHEMA(field_fixed_sizes, 3u), &codec),
                   SSZ_ERR_HASH_FAILURE);
        ASSERT_SIZE_EQ(entries[0].default_calls, 1u);
        ASSERT_SIZE_EQ(entries[1].default_calls, 1u);
        ASSERT_SIZE_EQ(entries[2].default_calls, 0u);
    }

    {
        codec_entry_t entries[] = {
            {
                .id = 0u,
                .current = {0x44u},
                .current_len = 1u,
                .default_bytes = {0x44u},
                .default_len = 1u,
            },
            {
                .id = 1u,
                .current = {0x55u},
                .current_len = 1u,
                .default_bytes = {0x05u},
                .default_len = 1u,
                .default_fail_at = 1u,
                .default_fail_err = SSZ_ERR_SELECTOR_INVALID,
            },
            {
                .id = 2u,
                .current = {0x66u},
                .current_len = 1u,
                .default_bytes = {0x06u},
                .default_len = 1u,
            },
        };
        codec_ctx_t ctx = {
            .entries = entries,
            .entry_count = sizeof(entries) / sizeof(entries[0]),
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        ASSERT_ERR(
            ssz_is_zero_container(
                CONTAINER_SCHEMA(field_fixed_sizes, 3u), &codec, scratch, sizeof(scratch), &is_zero),
            SSZ_ERR_SELECTOR_INVALID);
        ASSERT_SIZE_EQ(entries[0].write_calls, 4u);
        ASSERT_SIZE_EQ(entries[0].default_calls, 1u);
        ASSERT_SIZE_EQ(entries[0].restore_calls, 1u);
        ASSERT_SIZE_EQ(entries[1].write_calls, 2u);
        ASSERT_SIZE_EQ(entries[1].default_calls, 1u);
        ASSERT_SIZE_EQ(entries[1].restore_calls, 0u);
        ASSERT_SIZE_EQ(entries[2].write_calls, 0u);
    }

    {
        codec_entry_t entries[] = {
            {
                .id = 0u,
                .current = {0x77u},
                .current_len = 1u,
                .default_bytes = {0x07u},
                .default_len = 1u,
            },
            {
                .id = 1u,
                .current = {0x88u},
                .current_len = 1u,
                .default_bytes = {0x08u},
                .default_len = 1u,
                .default_fail_at = 1u,
                .default_fail_err = SSZ_ERR_HASH_FAILURE,
            },
        };
        codec_ctx_t ctx = {
            .entries = entries,
            .entry_count = sizeof(entries) / sizeof(entries[0]),
        };
        ssz_member_codec_t codec = make_codec(&ctx);

        is_zero = true;
        ASSERT_ERR(
            ssz_is_zero_container(
                CONTAINER_SCHEMA(((const size_t[]){1u, 1u}), 2u),
                &codec,
                scratch,
                sizeof(scratch),
                &is_zero),
            SSZ_SUCCESS);
        ASSERT_FALSE(is_zero);
        ASSERT_SIZE_EQ(entries[0].write_calls, 4u);
        ASSERT_SIZE_EQ(entries[0].default_calls, 1u);
        ASSERT_SIZE_EQ(entries[0].restore_calls, 1u);
        ASSERT_SIZE_EQ(entries[1].write_calls, 0u);
        ASSERT_SIZE_EQ(entries[1].default_calls, 0u);
        ASSERT_SIZE_EQ(entries[1].restore_calls, 0u);
        ASSERT_MEM_EQ(entries[0].current, ((const uint8_t[]){0x77u}), 1u);
    }

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"capture_member_error_paths", test_capture_member_error_paths},
        {"member_is_default_error_paths", test_member_is_default_error_paths},
        {"container_error_and_early_return_paths", test_container_error_and_early_return_paths},
    };

    for (size_t i = 0u; i < (sizeof(tests) / sizeof(tests[0])); i++)
    {
        if (!tests[i].fn())
        {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    printf("[OK] %zu/%zu types_internal tests passed\n",
           sizeof(tests) / sizeof(tests[0]),
           sizeof(tests) / sizeof(tests[0]));
    return 0;
}
