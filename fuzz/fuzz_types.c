#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ssz.h"

typedef struct
{
    uint8_t current[4];
    uint8_t defaults[4];
} fuzz_types_ctx_t;

static ssz_error_t fuzz_types_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const fuzz_types_ctx_t *state = (const fuzz_types_ctx_t *)ctx;
    size_t index = (size_t)(member_id % 4u);

    if ((state == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (out == NULL)
    {
        *out_written = 1u;
        return SSZ_SUCCESS;
    }
    if (out_cap < 1u)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    out[0] = state->current[index];
    *out_written = 1u;
    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_types_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    fuzz_types_ctx_t *state = (fuzz_types_ctx_t *)ctx;
    size_t index = (size_t)(member_id % 4u);

    if (state == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (data == NULL)
    {
        state->current[index] = state->defaults[index];
        return SSZ_SUCCESS;
    }
    if (data_len != 1u)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    state->current[index] = data[0];
    return SSZ_SUCCESS;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    fuzz_types_ctx_t ctx = {
        .current = {1u, 2u, 3u, 4u},
        .defaults = {0u, 0u, 0u, 0u},
    };
    size_t fixed_sizes[3] = {1u, 1u, 1u};
    ssz_container_schema_t schema = {
        .field_fixed_sizes = fixed_sizes,
        .field_count = 3u,
    };
    ssz_member_codec_t codec = {
        .ctx = &ctx,
        .write = fuzz_types_write,
        .read = fuzz_types_read,
        .root = NULL,
    };
    uint8_t scratch[16] = {0u};
    uint8_t selector = 0u;
    bool is_zero = false;

    if ((data != NULL) && (size != 0u))
    {
        for (size_t i = 0u; (i < size) && (i < sizeof(ctx.current)); i++)
        {
            ctx.current[i] = data[i];
        }
    }

    (void)ssz_default_container(&schema, &codec);

    ctx.current[0] = ((data != NULL) && (size > 0u)) ? data[0] : 7u;
    ctx.current[1] = ((data != NULL) && (size > 1u)) ? data[1] : 0u;
    ctx.current[2] = ((data != NULL) && (size > 2u)) ? data[2] : 9u;

    (void)ssz_default_union(2u, false, &codec, &selector);
    (void)ssz_default_union(2u, true, &codec, &selector);
    (void)ssz_default_union(0u, false, &codec, &selector);
    (void)ssz_default_union(2u, false, &codec, NULL);

    ctx.current[0] = ((data != NULL) && (size > 0u)) ? data[0] : 1u;
    ctx.current[1] = 0u;
    ctx.current[2] = ((data != NULL) && (size > 1u)) ? data[1] : 2u;

    (void)ssz_is_zero_vector_composite(3u, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_vector_composite(0u, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_vector_composite(3u, &codec, scratch, 0u, &is_zero);

    (void)ssz_is_zero_container(&schema, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_container(NULL, &codec, scratch, sizeof(scratch), &is_zero);

    (void)ssz_is_zero_union(0u, 2u, false, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_union(0u, 2u, true, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_union(1u, 2u, false, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_union(3u, 2u, false, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_union(0u, 0u, false, &codec, scratch, sizeof(scratch), &is_zero);
    (void)ssz_is_zero_union(0u, 2u, false, &codec, scratch, sizeof(scratch), NULL);

    for (ssz_error_t err = SSZ_SUCCESS; err <= SSZ_ERR_HASH_FAILURE; err++)
    {
        (void)ssz_error_string(err);
    }
    (void)ssz_error_string((ssz_error_t)255);

    return 0;
}
