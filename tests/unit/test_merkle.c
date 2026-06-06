#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ssz.h"

static ssz_chunk_t g_test_merkle_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_test_merkle_scratch = {
    .chunks = g_test_merkle_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

static ssz_error_t expected_uint64_max_bit_count_error(ssz_error_t size_t_fit_error)
{
    const uint64_t byte_count = (UINT64_MAX / UINT64_C(8)) + UINT64_C(1);

    return (byte_count > (uint64_t)SIZE_MAX) ? SSZ_ERR_OVERFLOW : size_t_fit_error;
}

#define ssz_hash_tree_root_bitvector(bits_le, bits_le_len, bit_count, hash_fn, out_root) \
    ssz_hash_tree_root_bitvector(                                                        \
        (bits_le),                                                                       \
        (bits_le_len),                                                                   \
        (bit_count),                                                                     \
        &g_test_merkle_scratch,                                                          \
        (hash_fn),                                                                       \
        (out_root))
#define ssz_hash_tree_root_bitlist(bits_le, bits_le_len, bit_len, bit_limit, hash_fn, out_root) \
    ssz_hash_tree_root_bitlist(                                                                 \
        (bits_le),                                                                              \
        (bits_le_len),                                                                          \
        (bit_len),                                                                              \
        (bit_limit),                                                                            \
        &g_test_merkle_scratch,                                                                 \
        (hash_fn),                                                                              \
        (out_root))
#define ssz_hash_tree_root_vector_fixed(elements, element_count, element_size, hash_fn, out_root) \
    ssz_hash_tree_root_vector_fixed(                                                              \
        (elements),                                                                               \
        (element_count),                                                                          \
        (element_size),                                                                           \
        &g_test_merkle_scratch,                                                                   \
        (hash_fn),                                                                                \
        (out_root))
#define ssz_hash_tree_root_vector_composite(element_count, codec, hash_fn, out_root) \
    ssz_hash_tree_root_vector_composite(                                             \
        (element_count),                                                             \
        (codec),                                                                     \
        &g_test_merkle_scratch,                                                      \
        (hash_fn),                                                                   \
        (out_root))
#define ssz_hash_tree_root_vector_roots(roots, count, hash_fn, out_root) \
    ssz_hash_tree_root_vector_roots((roots), (count), &g_test_merkle_scratch, (hash_fn), (out_root))
#define ssz_hash_tree_root_list_fixed( \
    elements,                          \
    element_count,                     \
    element_limit,                     \
    element_size,                      \
    hash_fn,                           \
    out_root)                          \
    ssz_hash_tree_root_list_fixed(     \
        (elements),                    \
        (element_count),               \
        (element_limit),               \
        (element_size),                \
        &g_test_merkle_scratch,        \
        (hash_fn),                     \
        (out_root))
#define ssz_hash_tree_root_list_composite(element_count, element_limit, codec, hash_fn, out_root) \
    ssz_hash_tree_root_list_composite(                                                            \
        (element_count),                                                                          \
        (element_limit),                                                                          \
        (codec),                                                                                  \
        &g_test_merkle_scratch,                                                                   \
        (hash_fn),                                                                                \
        (out_root))
#define ssz_hash_tree_root_list_roots(roots, count, limit, hash_fn, out_root) \
    ssz_hash_tree_root_list_roots(                                            \
        (roots),                                                              \
        (count),                                                              \
        (limit),                                                              \
        &g_test_merkle_scratch,                                               \
        (hash_fn),                                                            \
        (out_root))
#define ssz_merkleize(chunks, chunk_count, limit, hash_fn, out_root) \
    ssz_merkleize((chunks), (chunk_count), (limit), &g_test_merkle_scratch, (hash_fn), (out_root))

typedef bool (*test_fn_t)(void);

typedef struct
{
    const char *name;
    test_fn_t fn;
} test_case_t;

#define ASSERT_TRUE(cond)                                                                  \
    do                                                                                     \
    {                                                                                      \
        if (!(cond))                                                                       \
        {                                                                                  \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            return false;                                                                  \
        }                                                                                  \
    } while (0)

#define ASSERT_ERR(expr, expected)                                                    \
    do                                                                                \
    {                                                                                 \
        ssz_error_t _actual = (expr);                                                 \
        if (_actual != (expected))                                                    \
        {                                                                             \
            fprintf(                                                                  \
                stderr,                                                               \
                "Assertion failed at %s:%d: %s returned %s (%d), expected %s (%d)\n", \
                __FILE__,                                                             \
                __LINE__,                                                             \
                #expr,                                                                \
                ssz_error_string(_actual),                                            \
                (int)_actual,                                                         \
                ssz_error_string((expected)),                                         \
                (int)(expected));                                                     \
            return false;                                                             \
        }                                                                             \
    } while (0)

