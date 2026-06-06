#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ssz.h"

static ssz_chunk_t g_fuzz_merkle_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_fuzz_merkle_scratch = {
    .chunks = g_fuzz_merkle_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

/* Macro wrappers to inject scratch parameter into hash_tree_root functions */
#define ssz_hash_tree_root_bitvector(bits, len, count, hfn, out) \
    ssz_hash_tree_root_bitvector((bits), (len), (count), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_bitlist(bits, len, blen, blim, hfn, out) \
    ssz_hash_tree_root_bitlist((bits), (len), (blen), (blim), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_vector_fixed(elems, count, esz, hfn, out) \
    ssz_hash_tree_root_vector_fixed((elems), (count), (esz), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_vector_composite(count, codec, hfn, out) \
    ssz_hash_tree_root_vector_composite((count), (codec), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_vector_roots(roots, count, hfn, out) \
    ssz_hash_tree_root_vector_roots((roots), (count), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_list_fixed(elems, count, lim, esz, hfn, out) \
    ssz_hash_tree_root_list_fixed(                                      \
        (elems),                                                        \
        (count),                                                        \
        (lim),                                                          \
        (esz),                                                          \
        &g_fuzz_merkle_scratch,                                         \
        (hfn),                                                          \
        (out))

#define ssz_hash_tree_root_list_composite(count, lim, codec, hfn, out) \
    ssz_hash_tree_root_list_composite((count), (lim), (codec), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_hash_tree_root_list_roots(roots, count, lim, hfn, out) \
    ssz_hash_tree_root_list_roots((roots), (count), (lim), &g_fuzz_merkle_scratch, (hfn), (out))

#define ssz_merkleize(chunks, count, lim, hfn, out) \
    ssz_merkleize((chunks), (count), (lim), &g_fuzz_merkle_scratch, (hfn), (out))

typedef struct
{
    const uint8_t *ptr;
    size_t remaining;
} fuzz_input_t;

typedef struct
{
    const ssz_chunk_t *roots;
    size_t root_count;
    uint8_t mode;
} fuzz_root_ctx_t;

typedef struct
{
    uint8_t mode;
    uint64_t fail_member;
} fuzz_test_root_ctx_t;

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

static uint64_t fuzz_take_u64_bounded(fuzz_input_t *input, uint64_t max_inclusive)
{
    if (max_inclusive == 0u)
    {
        return 0u;
    }

    uint64_t value = fuzz_take_u64(input);
    if (max_inclusive == UINT64_MAX)
    {
        return value;
    }

    return value % (max_inclusive + 1u);
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

static ssz_error_t fuzz_member_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const fuzz_root_ctx_t *state = (const fuzz_root_ctx_t *)ctx;

    if ((state == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((state->mode & 3u) == 1u)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }
    if ((state->mode & 3u) == 2u)
    {
        if ((state->root_count == 0u) || ((size_t)member_id >= state->root_count))
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        *out_root = state->roots[state->root_count - 1u - ((size_t)member_id % state->root_count)];
        return SSZ_SUCCESS;
    }
    if ((size_t)member_id >= state->root_count)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_root = state->roots[member_id];
    return SSZ_SUCCESS;
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
        out[i] = 0x11u;
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
    (void)out;
    return SSZ_ERR_HASH_FAILURE;
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

static ssz_error_t fuzz_test_member_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const fuzz_test_root_ctx_t *state = (const fuzz_test_root_ctx_t *)ctx;

    if ((state == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (state->mode == 1u)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }
    if ((state->mode == 2u) && (member_id >= state->fail_member))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        out_root->bytes[i] = (uint8_t)(member_id + i);
    }
    return SSZ_SUCCESS;
}

static void fuzz_cover_merkle_errors(void)
{
    uint8_t value128[16] = {0u};
    uint8_t value256[32] = {0u};
    uint8_t bits_ok[2] = {0u};
    uint8_t bits_bad[1] = {0xFEu};
    uint8_t elements[32] = {0u};
    uint8_t bitlist_2chunk[64] = {0u};
    uint8_t active_valid[1] = {0x03u};
    uint8_t active_zero[1] = {0u};
    uint8_t active_bad_count[1] = {0x01u};
    uint8_t active_long[33] = {0u};

    ssz_chunk_t roots[4] = {{{0u}}};
    ssz_chunk_t out_root;
    ssz_chunk_t root = {{0u}};
    uint8_t misaligned_root_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t misaligned_scratch_raw[(sizeof(ssz_chunk_t) * 2u) + SSZ_CHUNK_ALIGNMENT] = {0u};

    fuzz_test_root_ctx_t root_ctx_ok = {
        .mode = 0u,
        .fail_member = UINT64_MAX,
    };
    fuzz_test_root_ctx_t root_ctx_fail = {
        .mode = 1u,
        .fail_member = 0u,
    };
    fuzz_test_root_ctx_t root_ctx_fail_after_first = {
        .mode = 2u,
        .fail_member = 1u,
    };
    ssz_member_codec_t codec_ok = {
        .ctx = &root_ctx_ok,
        .write = NULL,
        .read = NULL,
        .root = fuzz_test_member_root,
    };
    ssz_member_codec_t codec_fail = {
        .ctx = &root_ctx_fail,
        .write = NULL,
        .read = NULL,
        .root = fuzz_test_member_root,
    };
    ssz_member_codec_t codec_fail_after_first = {
        .ctx = &root_ctx_fail_after_first,
        .write = NULL,
        .read = NULL,
        .root = fuzz_test_member_root,
    };
    ssz_member_codec_t codec_no_root = {
        .ctx = &root_ctx_ok,
        .write = NULL,
        .read = NULL,
        .root = NULL,
    };
    ssz_hash_fn_t hash_err_2to1 = {
        .hash = fuzz_custom_hash,
        .hash_2to1 = fuzz_custom_2to1_err,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };
    ssz_hash_fn_t hash_err = {
        .hash = fuzz_custom_hash_err,
        .hash_2to1 = NULL,
        .hash_2to1_batch = NULL,
        .ctx = NULL,
    };

    (void)ssz_hash_tree_root_uint8(1u, NULL);
    (void)ssz_hash_tree_root_uint16(1u, NULL);
    (void)ssz_hash_tree_root_uint32(1u, NULL);
    (void)ssz_hash_tree_root_uint64(1u, NULL);
    (void)ssz_hash_tree_root_uint128(NULL, sizeof(value128), &out_root);
    (void)ssz_hash_tree_root_uint128(value128, sizeof(value128), NULL);
    (void)ssz_hash_tree_root_uint256(NULL, sizeof(value256), &out_root);
    (void)ssz_hash_tree_root_uint256(value256, sizeof(value256), NULL);

    (void)ssz_hash_tree_root_bitvector(bits_ok, 1u, 8u, ssz_hash_default(), NULL);
    (void)
        ssz_hash_tree_root_bitvector(bits_ok, SIZE_MAX, UINT64_MAX, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_bitvector(
        bits_ok,
        SIZE_MAX,
        UINT64_MAX - 7u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_bitvector(bits_bad, 1u, 1u, ssz_hash_default(), &out_root);

    (void)ssz_hash_tree_root_bitlist(bits_ok, 1u, 8u, SSZ_NO_LIMIT, ssz_hash_default(), NULL);
    (void)ssz_hash_tree_root_bitlist(
        bits_ok,
        sizeof(bits_ok),
        UINT64_MAX,
        SSZ_NO_LIMIT,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_bitlist(NULL, 0u, 0u, UINT64_MAX - 7u, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_bitlist(bits_ok, 1u, 8u, SSZ_NO_LIMIT, &hash_err_2to1, &out_root);
    (void)ssz_hash_tree_root_bitlist(
        bitlist_2chunk,
        sizeof(bitlist_2chunk),
        512u,
        SSZ_NO_LIMIT,
        &hash_err_2to1,
        &out_root);

    (void)ssz_hash_tree_root_vector_fixed(elements, 1u, 1u, ssz_hash_default(), NULL);
#if UINT64_MAX > SIZE_MAX
    (void)ssz_hash_tree_root_vector_fixed(
        elements,
        (uint64_t)SIZE_MAX + 1u,
        1u,
        ssz_hash_default(),
        &out_root);
#endif
    (void)ssz_hash_tree_root_vector_fixed(
        elements,
        (uint64_t)SIZE_MAX,
        2u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_vector_fixed(
        elements,
        (uint64_t)SIZE_MAX,
        1u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_vector_fixed(NULL, 1u, 1u, ssz_hash_default(), &out_root);

    (void)ssz_hash_tree_root_vector_composite(1u, &codec_ok, ssz_hash_default(), NULL);
    (void)ssz_hash_tree_root_vector_composite(1u, NULL, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_vector_composite(1u, &codec_no_root, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_vector_composite(1u, &codec_fail, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_vector_composite(
        (UINT64_C(1) << 63u) + 1u,
        &codec_ok,
        ssz_hash_default(),
        &out_root);

#if UINT64_MAX > SIZE_MAX
    (void)ssz_hash_tree_root_vector_roots(
        roots,
        (uint64_t)SIZE_MAX + 1u,
        ssz_hash_default(),
        &out_root);
#endif

    (void)ssz_hash_tree_root_list_fixed(elements, 1u, SSZ_NO_LIMIT, 1u, ssz_hash_default(), NULL);
#if UINT64_MAX > SIZE_MAX
    (void)ssz_hash_tree_root_list_fixed(
        elements,
        (uint64_t)SIZE_MAX + 1u,
        SSZ_NO_LIMIT,
        1u,
        ssz_hash_default(),
        &out_root);
#endif
    (void)ssz_hash_tree_root_list_fixed(
        elements,
        (uint64_t)SIZE_MAX,
        SSZ_NO_LIMIT,
        2u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_list_fixed(
        elements,
        (uint64_t)SIZE_MAX,
        SSZ_NO_LIMIT,
        1u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_list_fixed(NULL, 1u, SSZ_NO_LIMIT, 1u, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_list_fixed(
        elements,
        1u,
        UINT64_MAX - 1u,
        32u,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_list_fixed(elements, 1u, SSZ_NO_LIMIT, 1u, &hash_err_2to1, &out_root);

    (void)ssz_hash_tree_root_list_composite(1u, SSZ_NO_LIMIT, &codec_ok, ssz_hash_default(), NULL);
    (void)ssz_hash_tree_root_list_composite(1u, SSZ_NO_LIMIT, NULL, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_list_composite(
        1u,
        SSZ_NO_LIMIT,
        &codec_no_root,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_list_composite(
        2u,
        SSZ_NO_LIMIT,
        &codec_fail_after_first,
        ssz_hash_default(),
        &out_root);
    (void)ssz_hash_tree_root_list_composite(
        (UINT64_C(1) << 63u) + 1u,
        SSZ_NO_LIMIT,
        &codec_ok,
        ssz_hash_default(),
        &out_root);

    (void)ssz_hash_tree_root_list_roots(roots, 1u, SSZ_NO_LIMIT, ssz_hash_default(), NULL);
    (void)ssz_hash_tree_root_list_roots(
        roots,
        1u,
        (UINT64_C(1) << 63u) + 1u,
        ssz_hash_default(),
        &out_root);
#if UINT64_MAX > SIZE_MAX
    (void)ssz_hash_tree_root_list_roots(
        roots,
        (uint64_t)SIZE_MAX + 1u,
        SSZ_NO_LIMIT,
        ssz_hash_default(),
        &out_root);
#endif
    (void)ssz_hash_tree_root_list_roots(roots, 1u, SSZ_NO_LIMIT, &hash_err_2to1, &out_root);

    (void)ssz_hash_tree_root_union(1u, false, &codec_ok, ssz_hash_default(), NULL);
    (void)ssz_hash_tree_root_union(1u, false, NULL, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_union(1u, false, &codec_no_root, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_union(1u, false, &codec_fail, ssz_hash_default(), &out_root);
    (void)ssz_hash_tree_root_union(1u, false, &codec_ok, &hash_err_2to1, &out_root);

    (void)ssz_merkleize(roots, 1u, SSZ_NO_LIMIT, ssz_hash_default(), NULL);
    (void)ssz_merkleize(NULL, 1u, SSZ_NO_LIMIT, ssz_hash_default(), &out_root);
    (void)ssz_merkleize(roots, 1u, (UINT64_C(1) << 63u) + 1u, ssz_hash_default(), &out_root);
    (void)ssz_merkleize(roots, 1u, SSZ_NO_LIMIT, &hash_err_2to1, &out_root);

    (void)ssz_mix_in_length(NULL, (const uint8_t[32]){0u}, 32u, ssz_hash_default(), &out_root);
    (void)ssz_mix_in_length(&root, NULL, 32u, ssz_hash_default(), &out_root);
    (void)ssz_mix_in_length(&root, (const uint8_t[32]){0u}, 32u, ssz_hash_default(), NULL);
    (void)ssz_mix_in_length_u64(&root, 1u, &hash_err_2to1, &out_root);
    (void)ssz_mix_in_selector(NULL, 1u, ssz_hash_default(), &out_root);
    (void)ssz_mix_in_selector(&root, 1u, ssz_hash_default(), NULL);
    (void)ssz_mix_in_selector(&root, 1u, &hash_err_2to1, &out_root);

    (void)ssz_mix_in_active_fields(NULL, active_valid, 1u, ssz_hash_default(), &out_root);
    (void)ssz_mix_in_active_fields(&root, active_valid, 1u, ssz_hash_default(), NULL);
    (void)ssz_mix_in_active_fields(
        &root,
        active_long,
        sizeof(active_long),
        ssz_hash_default(),
        &out_root);
    (void)ssz_mix_in_active_fields(&root, NULL, 1u, ssz_hash_default(), &out_root);

    if (SSZ_CHUNK_ALIGNMENT > 1u)
    {
        const ssz_merkle_scratch_t misaligned_scratch = {
            .chunks = fuzz_misaligned_chunk_ptr(misaligned_scratch_raw),
            .chunk_count = 2u,
        };

        (void)memcpy((void *)fuzz_misaligned_chunk_ptr(misaligned_root_raw), &root, sizeof(root));
        (void)ssz_hash_tree_root_uint64(7u, fuzz_misaligned_chunk_ptr(misaligned_root_raw));
        (void)ssz_merkleize((const ssz_chunk_t *)(const void *)
                                fuzz_misaligned_chunk_ptr(misaligned_root_raw),
                            1u,
                            SSZ_NO_LIMIT,
                            ssz_hash_default(),
                            &out_root);
        (void)(ssz_merkleize)(roots, 1u, SSZ_NO_LIMIT, &misaligned_scratch, ssz_hash_default(), &out_root);
        (void)ssz_mix_in_length((const ssz_chunk_t *)(const void *)
                                    fuzz_misaligned_chunk_ptr(misaligned_root_raw),
                                (const uint8_t[32]){0u},
                                32u,
                                ssz_hash_default(),
                                &out_root);
    }
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

    const ssz_hash_fn_t *hash_fn = ssz_hash_default();
    uint8_t api_selector = fuzz_take_u8(&input);

    switch (api_selector % 20u)
    {
    case 0u:
    {
        size_t chunk_count = fuzz_take_size_bounded(&input, 16u);
        ssz_chunk_t chunks[16] = {{{0u}}};
        fuzz_fill_chunks(&input, chunks, chunk_count);

        uint64_t limit =
            (fuzz_take_u8(&input) & 1u) != 0u ? SSZ_NO_LIMIT : fuzz_take_u64_bounded(&input, 32u);
        ssz_chunk_t out_root;
        (void)ssz_merkleize(chunks, chunk_count, limit, hash_fn, &out_root);
        break;
    }

    case 1u:
    {
        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint8(fuzz_take_u8(&input), &out_root);
        break;
    }

    case 3u:
    {
        uint16_t value = (uint16_t)fuzz_take_u64(&input);
        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint16(value, &out_root);
        break;
    }

    case 4u:
    {
        uint32_t value = (uint32_t)fuzz_take_u64(&input);
        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint32(value, &out_root);
        break;
    }

    case 5u:
    {
        uint64_t value = fuzz_take_u64(&input);
        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint64(value, &out_root);
        break;
    }

    case 6u:
    {
        uint8_t value[16] = {0u};
        fuzz_fill_bytes(&input, value, sizeof(value));

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint128(value, sizeof(value), &out_root);
        break;
    }

    case 7u:
    {
        uint8_t value[32] = {0u};
        fuzz_fill_bytes(&input, value, sizeof(value));

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_uint256(value, sizeof(value), &out_root);
        break;
    }

    case 8u:
    {
        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_boolean(fuzz_take_u8(&input), &out_root);
        break;
    }

    case 9u:
    {
        uint8_t bits[128] = {0u};
        size_t bits_len = fuzz_take_size_bounded(&input, sizeof(bits));
        fuzz_fill_bytes(&input, bits, bits_len);
        uint64_t bit_count = fuzz_take_u64_bounded(&input, 1024u);

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_bitvector(bits, bits_len, bit_count, hash_fn, &out_root);
        break;
    }

    case 10u:
    {
        uint8_t bits[128] = {0u};
        size_t bits_len = fuzz_take_size_bounded(&input, sizeof(bits));
        fuzz_fill_bytes(&input, bits, bits_len);

        uint64_t bit_len = fuzz_take_u64_bounded(&input, 1024u);
        uint64_t bit_limit =
            (fuzz_take_u8(&input) & 1u) != 0u ? SSZ_NO_LIMIT : fuzz_take_u64_bounded(&input, 1024u);

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_bitlist(bits, bits_len, bit_len, bit_limit, hash_fn, &out_root);
        break;
    }

    case 11u:
    {
        uint8_t elements[512] = {0u};
        size_t element_size = fuzz_take_size_bounded(&input, 32u);
        uint64_t element_count = 0u;

        if (element_size == 0u)
        {
            element_count = fuzz_take_u64_bounded(&input, 64u);
        }
        else
        {
            uint64_t max_count = (uint64_t)(sizeof(elements) / element_size);
            element_count = fuzz_take_u64_bounded(&input, max_count);
        }

        fuzz_fill_bytes(&input, elements, sizeof(elements));

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_vector_fixed(
            elements,
            element_count,
            element_size,
            hash_fn,
            &out_root);
        break;
    }

    case 12u:
    {
        uint64_t count = fuzz_take_u64_bounded(&input, 16u);
        ssz_chunk_t roots[16] = {{{0u}}};
        fuzz_fill_chunks(&input, roots, (size_t)count);

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_vector_roots(roots, count, hash_fn, &out_root);
        break;
    }

    case 13u:
    {
        ssz_chunk_t roots[16] = {{{0u}}};
        size_t root_count = fuzz_take_size_bounded(&input, 16u);
        fuzz_fill_chunks(&input, roots, root_count);

        uint64_t element_count = fuzz_take_u64_bounded(&input, 16u);
        fuzz_root_ctx_t root_ctx = {
            .roots = roots,
            .root_count = root_count,
            .mode = fuzz_take_u8(&input),
        };
        ssz_member_codec_t codec = {
            .ctx = &root_ctx,
            .write = NULL,
            .read = NULL,
            .root = fuzz_member_root,
        };

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_vector_composite(element_count, &codec, hash_fn, &out_root);
        break;
    }

    case 14u:
    {
        uint8_t elements[512] = {0u};
        size_t element_size = fuzz_take_size_bounded(&input, 32u);
        uint64_t element_count = 0u;

        if (element_size == 0u)
        {
            element_count = fuzz_take_u64_bounded(&input, 64u);
        }
        else
        {
            uint64_t max_count = (uint64_t)(sizeof(elements) / element_size);
            element_count = fuzz_take_u64_bounded(&input, max_count);
        }

        uint64_t element_limit =
            (fuzz_take_u8(&input) & 1u) != 0u ? SSZ_NO_LIMIT : fuzz_take_u64_bounded(&input, 64u);
        fuzz_fill_bytes(&input, elements, sizeof(elements));

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_list_fixed(
            elements,
            element_count,
            element_limit,
            element_size,
            hash_fn,
            &out_root);
        break;
    }

    case 15u:
    {
        uint64_t count = fuzz_take_u64_bounded(&input, 16u);
        uint64_t limit =
            (fuzz_take_u8(&input) & 1u) != 0u ? SSZ_NO_LIMIT : fuzz_take_u64_bounded(&input, 16u);

        ssz_chunk_t roots[16] = {{{0u}}};
        fuzz_fill_chunks(&input, roots, (size_t)count);

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_list_roots(roots, count, limit, hash_fn, &out_root);
        break;
    }

    case 16u:
    {
        ssz_chunk_t roots[16] = {{{0u}}};
        size_t root_count = fuzz_take_size_bounded(&input, 16u);
        fuzz_fill_chunks(&input, roots, root_count);

        uint64_t element_count = fuzz_take_u64_bounded(&input, 16u);
        uint64_t element_limit =
            (fuzz_take_u8(&input) & 1u) != 0u ? SSZ_NO_LIMIT : fuzz_take_u64_bounded(&input, 16u);
        fuzz_root_ctx_t root_ctx = {
            .roots = roots,
            .root_count = root_count,
            .mode = fuzz_take_u8(&input),
        };
        ssz_member_codec_t codec = {
            .ctx = &root_ctx,
            .write = NULL,
            .read = NULL,
            .root = fuzz_member_root,
        };

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_list_composite(
            element_count,
            element_limit,
            &codec,
            hash_fn,
            &out_root);
        break;
    }

    case 17u:
    {
        ssz_chunk_t root = {{0u}};
        fuzz_fill_bytes(&input, root.bytes, SSZ_BYTES_PER_CHUNK);
        uint8_t length[32] = {0u};
        fuzz_fill_bytes(&input, length, sizeof(length));

        ssz_chunk_t out_root;
        (void)ssz_mix_in_length(&root, length, sizeof(length), hash_fn, &out_root);
        break;
    }

    case 18u:
    {
        ssz_chunk_t root = {{0u}};
        fuzz_fill_bytes(&input, root.bytes, SSZ_BYTES_PER_CHUNK);
        uint8_t selector = fuzz_take_u8(&input);

        ssz_chunk_t out_root;
        (void)ssz_mix_in_selector(&root, selector, hash_fn, &out_root);
        break;
    }

    case 19u:
    {
        ssz_chunk_t root = {{0u}};
        fuzz_fill_bytes(&input, root.bytes, SSZ_BYTES_PER_CHUNK);

        uint8_t active_fields[40] = {0u};
        size_t active_fields_len = fuzz_take_size_bounded(&input, sizeof(active_fields));
        fuzz_fill_bytes(&input, active_fields, active_fields_len);

        ssz_chunk_t out_root;
        (void)ssz_mix_in_active_fields(&root, active_fields, active_fields_len, hash_fn, &out_root);
        break;
    }

    case 20u:
    {
        ssz_chunk_t roots[16] = {{{0u}}};
        size_t root_count = fuzz_take_size_bounded(&input, 16u);
        fuzz_fill_chunks(&input, roots, root_count);

        uint8_t selector = fuzz_take_u8(&input);
        bool has_none = (fuzz_take_u8(&input) & 1u) != 0u;
        uint8_t normal_selector = (uint8_t)((selector & 15u) + 1u);

        fuzz_root_ctx_t root_ctx = {
            .roots = roots,
            .root_count = root_count,
            .mode = fuzz_take_u8(&input),
        };
        ssz_member_codec_t codec = {
            .ctx = &root_ctx,
            .write = NULL,
            .read = NULL,
            .root = fuzz_member_root,
        };

        ssz_chunk_t out_root;
        (void)ssz_hash_tree_root_union(0u, true, &codec, hash_fn, &out_root);
        (void)ssz_hash_tree_root_union(selector, has_none, &codec, hash_fn, &out_root);
        (void)ssz_hash_tree_root_union(normal_selector, false, &codec, hash_fn, &out_root);
        break;
    }

    }

    fuzz_cover_merkle_errors();

    return 0;
}
