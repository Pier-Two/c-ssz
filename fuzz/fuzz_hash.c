#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ssz.h"

typedef struct
{
    const uint8_t *ptr;
    size_t remaining;
} fuzz_input_t;

static uint8_t fuzz_take_u8(fuzz_input_t *input)
{
    if ((input == NULL) || (input->remaining == 0u))
    {
        return 0u;
    }

    uint8_t value = input->ptr[0];
    input->ptr++;
    input->remaining--;
    return value;
}

static uint64_t fuzz_take_u64(fuzz_input_t *input)
{
    uint64_t value = 0u;

    for (size_t i = 0u; i < 8u; i++)
    {
        value |= ((uint64_t)fuzz_take_u8(input)) << (8u * i);
    }

    return value;
}

static size_t fuzz_take_size_bounded(fuzz_input_t *input, size_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    uint64_t bound = (uint64_t)max_inclusive;
    return (size_t)(value % (bound + 1u));
}

static void fuzz_fill_bytes(fuzz_input_t *input, uint8_t *out, size_t out_len)
{
    if (out == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < out_len; i++)
    {
        out[i] = fuzz_take_u8(input);
    }
}

static void fuzz_fill_chunks(fuzz_input_t *input, ssz_chunk_t *chunks, size_t chunk_count)
{
    if (chunks == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < chunk_count; i++)
    {
        fuzz_fill_bytes(input, chunks[i].bytes, SSZ_BYTES_PER_CHUNK);
    }
}

static ssz_chunk_t *fuzz_misaligned_chunk_ptr(void *storage)
{
    uintptr_t base = (uintptr_t)storage;
    uintptr_t aligned = (base + (uintptr_t)(SSZ_CHUNK_ALIGNMENT - 1u)) &
                        ~((uintptr_t)SSZ_CHUNK_ALIGNMENT - 1u);
    return (ssz_chunk_t *)(void *)(aligned + 1u);
}