#define ASSERT_CHUNK_EQ(actual, expected)                                                       \
    do                                                                                          \
    {                                                                                           \
        if (memcmp((actual).bytes, (expected).bytes, SSZ_BYTES_PER_CHUNK) != 0)                 \
        {                                                                                       \
            fprintf(stderr, "Assertion failed at %s:%d: chunk mismatch\n", __FILE__, __LINE__); \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

#define ASSERT_MEM_EQ(actual, expected, len)                                        \
    do                                                                              \
    {                                                                               \
        if (memcmp((actual), (expected), (len)) != 0)                               \
        {                                                                           \
            fprintf(                                                                \
                stderr,                                                             \
                "Assertion failed at %s:%d: memory mismatch (%s vs %s, len=%zu)\n", \
                __FILE__,                                                           \
                __LINE__,                                                           \
                #actual,                                                            \
                #expected,                                                          \
                (size_t)(len));                                                     \
            return false;                                                           \
        }                                                                           \
    } while (0)

typedef struct
{
    uint64_t id;
    ssz_chunk_t root;
} root_entry_t;

typedef struct
{
    const root_entry_t *entries;
    size_t entry_count;
} root_map_ctx_t;

static ssz_error_t root_map_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const root_map_ctx_t *map = (const root_map_ctx_t *)ctx;

    if ((map == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < map->entry_count; i++)
    {
        if (map->entries[i].id == member_id)
        {
            *out_root = map->entries[i].root;
            return SSZ_SUCCESS;
        }
    }

    return SSZ_ERR_INVALID_ARGUMENT;
}

static ssz_error_t fail_if_called_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    (void)ctx;
    (void)member_id;
    (void)out_root;
    return SSZ_ERR_TYPE_MISMATCH;
}

static ssz_chunk_t make_chunk(uint8_t seed)
{
    ssz_chunk_t chunk;
    for (size_t i = 0u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        chunk.bytes[i] = (uint8_t)(seed + (uint8_t)i);
    }
    return chunk;
}

static ssz_chunk_t *misaligned_chunk_ptr(void *storage)
{
    uintptr_t base = (uintptr_t)storage;
    uintptr_t aligned = (base + (uintptr_t)(SSZ_CHUNK_ALIGNMENT - 1u)) &
                        ~((uintptr_t)SSZ_CHUNK_ALIGNMENT - 1u);
    return (ssz_chunk_t *)(void *)(aligned + 1u);
}

static ssz_chunk_t zero_chunk(void)
{
    ssz_chunk_t chunk;
    memset(chunk.bytes, 0, sizeof(chunk.bytes));
    return chunk;
}

static void write_u64_le(uint8_t out[8], uint64_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
    out[4] = (uint8_t)((value >> 32u) & 0xFFu);
    out[5] = (uint8_t)((value >> 40u) & 0xFFu);
    out[6] = (uint8_t)((value >> 48u) & 0xFFu);
    out[7] = (uint8_t)((value >> 56u) & 0xFFu);
}

static void write_u64_as_u256_le(uint8_t out[32], uint64_t value)
{
    memset(out, 0, 32u);
    write_u64_le(out, value);
}

typedef struct
{
    ssz_error_t hash_err;
    ssz_error_t hash_2to1_err;
} hash_fail_ctx_t;

static ssz_error_t mock_hash_callback(
    const void *ctx,
    const uint8_t *data,
    size_t data_len,
    uint8_t out[32])
{
    const hash_fail_ctx_t *cfg = (const hash_fail_ctx_t *)ctx;
    (void)data;
    (void)data_len;
    if (out != NULL)
    {
        memset(out, 0u, 32u);
    }
    if (cfg == NULL)
    {
        return SSZ_SUCCESS;
    }
    return cfg->hash_err;
}

static ssz_error_t mock_hash_2to1_callback(
    const void *ctx,
    const ssz_chunk_t *left,
    const ssz_chunk_t *right,
    ssz_chunk_t *out)
{
    const hash_fail_ctx_t *cfg = (const hash_fail_ctx_t *)ctx;
    (void)left;
    (void)right;
    if (out != NULL)
    {
        memset(out->bytes, 0u, SSZ_BYTES_PER_CHUNK);
    }
    if (cfg == NULL)
    {
        return SSZ_SUCCESS;
    }
    return cfg->hash_2to1_err;
}

typedef struct
{
    const root_map_ctx_t *map;
    uint64_t fail_member;
    ssz_error_t fail_err;
} selective_root_ctx_t;

static ssz_error_t selective_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const selective_root_ctx_t *cfg = (const selective_root_ctx_t *)ctx;
    if ((cfg == NULL) || (cfg->map == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id == cfg->fail_member)
    {
        return cfg->fail_err;
    }
    return root_map_root(cfg->map, member_id, out_root);
}

static bool test_hash_tree_root_basic_types(void)
{
    ssz_chunk_t root;

    ASSERT_ERR(ssz_hash_tree_root_uint8(0xABu, &root), SSZ_SUCCESS);
    ASSERT_TRUE(root.bytes[0] == 0xABu);
    for (size_t i = 1u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(root.bytes[i] == 0u);
    }

    ASSERT_ERR(ssz_hash_tree_root_uint16(UINT16_C(0xBEEF), &root), SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, ((const uint8_t[2]){0xEFu, 0xBEu}), 2u);
    for (size_t i = 2u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(root.bytes[i] == 0u);
    }

    ASSERT_ERR(ssz_hash_tree_root_uint32(UINT32_C(0x78563412), &root), SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, ((const uint8_t[4]){0x12u, 0x34u, 0x56u, 0x78u}), 4u);

    ASSERT_ERR(ssz_hash_tree_root_uint64(UINT64_C(0x0102030405060708), &root), SSZ_SUCCESS);
    ASSERT_MEM_EQ(
        root.bytes,
        ((const uint8_t[8]){0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u}),
        8u);

    const uint8_t value128[16] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
        0xFFu,
    };
    ASSERT_ERR(ssz_hash_tree_root_uint128(value128, sizeof(value128), &root), SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, value128, sizeof(value128));
    for (size_t i = 16u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(root.bytes[i] == 0u);
    }

    const uint8_t value256[32] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu,
    };
    ASSERT_ERR(ssz_hash_tree_root_uint256(value256, sizeof(value256), &root), SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, value256, sizeof(value256));

    ASSERT_ERR(ssz_hash_tree_root_boolean(1u, &root), SSZ_SUCCESS);
    ASSERT_TRUE(root.bytes[0] == 1u);
    for (size_t i = 1u; i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(root.bytes[i] == 0u);
    }

    ASSERT_ERR(ssz_hash_tree_root_uint8(0x42u, &root), SSZ_SUCCESS);
    ASSERT_TRUE(root.bytes[0] == 0x42u);

    ASSERT_ERR(ssz_hash_tree_root_boolean(0u, &root), SSZ_SUCCESS);
    ASSERT_TRUE(root.bytes[0] == 0u);

    return true;
}

