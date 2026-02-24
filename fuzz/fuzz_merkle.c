#include <stddef.h>
#include <stdint.h>

#include "ssz.h"

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

static size_t fuzz_make_active_fields(
    uint8_t *active_fields,
    size_t active_fields_cap,
    uint32_t field_count)
{
    if ((active_fields == NULL) || (active_fields_cap == 0u))
    {
        return 0u;
    }

    for (size_t i = 0u; i < active_fields_cap; i++)
    {
        active_fields[i] = 0u;
    }
    if (field_count == 0u)
    {
        return 0u;
    }

    size_t active_fields_len = (size_t)((field_count + 7u) / 8u);
    if (active_fields_len > active_fields_cap)
    {
        return 0u;
    }

    for (uint32_t i = 0u; i < field_count; i++)
    {
        size_t byte_index = (size_t)(i / 8u);
        uint8_t bit_mask = (uint8_t)(1u << (i % 8u));
        active_fields[byte_index] = (uint8_t)(active_fields[byte_index] | bit_mask);
    }

    return active_fields_len;
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

    switch (api_selector % 27u)
    {
        case 0u:
        {
            size_t chunk_count = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t chunks[16] = {{{0u}}};
            fuzz_fill_chunks(&input, chunks, chunk_count);

            uint64_t limit = (fuzz_take_u8(&input) & 1u) != 0u
                                 ? SSZ_NO_LIMIT
                                 : fuzz_take_u64_bounded(&input, 32u);
            ssz_chunk_t out_root;
            (void)ssz_merkleize(chunks, chunk_count, limit, hash_fn, &out_root);
            break;
        }

        case 1u:
        {
            size_t chunk_count = fuzz_take_size_bounded(&input, 16u);
            ssz_chunk_t chunks[16] = {{{0u}}};
            fuzz_fill_chunks(&input, chunks, chunk_count);

            ssz_chunk_t out_root;
            (void)ssz_merkleize_progressive(chunks, chunk_count, hash_fn, &out_root);
            break;
        }

        case 2u:
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
            (void)ssz_hash_tree_root_uint128(value, &out_root);
            break;
        }

        case 7u:
        {
            uint8_t value[32] = {0u};
            fuzz_fill_bytes(&input, value, sizeof(value));

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_uint256(value, &out_root);
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
            uint64_t bit_limit = (fuzz_take_u8(&input) & 1u) != 0u
                                     ? SSZ_NO_LIMIT
                                     : fuzz_take_u64_bounded(&input, 1024u);

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_bitlist(
                bits,
                bits_len,
                bit_len,
                bit_limit,
                hash_fn,
                &out_root);
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

            uint64_t element_limit = (fuzz_take_u8(&input) & 1u) != 0u
                                         ? SSZ_NO_LIMIT
                                         : fuzz_take_u64_bounded(&input, 64u);
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
            uint64_t limit = (fuzz_take_u8(&input) & 1u) != 0u
                                 ? SSZ_NO_LIMIT
                                 : fuzz_take_u64_bounded(&input, 16u);

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
            uint64_t element_limit = (fuzz_take_u8(&input) & 1u) != 0u
                                         ? SSZ_NO_LIMIT
                                         : fuzz_take_u64_bounded(&input, 16u);
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
            uint64_t length = fuzz_take_u64(&input);

            ssz_chunk_t out_root;
            (void)ssz_mix_in_length(&root, length, hash_fn, &out_root);
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
            (void)ssz_mix_in_active_fields(
                &root,
                active_fields,
                active_fields_len,
                hash_fn,
                &out_root);
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

        case 21u:
        {
            uint32_t field_count = (uint32_t)(fuzz_take_u64_bounded(&input, 7u) + 1u);
            ssz_chunk_t roots[8] = {{{0u}}};
            fuzz_fill_chunks(&input, roots, field_count);

            uint8_t active_fields[1] = {0u};
            size_t active_fields_len =
                fuzz_make_active_fields(active_fields, sizeof(active_fields), field_count);

            fuzz_root_ctx_t root_ctx = {
                .roots = roots,
                .root_count = field_count,
                .mode = (uint8_t)(fuzz_take_u8(&input) & 2u),
            };
            ssz_member_codec_t codec = {
                .ctx = &root_ctx,
                .write = NULL,
                .read = NULL,
                .root = fuzz_member_root,
            };

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_container(
                field_count,
                active_fields,
                active_fields_len,
                &codec,
                hash_fn,
                &out_root);
            break;
        }

        case 22u:
        {
            uint64_t element_count = fuzz_take_u64_bounded(&input, 16u);
            size_t element_size = fuzz_take_size_bounded(&input, 31u) + 1u;
            size_t total_bytes = (size_t)element_count * element_size;

            uint8_t elements[512] = {0u};
            fuzz_fill_bytes(&input, elements, total_bytes);

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_list_fixed(
                elements,
                element_count,
                element_size,
                hash_fn,
                &out_root);
            break;
        }

        case 23u:
        {
            uint64_t element_count = fuzz_take_u64_bounded(&input, 16u);
            ssz_chunk_t roots[16] = {{{0u}}};
            fuzz_fill_chunks(&input, roots, sizeof(roots) / sizeof(roots[0]));

            fuzz_root_ctx_t root_ctx = {
                .roots = roots,
                .root_count = sizeof(roots) / sizeof(roots[0]),
                .mode = (uint8_t)(fuzz_take_u8(&input) & 2u),
            };
            ssz_member_codec_t codec = {
                .ctx = &root_ctx,
                .write = NULL,
                .read = NULL,
                .root = fuzz_member_root,
            };

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_list_composite(
                element_count,
                &codec,
                hash_fn,
                &out_root);
            break;
        }

        case 24u:
        {
            uint8_t bits[128] = {0u};
            uint64_t bit_len = fuzz_take_u64_bounded(&input, 1024u);
            size_t required_bits_len = (size_t)((bit_len + 7u) / 8u);
            size_t bits_len = fuzz_take_size_bounded(&input, sizeof(bits));
            if (((fuzz_take_u8(&input) & 1u) == 0u) && (bits_len < required_bits_len) &&
                (required_bits_len <= sizeof(bits)))
            {
                bits_len = required_bits_len;
            }
            fuzz_fill_bytes(&input, bits, bits_len);

            if ((bit_len != 0u) && ((bit_len % 8u) != 0u) && (required_bits_len != 0u) &&
                (required_bits_len <= bits_len))
            {
                uint8_t mask = (uint8_t)((1u << (bit_len % 8u)) - 1u);
                bits[required_bits_len - 1u] = (uint8_t)(bits[required_bits_len - 1u] & mask);
            }

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_bitlist(
                bits,
                bits_len,
                bit_len,
                hash_fn,
                &out_root);
            break;
        }

        case 25u:
        {
            uint32_t count = (uint32_t)(fuzz_take_u64_bounded(&input, 7u) + 1u);
            ssz_chunk_t roots[8] = {{{0u}}};
            fuzz_fill_chunks(&input, roots, count);

            uint8_t active_fields[1] = {0u};
            size_t active_fields_len =
                fuzz_make_active_fields(active_fields, sizeof(active_fields), count);

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_container_roots(
                roots,
                count,
                active_fields,
                active_fields_len,
                hash_fn,
                &out_root);
            break;
        }

        case 26u:
        {
            uint64_t count = fuzz_take_u64_bounded(&input, 16u);
            ssz_chunk_t roots[16] = {{{0u}}};
            fuzz_fill_chunks(&input, roots, (size_t)count);

            ssz_chunk_t out_root;
            (void)ssz_hash_tree_root_progressive_list_roots(roots, count, hash_fn, &out_root);
            break;
        }
    }

    return 0;
}