static ssz_error_t fuzz_custom_hash(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    (void)data;
    (void)data_len;

    if (out == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < 32u; i++)
    {
        out[i] = 0xAAu;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_custom_hash_err(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    (void)ctx;
    (void)data;
    (void)data_len;

    if (out != NULL)
    {
        for (size_t i = 0u; i < 32u; i++)
        {
            out[i] = 0u;
        }
    }

    return SSZ_ERR_HASH_FAILURE;
}

static ssz_error_t fuzz_custom_2to1(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    (void)ctx;
    (void)right;

    if ((left == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        out->bytes[i] = left->bytes[i];
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_custom_2to1_err(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    (void)ctx;
    (void)left;
    (void)right;
    (void)out;
    return SSZ_ERR_HASH_FAILURE;
}

static ssz_error_t fuzz_custom_batch(
    const void *ctx,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    (void)ctx;
    (void)pairs;

    if (out == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((pair_count != 0u) && (pairs == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < pair_count; i++)
    {
        for (size_t j = 0u; j < SSZ_BYTES_PER_CHUNK; j++)
        {
            out[i].bytes[j] = 0xBBu;
        }
    }

    return SSZ_SUCCESS;
}

static ssz_error_t fuzz_custom_batch_err(
    const void *ctx,
    const ssz_chunk_t *pairs,
    size_t pair_count,
    ssz_chunk_t *out)
{
    (void)ctx;
    (void)pairs;
    (void)pair_count;
    (void)out;
    return SSZ_ERR_HASH_FAILURE;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if ((data == NULL) || (size == 0u))
    {
        return 0;
    }

    fuzz_input_t input = {
        .ptr = data,
        .remaining = size,
    };

    uint8_t api_selector = fuzz_take_u8(&input);

    switch (api_selector % 10u)
    {
        case 0u:
        {
            uint8_t out[32] = {0u};
            size_t data_len = fuzz_take_size_bounded(&input, 256u);
            if (data_len > input.remaining)
            {
                data_len = input.remaining;
            }

            (void)ssz_hash_sha256(input.ptr, data_len, out);
            (void)ssz_hash_sha256(NULL, 0u, out);
            (void)ssz_hash_sha256(NULL, 1u, out);
            (void)ssz_hash_sha256(input.ptr, data_len, NULL);
#if SIZE_MAX > UINT32_MAX
            (void)ssz_hash_sha256(input.ptr, (size_t)UINT32_MAX + 1u, out);
#endif
            break;
        }

        case 1u:
        {
            ssz_chunk_t left = {{0u}};
            ssz_chunk_t right = {{0u}};
            ssz_chunk_t out;
            fuzz_fill_bytes(&input, left.bytes, SSZ_BYTES_PER_CHUNK);
            fuzz_fill_bytes(&input, right.bytes, SSZ_BYTES_PER_CHUNK);

            (void)ssz_hash_2to1(ssz_hash_default(), &left, &right, &out);
            break;
        }

        case 2u:
        {
            ssz_chunk_t left = {{0u}};
            ssz_chunk_t right = {{0u}};
            ssz_chunk_t out;
            fuzz_fill_bytes(&input, left.bytes, SSZ_BYTES_PER_CHUNK);
            fuzz_fill_bytes(&input, right.bytes, SSZ_BYTES_PER_CHUNK);

            ssz_hash_fn_t hash_fn = {
                .hash = fuzz_custom_hash,
                .hash_2to1 = fuzz_custom_2to1,
                .hash_2to1_batch = NULL,
                .ctx = NULL,
            };

            (void)ssz_hash_2to1(&hash_fn, &left, &right, &out);
            break;
        }

        case 3u:
        {
            ssz_chunk_t left = {{0u}};
            ssz_chunk_t right = {{0u}};
            ssz_chunk_t out;
            fuzz_fill_bytes(&input, left.bytes, SSZ_BYTES_PER_CHUNK);
            fuzz_fill_bytes(&input, right.bytes, SSZ_BYTES_PER_CHUNK);

            ssz_hash_fn_t hash_fn = {
                .hash = fuzz_custom_hash,
                .hash_2to1 = fuzz_custom_2to1_err,
                .hash_2to1_batch = NULL,
                .ctx = NULL,
            };

            (void)ssz_hash_2to1(&hash_fn, &left, &right, &out);
            break;
        }

        case 4u:
        {
            size_t pair_count = fuzz_take_size_bounded(&input, 4u);
            ssz_chunk_t pairs[8] = {{{0u}}};
            ssz_chunk_t out[4] = {{{0u}}};
            fuzz_fill_chunks(&input, pairs, pair_count * 2u);

            (void)ssz_hash_2to1_batch(ssz_hash_default(), pairs, pair_count, out);
            break;
        }

        case 5u:
        {
            size_t pair_count = fuzz_take_size_bounded(&input, 4u);
            ssz_chunk_t pairs[8] = {{{0u}}};
            ssz_chunk_t out[4] = {{{0u}}};
            fuzz_fill_chunks(&input, pairs, pair_count * 2u);

            ssz_hash_fn_t hash_fn = {
                .hash = fuzz_custom_hash,
                .hash_2to1 = NULL,
                .hash_2to1_batch = fuzz_custom_batch,
                .ctx = NULL,
            };

            (void)ssz_hash_2to1_batch(&hash_fn, pairs, pair_count, out);
            break;
        }

        case 6u:
        {
            size_t pair_count = fuzz_take_size_bounded(&input, 4u);
            ssz_chunk_t pairs[8] = {{{0u}}};
            ssz_chunk_t out[4] = {{{0u}}};
            fuzz_fill_chunks(&input, pairs, pair_count * 2u);

            ssz_hash_fn_t hash_fn = {
                .hash = fuzz_custom_hash,
                .hash_2to1 = NULL,
                .hash_2to1_batch = fuzz_custom_batch_err,
                .ctx = NULL,
            };

            (void)ssz_hash_2to1_batch(&hash_fn, pairs, pair_count, out);
            break;
        }

        case 7u:
        {
            size_t pair_count = fuzz_take_size_bounded(&input, 3u) + 1u;
            ssz_chunk_t pairs[8] = {{{0u}}};
            ssz_chunk_t out[4] = {{{0u}}};
            fuzz_fill_chunks(&input, pairs, pair_count * 2u);

            ssz_hash_fn_t hash_fn = {
                .hash = fuzz_custom_hash_err,
                .hash_2to1 = NULL,
                .hash_2to1_batch = NULL,
                .ctx = NULL,
            };

            (void)ssz_hash_2to1_batch(&hash_fn, pairs, pair_count, out);
            break;
        }

        case 8u:
        {
            ssz_chunk_t out[1] = {{{0u}}};
            (void)ssz_hash_2to1_batch(ssz_hash_default(), NULL, 0u, out);
            break;
        }

        default:
        {
            ssz_chunk_t left = {{0u}};
            ssz_chunk_t right = {{0u}};
            ssz_chunk_t out;
            fuzz_fill_bytes(&input, left.bytes, SSZ_BYTES_PER_CHUNK);
            fuzz_fill_bytes(&input, right.bytes, SSZ_BYTES_PER_CHUNK);

            ssz_chunk_t pairs[2] = {left, right};
            ssz_chunk_t batch_out[1] = {{{0u}}};
            const uint8_t zero_pairs64[(size_t)SSZ_BYTES_PER_CHUNK * 2u] = {0u};
            uint8_t misaligned_raw[(sizeof(ssz_chunk_t) * 2u) + SSZ_CHUNK_ALIGNMENT] = {0u};
            uint8_t misaligned_out_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};

            (void)ssz_hash_2to1(ssz_hash_default(), NULL, &right, &out);
            (void)ssz_hash_2to1(ssz_hash_default(), &left, NULL, &out);
            (void)ssz_hash_2to1(ssz_hash_default(), &left, &right, NULL);

            (void)ssz_hash_2to1_batch(ssz_hash_default(), NULL, 1u, batch_out);
            (void)ssz_hash_2to1_batch(ssz_hash_default(), pairs, 1u, NULL);
            (void)ssz_hash_2to1_batch(NULL, pairs, 1u, batch_out);
#if SIZE_MAX > UINT32_MAX
            (void)ssz_hash_2to1_batch(
                ssz_hash_default(),
                pairs,
                (SIZE_MAX / 2u) + 1u,
                batch_out);
#endif
            if (SSZ_CHUNK_ALIGNMENT > 1u)
            {
                (void)memcpy((void *)fuzz_misaligned_chunk_ptr(misaligned_raw), &left, sizeof(left));
                (void)ssz_hash_2to1(ssz_hash_default(),
                                    (const ssz_chunk_t *)(const void *)
                                        fuzz_misaligned_chunk_ptr(misaligned_raw),
                                    &right,
                                    &out);
                (void)ssz_hash_2to1_batch_raw(
                    ssz_hash_default(),
                    zero_pairs64,
                    1u,
                    fuzz_misaligned_chunk_ptr(misaligned_out_raw));
            }
            break;
        }
    }

    return 0;
}