static bool test_hash_tree_root_bitvector_and_bitlist(void)
{
    const uint8_t bits[2] = {0x55u, 0x01u};
    ssz_chunk_t root_bitvector;
    ssz_chunk_t root_bitlist;

    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(bits, sizeof(bits), 10u, NULL, &root_bitvector),
        SSZ_SUCCESS);

    ssz_chunk_t expected_bitvector = zero_chunk();
    expected_bitvector.bytes[0] = 0x55u;
    expected_bitvector.bytes[1] = 0x01u;
    ASSERT_CHUNK_EQ(root_bitvector, expected_bitvector);

    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(bits, sizeof(bits), 10u, 10u, NULL, &root_bitlist),
        SSZ_SUCCESS);

    ssz_chunk_t length_chunk = zero_chunk();
    write_u64_le(length_chunk.bytes, 10u);
    ssz_chunk_t expected_bitlist;
    ASSERT_ERR(
        ssz_hash_2to1(NULL, &expected_bitvector, &length_chunk, &expected_bitlist),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_bitlist, expected_bitlist);

    return true;
}

static bool test_hash_tree_root_vector_fixed_pack_and_merkleize(void)
{
    uint8_t elements[40] = {0u};
    for (size_t i = 0u; i < sizeof(elements); i++)
    {
        elements[i] = (uint8_t)i;
    }

    ssz_chunk_t root;
    ASSERT_ERR(ssz_hash_tree_root_vector_fixed(elements, 5u, 8u, NULL, &root), SSZ_SUCCESS);

    ssz_chunk_t chunks[2];
    memset(chunks[0].bytes, 0, SSZ_BYTES_PER_CHUNK);
    memset(chunks[1].bytes, 0, SSZ_BYTES_PER_CHUNK);
    memcpy(chunks[0].bytes, elements, 32u);
    memcpy(chunks[1].bytes, elements + 32u, 8u);

    ssz_chunk_t expected;
    ASSERT_ERR(ssz_merkleize(chunks, 2u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    return true;
}

static bool test_hash_tree_root_vector_composite_and_roots_match(void)
{
    const root_entry_t entries[] = {
        {0u, make_chunk(0x10u)},
        {1u, make_chunk(0x40u)},
        {2u, make_chunk(0x70u)},
    };
    const root_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = NULL,
        .read = NULL,
        .root = root_map_root,
    };

    ssz_chunk_t composite_root;
    ssz_chunk_t roots_root;
    ssz_chunk_t expected;
    ssz_chunk_t roots[3] = {entries[0].root, entries[1].root, entries[2].root};

    ASSERT_ERR(ssz_hash_tree_root_vector_composite(3u, &codec, NULL, &composite_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_vector_roots(roots, 3u, NULL, &roots_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(roots, 3u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);

    ASSERT_CHUNK_EQ(composite_root, roots_root);
    ASSERT_CHUNK_EQ(composite_root, expected);

    return true;
}

static bool test_hash_tree_root_list_variants_and_mix_in_length(void)
{
    uint8_t fixed_elements[24] = {0u};
    for (size_t i = 0u; i < sizeof(fixed_elements); i++)
    {
        fixed_elements[i] = (uint8_t)(0xA0u + i);
    }

    ssz_chunk_t fixed_root;
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(fixed_elements, 3u, 6u, 8u, NULL, &fixed_root),
        SSZ_SUCCESS);

    ssz_chunk_t fixed_data_chunk = zero_chunk();
    memcpy(fixed_data_chunk.bytes, fixed_elements, sizeof(fixed_elements));
    ssz_chunk_t zero = zero_chunk();
    ssz_chunk_t fixed_data_root;
    ASSERT_ERR(ssz_hash_2to1(NULL, &fixed_data_chunk, &zero, &fixed_data_root), SSZ_SUCCESS);

    ssz_chunk_t length_chunk = zero_chunk();
    write_u64_le(length_chunk.bytes, 3u);
    ssz_chunk_t expected_fixed_root;
    ASSERT_ERR(
        ssz_hash_2to1(NULL, &fixed_data_root, &length_chunk, &expected_fixed_root),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(fixed_root, expected_fixed_root);

    const root_entry_t composite_entries[] = {
        {0u, make_chunk(0x01u)},
        {1u, make_chunk(0x31u)},
        {2u, make_chunk(0x61u)},
    };
    const root_map_ctx_t composite_ctx = {
        .entries = composite_entries,
        .entry_count = sizeof(composite_entries) / sizeof(composite_entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&composite_ctx,
        .write = NULL,
        .read = NULL,
        .root = root_map_root,
    };

    ssz_chunk_t composite_root;
    ssz_chunk_t roots_root;
    ssz_chunk_t expected_composite_root;
    const ssz_chunk_t roots[3] = {
        composite_entries[0].root,
        composite_entries[1].root,
        composite_entries[2].root,
    };

    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(3u, 4u, &codec, NULL, &composite_root),
        SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_tree_root_list_roots(roots, 3u, 4u, NULL, &roots_root), SSZ_SUCCESS);

    ssz_chunk_t data_root;
    ASSERT_ERR(ssz_merkleize(roots, 3u, 4u, NULL, &data_root), SSZ_SUCCESS);
    ASSERT_ERR(ssz_mix_in_length_u64(&data_root, 3u, NULL, &expected_composite_root), SSZ_SUCCESS);

    ASSERT_CHUNK_EQ(composite_root, roots_root);
    ASSERT_CHUNK_EQ(composite_root, expected_composite_root);

    return true;
}

static bool test_hash_tree_root_union_and_aliases(void)
{
    const ssz_member_codec_t forbidden_codec = {
        .ctx = NULL,
        .write = NULL,
        .read = NULL,
        .root = fail_if_called_root,
    };

    ssz_chunk_t none_root;
    ASSERT_ERR(ssz_hash_tree_root_union(0u, true, &forbidden_codec, NULL, &none_root), SSZ_SUCCESS);

    ssz_chunk_t zero = zero_chunk();
    ssz_chunk_t expected_none;
    ASSERT_ERR(ssz_mix_in_selector(&zero, 0u, NULL, &expected_none), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(none_root, expected_none);

    const root_entry_t entries[] = {
        {2u, make_chunk(0x44u)},
    };
    const root_map_ctx_t ctx = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };
    const ssz_member_codec_t codec = {
        .ctx = (void *)&ctx,
        .write = NULL,
        .read = NULL,
        .root = root_map_root,
    };

    ssz_chunk_t union_root;
    ASSERT_ERR(ssz_hash_tree_root_union(2u, true, &codec, NULL, &union_root), SSZ_SUCCESS);

    ssz_chunk_t expected_union;
    ASSERT_ERR(ssz_mix_in_selector(&entries[0].root, 2u, NULL, &expected_union), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(union_root, expected_union);

    const root_entry_t container_entries[] = {
        {0u, make_chunk(0x10u)},
        {1u, make_chunk(0x20u)},
    };
    const root_map_ctx_t container_ctx = {
        .entries = container_entries,
        .entry_count = sizeof(container_entries) / sizeof(container_entries[0]),
    };
    const ssz_member_codec_t container_codec = {
        .ctx = (void *)&container_ctx,
        .write = NULL,
        .read = NULL,
        .root = root_map_root,
    };
    const ssz_chunk_t container_roots[2] = {
        container_entries[0].root,
        container_entries[1].root,
    };

    ssz_chunk_t container_alias_root;
    ssz_chunk_t container_expected;
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(2u, &container_codec, NULL, &container_alias_root),
        SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_roots(container_roots, 2u, NULL, &container_expected),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(container_alias_root, container_expected);

    ssz_chunk_t compat_union_root;
    ASSERT_ERR(ssz_hash_tree_root_union(2u, true, &codec, NULL, &compat_union_root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(compat_union_root, union_root);

    return true;
}

static bool test_merkleize_edge_cases(void)
{
    const ssz_chunk_t c0 = make_chunk(0x00u);
    const ssz_chunk_t c1 = make_chunk(0x20u);
    const ssz_chunk_t c2 = make_chunk(0x40u);
    const ssz_chunk_t c3 = make_chunk(0x60u);
    const ssz_chunk_t chunks4[4] = {c0, c1, c2, c3};

    ssz_chunk_t root;

    ASSERT_ERR(ssz_merkleize(NULL, 0u, SSZ_NO_LIMIT, NULL, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, zero_chunk());

    ASSERT_ERR(ssz_merkleize(&c0, 1u, SSZ_NO_LIMIT, NULL, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, c0);

    ssz_chunk_t expected2;
    ASSERT_ERR(ssz_hash_2to1(NULL, &c0, &c1, &expected2), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkleize(((const ssz_chunk_t[]){c0, c1}), 2u, SSZ_NO_LIMIT, NULL, &root),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected2);

    ssz_chunk_t h01;
    ssz_chunk_t h23;
    ssz_chunk_t expected4;
    ASSERT_ERR(ssz_hash_2to1(NULL, &c0, &c1, &h01), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &c2, &c3, &h23), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &h01, &h23, &expected4), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(chunks4, 4u, SSZ_NO_LIMIT, NULL, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected4);

    ssz_chunk_t zero = zero_chunk();
    ssz_chunk_t h2z;
    ssz_chunk_t expected3;
    ASSERT_ERR(ssz_hash_2to1(NULL, &c2, &zero, &h2z), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &h01, &h2z, &expected3), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkleize(((const ssz_chunk_t[]){c0, c1, c2}), 3u, SSZ_NO_LIMIT, NULL, &root),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected3);

    ssz_chunk_t zz;
    ssz_chunk_t expected_limit4;
    ASSERT_ERR(ssz_hash_2to1(NULL, &zero, &zero, &zz), SSZ_SUCCESS);
    ASSERT_ERR(ssz_hash_2to1(NULL, &expected2, &zz, &expected_limit4), SSZ_SUCCESS);
    ASSERT_ERR(ssz_merkleize(((const ssz_chunk_t[]){c0, c1}), 2u, 4u, NULL, &root), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected_limit4);

    ASSERT_ERR(
        ssz_merkleize(((const ssz_chunk_t[]){c0, c1, c2}), 3u, 2u, NULL, &root),
        SSZ_ERR_LIMIT_EXCEEDED);

    return true;
}

static bool test_mix_in_helpers(void)
{
    const ssz_chunk_t base = make_chunk(0xAAu);
    ssz_chunk_t root;
    ssz_chunk_t root_u64;

    ASSERT_ERR(ssz_mix_in_length_u64(&base, 0x1122334455667788u, NULL, &root_u64), SSZ_SUCCESS);
    ssz_chunk_t length_chunk = zero_chunk();
    write_u64_le(length_chunk.bytes, 0x1122334455667788u);
    ssz_chunk_t expected_length_u64;
    ASSERT_ERR(ssz_hash_2to1(NULL, &base, &length_chunk, &expected_length_u64), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_u64, expected_length_u64);

    uint8_t length_u256[32];
    write_u64_as_u256_le(length_u256, 0x1122334455667788u);
    length_u256[8] = 0xA5u;
    length_u256[31] = 0x3Cu;

    ASSERT_ERR(
        ssz_mix_in_length(&base, length_u256, sizeof(length_u256), NULL, &root),
        SSZ_SUCCESS);
    memcpy(length_chunk.bytes, length_u256, 32u);
    ssz_chunk_t expected_length_u256;
    ASSERT_ERR(ssz_hash_2to1(NULL, &base, &length_chunk, &expected_length_u256), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected_length_u256);
    ASSERT_TRUE(memcmp(root.bytes, root_u64.bytes, SSZ_BYTES_PER_CHUNK) != 0);

    ASSERT_ERR(ssz_mix_in_selector(&base, 0x5Au, NULL, &root), SSZ_SUCCESS);
    ssz_chunk_t selector_chunk = zero_chunk();
    selector_chunk.bytes[0] = 0x5Au;
    ssz_chunk_t expected_selector;
    ASSERT_ERR(ssz_hash_2to1(NULL, &base, &selector_chunk, &expected_selector), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected_selector);

    const uint8_t active_fields[] = {0x05u, 0x80u};
    ASSERT_ERR(
        ssz_mix_in_active_fields(&base, active_fields, sizeof(active_fields), NULL, &root),
        SSZ_SUCCESS);
    ssz_chunk_t active_chunk = zero_chunk();
    memcpy(active_chunk.bytes, active_fields, sizeof(active_fields));
    ssz_chunk_t expected_active;
    ASSERT_ERR(ssz_hash_2to1(NULL, &base, &active_chunk, &expected_active), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected_active);

    return true;
}

static bool test_null_hash_fn_fallback_uses_default(void)
{
    const ssz_chunk_t chunks[2] = {make_chunk(0x01u), make_chunk(0x81u)};

    ssz_chunk_t root_null;
    ssz_chunk_t root_explicit;

    ASSERT_ERR(ssz_merkleize(chunks, 2u, SSZ_NO_LIMIT, NULL, &root_null), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_merkleize(chunks, 2u, SSZ_NO_LIMIT, ssz_hash_default(), &root_explicit),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root_null, root_explicit);

    ssz_chunk_t mixed_null;
    ssz_chunk_t mixed_explicit;
    ASSERT_ERR(ssz_mix_in_length_u64(&root_null, 123u, NULL, &mixed_null), SSZ_SUCCESS);
    ASSERT_ERR(
        ssz_mix_in_length_u64(&root_null, 123u, ssz_hash_default(), &mixed_explicit),
        SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(mixed_null, mixed_explicit);

    return true;
}

static bool test_merkle_additional_error_paths(void)
{
    ssz_chunk_t root = zero_chunk();
    const uint8_t one_byte = 0x01u;

    ASSERT_ERR(ssz_hash_tree_root_uint8(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_uint16(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_uint32(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_uint64(1u, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_uint128(NULL, 16u, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_uint128((const uint8_t[16]){0u}, 16u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_uint256(NULL, 32u, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_uint256((const uint8_t[32]){0u}, 32u, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_boolean(2u, &root), SSZ_ERR_ENCODING_INVALID);

    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(&one_byte, 1u, 1u, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(&one_byte, 1u, 0u, NULL, &root),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(&one_byte, 1u, UINT64_MAX, NULL, &root),
        expected_uint64_max_bit_count_error(SSZ_ERR_INVALID_ARGUMENT));
    ASSERT_ERR(ssz_hash_tree_root_bitvector(NULL, 0u, 8u, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(((const uint8_t[2]){0xFFu, 0xC0u}), 2u, 10u, NULL, &root),
        SSZ_ERR_ENCODING_INVALID);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(&one_byte, SIZE_MAX, UINT64_MAX, NULL, &root),
        SSZ_ERR_OVERFLOW);
    ASSERT_ERR(
        ssz_hash_tree_root_bitvector(&one_byte, SIZE_MAX, UINT64_MAX - 15u, NULL, &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(&one_byte, 1u, 1u, SSZ_NO_LIMIT, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(&one_byte, 1u, 5u, 4u, NULL, &root),
        SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(&one_byte, 1u, UINT64_MAX, SSZ_NO_LIMIT, NULL, &root),
        expected_uint64_max_bit_count_error(SSZ_ERR_INVALID_ARGUMENT));
    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(NULL, 0u, 1u, SSZ_NO_LIMIT, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(
            ((const uint8_t[2]){0x00u, 0xF0u}),
            2u,
            9u,
            SSZ_NO_LIMIT,
            NULL,
            &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(NULL, 0u, 0u, UINT64_MAX - 1u, NULL, &root),
        SSZ_ERR_OVERFLOW);

    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(((const uint8_t[]){0x01u}), 1u, 1u, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(((const uint8_t[]){0x01u}), 0u, 1u, NULL, &root),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(((const uint8_t[]){0x01u}), 1u, 0u, NULL, &root),
        SSZ_ERR_SCHEMA_INVALID);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(
            ((const uint8_t[]){0x01u}),
            ((uint64_t)SIZE_MAX / 2u) + 1u,
            2u,
            NULL,
            &root),
        SSZ_ERR_OVERFLOW);
#endif
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(NULL, 1u, 1u, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(&one_byte, SIZE_MAX, 1u, NULL, &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(1u, NULL, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(
            1u,
            (&(ssz_member_codec_t){.ctx = NULL,
                                   .write = NULL,
                                   .read = NULL,
                                   .root = fail_if_called_root}),
            NULL,
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(
            0u,
            (&(ssz_member_codec_t){.ctx = NULL,
                                   .write = NULL,
                                   .read = NULL,
                                   .root = fail_if_called_root}),
            NULL,
            &root),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(
            1u,
            (&(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL}),
            NULL,
            &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_vector_roots(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            0u,
            NULL,
            &root),
        SSZ_ERR_SCHEMA_INVALID);
#if SIZE_MAX < UINT64_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_vector_roots(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            (uint64_t)SIZE_MAX + 1u,
            NULL,
            &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(((const uint8_t[]){0x01u}), 1u, SSZ_NO_LIMIT, 1u, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(
            ((const uint8_t[]){0x01u}),
            1u,
            SSZ_NO_LIMIT,
            0u,
            NULL,
            &root),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(((const uint8_t[]){0x01u}), 2u, 1u, 1u, NULL, &root),
        SSZ_ERR_LIMIT_EXCEEDED);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(
            ((const uint8_t[]){0x01u}),
            ((uint64_t)SIZE_MAX / 2u) + 1u,
            SSZ_NO_LIMIT,
            2u,
            NULL,
            &root),
        SSZ_ERR_OVERFLOW);
#endif
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(NULL, 1u, SSZ_NO_LIMIT, 1u, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(NULL, 0u, 2u, SIZE_MAX, NULL, &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(1u, SSZ_NO_LIMIT, NULL, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(
            1u,
            SSZ_NO_LIMIT,
            (&(ssz_member_codec_t){.ctx = NULL,
                                   .write = NULL,
                                   .read = NULL,
                                   .root = fail_if_called_root}),
            NULL,
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(
            2u,
            1u,
            (&(ssz_member_codec_t){.ctx = NULL,
                                   .write = NULL,
                                   .read = NULL,
                                   .root = fail_if_called_root}),
            NULL,
            &root),
        SSZ_ERR_LIMIT_EXCEEDED);
    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(
            1u,
            SSZ_NO_LIMIT,
            (&(ssz_member_codec_t){.ctx = NULL, .write = NULL, .read = NULL, .root = NULL}),
            NULL,
            &root),
        SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_hash_tree_root_list_roots(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            1u,
            SSZ_NO_LIMIT,
            NULL,
            NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_hash_tree_root_list_roots(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            2u,
            1u,
            NULL,
            &root),
        SSZ_ERR_LIMIT_EXCEEDED);
#if SIZE_MAX < UINT64_MAX
    ASSERT_ERR(
        ssz_hash_tree_root_list_roots(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            (uint64_t)SIZE_MAX + 1u,
            SSZ_NO_LIMIT,
            NULL,
            &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(ssz_hash_tree_root_union(1u, true, NULL, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_hash_tree_root_union(1u, false, NULL, NULL, NULL), SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_merkleize(((const ssz_chunk_t[]){make_chunk(0x01u)}), 1u, SSZ_NO_LIMIT, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_merkleize(NULL, 1u, SSZ_NO_LIMIT, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
#if SIZE_MAX > UINT32_MAX
    ASSERT_ERR(
        ssz_merkleize(
            ((const ssz_chunk_t[]){make_chunk(0x01u)}),
            SIZE_MAX,
            SSZ_NO_LIMIT,
            NULL,
            &root),
        SSZ_ERR_OVERFLOW);
#endif

    ASSERT_ERR(
        ssz_mix_in_length(NULL, (const uint8_t[32]){0u}, 32u, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_mix_in_length(&root, NULL, 32u, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_mix_in_length(&root, (const uint8_t[32]){0u}, 32u, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_mix_in_length_u64(NULL, 1u, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_mix_in_length_u64(&root, 1u, NULL, NULL), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_mix_in_selector(NULL, 1u, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(ssz_mix_in_selector(&root, 1u, NULL, NULL), SSZ_ERR_INVALID_ARGUMENT);

    ASSERT_ERR(
        ssz_mix_in_active_fields(NULL, (const uint8_t[]){0x01u}, 1u, NULL, &root),
        SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_mix_in_active_fields(&root, (const uint8_t[33]){0u}, 33u, NULL, &root),
        SSZ_ERR_SCHEMA_INVALID);
    ASSERT_ERR(ssz_mix_in_active_fields(&root, NULL, 1u, NULL, &root), SSZ_ERR_INVALID_ARGUMENT);
    ASSERT_ERR(
        ssz_mix_in_active_fields(&root, (const uint8_t[]){0x01u}, 1u, NULL, NULL),
        SSZ_ERR_INVALID_ARGUMENT);

    return true;
}

static bool test_merkle_failure_propagation_paths(void)
{
    ssz_chunk_t root;

    hash_fail_ctx_t hash_fail_cfg = {
        .hash_err = SSZ_SUCCESS,
        .hash_2to1_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_hash_fn_t failing_hash = {
        .hash = mock_hash_callback,
        .hash_2to1 = mock_hash_2to1_callback,
        .hash_2to1_batch = NULL,
        .ctx = &hash_fail_cfg,
    };

    const ssz_chunk_t merkle_chunks[2] = {make_chunk(0x01u), make_chunk(0x21u)};
    ASSERT_ERR(
        ssz_merkleize(merkle_chunks, 2u, SSZ_NO_LIMIT, &failing_hash, &root),
        SSZ_ERR_HASH_FAILURE);

    ASSERT_ERR(
        ssz_hash_tree_root_bitlist(
            ((const uint8_t[]){0xAAu, 0xBBu, 0xCCu, 0xDDu, 0xEEu, 0xFFu, 0x11u, 0x22u, 0x33u,
                               0x44u, 0x55u, 0x66u, 0x77u, 0x88u, 0x99u, 0xAAu, 0xBBu, 0xCCu,
                               0xDDu, 0xEEu, 0xFFu, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u,
                               0x77u, 0x88u, 0x99u, 0xAAu, 0xBBu, 0x0Cu}),
            33u,
            260u,
            SSZ_NO_LIMIT,
            &failing_hash,
            &root),
        SSZ_ERR_HASH_FAILURE);
    ASSERT_ERR(
        ssz_hash_tree_root_list_fixed(
            ((const uint8_t[]){0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u,
                               0x0Au, 0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u,
                               0x13u, 0x14u, 0x15u, 0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu,
                               0x1Cu, 0x1Du, 0x1Eu, 0x1Fu, 0x20u, 0x21u}),
            33u,
            SSZ_NO_LIMIT,
            1u,
            &failing_hash,
            &root),
        SSZ_ERR_HASH_FAILURE);
    ASSERT_ERR(
        ssz_hash_tree_root_list_roots(merkle_chunks, 2u, SSZ_NO_LIMIT, &failing_hash, &root),
        SSZ_ERR_HASH_FAILURE);

    const root_entry_t entries[] = {
        {0u, make_chunk(0x31u)},
        {1u, make_chunk(0x51u)},
    };
    const root_map_ctx_t map = {
        .entries = entries,
        .entry_count = sizeof(entries) / sizeof(entries[0]),
    };

    selective_root_ctx_t left_fail_ctx = {
        .map = &map,
        .fail_member = 0u,
        .fail_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_member_codec_t left_fail_codec = {
        .ctx = &left_fail_ctx,
        .write = NULL,
        .read = NULL,
        .root = selective_root,
    };
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(2u, &left_fail_codec, NULL, &root),
        SSZ_ERR_TYPE_MISMATCH);
    ASSERT_ERR(
        ssz_hash_tree_root_list_composite(2u, SSZ_NO_LIMIT, &left_fail_codec, NULL, &root),
        SSZ_ERR_TYPE_MISMATCH);

    selective_root_ctx_t right_fail_ctx = {
        .map = &map,
        .fail_member = 1u,
        .fail_err = SSZ_ERR_TYPE_MISMATCH,
    };
    const ssz_member_codec_t right_fail_codec = {
        .ctx = &right_fail_ctx,
        .write = NULL,
        .read = NULL,
        .root = selective_root,
    };
    ASSERT_ERR(
        ssz_hash_tree_root_vector_composite(2u, &right_fail_codec, NULL, &root),
        SSZ_ERR_TYPE_MISMATCH);
    ASSERT_ERR(
        ssz_hash_tree_root_union(1u, false, &right_fail_codec, NULL, &root),
        SSZ_ERR_TYPE_MISMATCH);

    return true;
}

static bool test_bytes_reader_batch_path_large_vector(void)
{
    /* 128 chunks = 4096 bytes; chunk count is a power of two and exceeds the
       64-leaf stack threshold, so this exercises the bytes-reader batch fast
       path in ssz_internal_merkleize_reader_fast (ssz_merkle.c ~lines 275-305). */
    uint8_t buf[128u * SSZ_BYTES_PER_CHUNK];
    ssz_chunk_t root;
    ssz_chunk_t expected;
    ssz_chunk_t chunks[128];

    for (size_t i = 0u; i < sizeof(buf); i++)
    {
        buf[i] = (uint8_t)(i & 0xFFu);
    }

    ASSERT_ERR(
        ssz_hash_tree_root_vector_fixed(buf, 128u * SSZ_BYTES_PER_CHUNK, 1u, NULL, &root),
        SSZ_SUCCESS);

    /* Verify against chunk-based merkleize for correctness. */
    for (size_t i = 0u; i < 128u; i++)
    {
        memcpy(chunks[i].bytes, buf + (i * SSZ_BYTES_PER_CHUNK), SSZ_BYTES_PER_CHUNK);
    }
    ASSERT_ERR(ssz_merkleize(chunks, 128u, SSZ_NO_LIMIT, NULL, &expected), SSZ_SUCCESS);
    ASSERT_CHUNK_EQ(root, expected);

    return true;
}

static bool test_merkle_fixed_width_exact_and_invalid_lengths(void)
{
    const uint8_t value128_exact[16] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
        0xFFu,
    };
    const uint8_t value128[15] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
    };
    const uint8_t value128_overlong[17] = {
        0x00u,
        0x11u,
        0x22u,
        0x33u,
        0x44u,
        0x55u,
        0x66u,
        0x77u,
        0x88u,
        0x99u,
        0xAAu,
        0xBBu,
        0xCCu,
        0xDDu,
        0xEEu,
        0xFFu,
        0x42u,
    };
    const uint8_t value256_exact[32] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu,
    };
    const uint8_t value256[31] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu,
    };
    const uint8_t value256_overlong[33] = {
        0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x0Au,
        0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0x0Fu, 0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u,
        0x16u, 0x17u, 0x18u, 0x19u, 0x1Au, 0x1Bu, 0x1Cu, 0x1Du, 0x1Eu, 0x1Fu, 0x42u,
    };
    const uint8_t length_overlong[33] = {
        0x88u, 0x77u, 0x66u, 0x55u, 0x44u, 0x33u, 0x22u, 0x11u, 0x10u, 0x20u, 0x30u,
        0x40u, 0x50u, 0x60u, 0x70u, 0x80u, 0x90u, 0xA0u, 0xB0u, 0xC0u, 0xD0u, 0xE0u,
        0xF0u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u, 0x09u, 0x42u,
    };
    const uint8_t length[31] = {
        0x88u, 0x77u, 0x66u, 0x55u, 0x44u, 0x33u, 0x22u, 0x11u, 0x10u, 0x20u, 0x30u,
        0x40u, 0x50u, 0x60u, 0x70u, 0x80u, 0x90u, 0xA0u, 0xB0u, 0xC0u, 0xD0u, 0xE0u,
        0xF0u, 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u,
    };
    ssz_chunk_t root = make_chunk(0xA5u);
    const ssz_chunk_t expected_invalid = make_chunk(0xA5u);
    const ssz_chunk_t base = make_chunk(0xABu);

    ASSERT_ERR(
        ssz_hash_tree_root_uint128(value128_exact, sizeof(value128_exact), &root),
        SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, value128_exact, sizeof(value128_exact));
    for (size_t i = sizeof(value128_exact); i < SSZ_BYTES_PER_CHUNK; i++)
    {
        ASSERT_TRUE(root.bytes[i] == 0u);
    }

    ASSERT_ERR(
        ssz_hash_tree_root_uint256(value256_exact, sizeof(value256_exact), &root),
        SSZ_SUCCESS);
    ASSERT_MEM_EQ(root.bytes, value256_exact, sizeof(value256_exact));

    root = expected_invalid;
    ASSERT_ERR(
        ssz_hash_tree_root_uint128(value128, sizeof(value128), &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_uint128(value128_overlong, sizeof(value128_overlong), &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_uint256(value256, sizeof(value256), &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_hash_tree_root_uint256(value256_overlong, sizeof(value256_overlong), &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_mix_in_length(&base, length, sizeof(length), NULL, &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_ERR(
        ssz_mix_in_length(&base, length_overlong, sizeof(length_overlong), NULL, &root),
        SSZ_ERR_ENCODING_INVALID);
    ASSERT_CHUNK_EQ(root, expected_invalid);

    return true;
}

static bool test_merkle_alignment_error_paths(void)
{
    const ssz_chunk_t chunk = make_chunk(0x41u);
    ssz_chunk_t out = zero_chunk();
    uint8_t out_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t chunks_raw[sizeof(ssz_chunk_t) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t scratch_raw[(sizeof(ssz_chunk_t) * 2u) + SSZ_CHUNK_ALIGNMENT] = {0u};
    uint8_t length[SSZ_BYTES_PER_CHUNK] = {0u};
    const ssz_merkle_scratch_t misaligned_scratch = {
        .chunks = misaligned_chunk_ptr(scratch_raw),
        .chunk_count = 2u,
    };

    ASSERT_TRUE(SSZ_CHUNK_ALIGNMENT > 1u);

    (void)memcpy((void *)misaligned_chunk_ptr(chunks_raw), &chunk, sizeof(chunk));
    (void)memcpy((void *)misaligned_chunk_ptr(out_raw), &chunk, sizeof(chunk));

    ASSERT_ERR(ssz_hash_tree_root_uint64(7u, misaligned_chunk_ptr(out_raw)), SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR(ssz_merkleize(
                   (const ssz_chunk_t *)(const void *)misaligned_chunk_ptr(chunks_raw),
                   1u,
                   SSZ_NO_LIMIT,
                   NULL,
                   &out),
               SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR((ssz_merkleize)(&chunk, 1u, SSZ_NO_LIMIT, &misaligned_scratch, NULL, &out),
               SSZ_ERR_ALIGNMENT_INVALID);
    ASSERT_ERR(ssz_mix_in_length((const ssz_chunk_t *)(const void *)misaligned_chunk_ptr(chunks_raw),
                                 length,
                                 sizeof(length),
                                 NULL,
                                 &out),
               SSZ_ERR_ALIGNMENT_INVALID);

    return true;
}

int main(void)
{
    const test_case_t tests[] = {
        {"hash_tree_root_basic_types", test_hash_tree_root_basic_types},
        {"hash_tree_root_bitvector_and_bitlist", test_hash_tree_root_bitvector_and_bitlist},
        {"hash_tree_root_vector_fixed_pack_and_merkleize",
         test_hash_tree_root_vector_fixed_pack_and_merkleize},
        {"hash_tree_root_vector_composite_and_roots_match",
         test_hash_tree_root_vector_composite_and_roots_match},
        {"hash_tree_root_list_variants_and_mix_in_length",
         test_hash_tree_root_list_variants_and_mix_in_length},
        {"hash_tree_root_union_and_aliases", test_hash_tree_root_union_and_aliases},
        {"merkleize_edge_cases", test_merkleize_edge_cases},
        {"mix_in_helpers", test_mix_in_helpers},
        {"null_hash_fn_fallback_uses_default", test_null_hash_fn_fallback_uses_default},
        {"merkle_additional_error_paths", test_merkle_additional_error_paths},
        {"merkle_fixed_width_exact_and_invalid_lengths",
         test_merkle_fixed_width_exact_and_invalid_lengths},
        {"merkle_alignment_error_paths", test_merkle_alignment_error_paths},
        {"merkle_failure_propagation_paths", test_merkle_failure_propagation_paths},
        {"bytes_reader_batch_path_large_vector", test_bytes_reader_batch_path_large_vector},
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

    printf("[OK] %zu/%zu merkle tests passed\n", passed, total);
    return 0;
}
