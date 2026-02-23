#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/progressive_containers"
#endif

typedef enum
{
    TYPE_UINT8 = 0,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,
    TYPE_UINT128,
    TYPE_UINT256,
    TYPE_BITVECTOR,
    TYPE_BITLIST,
    TYPE_PROGRESSIVE_BITLIST,
    TYPE_BYTE_LIST,
    TYPE_VECTOR,
    TYPE_LIST,
    TYPE_PROGRESSIVE_LIST,
    TYPE_CONTAINER,
} value_type_kind_t;

typedef struct value_type value_type_t;

typedef struct
{
    const char *name;
    const value_type_t *type;
    uint8_t active_index;
} field_def_t;

typedef struct
{
    const char *name;
    const field_def_t *fields;
    uint32_t field_count;
    bool is_progressive;
} container_def_t;

struct value_type
{
    value_type_kind_t kind;
    const value_type_t *elem_type;
    uint64_t param;
    const container_def_t *container;
};

typedef struct
{
    uint8_t *bytes;
    size_t len;
    ssz_chunk_t root;
} computed_value_t;

static ssz_error_t compute_value_from_yaml(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value);
static ssz_error_t validate_value_bytes(
    const value_type_t *type,
    const uint8_t *bytes,
    size_t len);

static bool add_overflow_size(size_t a, size_t b, size_t *out)
{
    if (out == NULL)
    {
        return true;
    }
    if (a > (SIZE_MAX - b))
    {
        return true;
    }
    *out = a + b;
    return false;
}

static bool mul_overflow_size(size_t a, size_t b, size_t *out)
{
    if (out == NULL)
    {
        return true;
    }
    if ((a != 0u) && (b > (SIZE_MAX / a)))
    {
        return true;
    }
    *out = a * b;
    return false;
}

static bool bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    if (out_bytes == NULL)
    {
        return false;
    }
    if (bit_count > (uint64_t)SIZE_MAX * 8u)
    {
        return false;
    }
    *out_bytes = (size_t)((bit_count + 7u) / 8u);
    return true;
}

static bool value_type_is_basic(const value_type_t *type)
{
    if (type == NULL)
    {
        return false;
    }

    return (type->kind == TYPE_UINT8) || (type->kind == TYPE_UINT16) ||
        (type->kind == TYPE_UINT32) || (type->kind == TYPE_UINT64) ||
        (type->kind == TYPE_UINT128) || (type->kind == TYPE_UINT256);
}

static bool value_type_is_fixed(const value_type_t *type, size_t *out_size)
{
    size_t elem_size;
    size_t total;

    if ((type == NULL) || (out_size == NULL))
    {
        return false;
    }

    if (type->kind == TYPE_UINT8)
    {
        *out_size = 1u;
        return true;
    }
    if (type->kind == TYPE_UINT16)
    {
        *out_size = 2u;
        return true;
    }
    if (type->kind == TYPE_UINT32)
    {
        *out_size = 4u;
        return true;
    }
    if (type->kind == TYPE_UINT64)
    {
        *out_size = 8u;
        return true;
    }
    if (type->kind == TYPE_UINT128)
    {
        *out_size = 16u;
        return true;
    }
    if (type->kind == TYPE_UINT256)
    {
        *out_size = 32u;
        return true;
    }
    if (type->kind == TYPE_BITVECTOR)
    {
        return bits_to_bytes(type->param, out_size);
    }

    if ((type->kind == TYPE_BITLIST) || (type->kind == TYPE_PROGRESSIVE_BITLIST) ||
        (type->kind == TYPE_BYTE_LIST) || (type->kind == TYPE_LIST) ||
        (type->kind == TYPE_PROGRESSIVE_LIST))
    {
        return false;
    }

    if (type->kind == TYPE_VECTOR)
    {
        if ((type->elem_type == NULL) || !value_type_is_fixed(type->elem_type, &elem_size))
        {
            return false;
        }

        if (type->param > (uint64_t)SIZE_MAX)
        {
            return false;
        }

        if (mul_overflow_size((size_t)type->param, elem_size, &total))
        {
            return false;
        }

        *out_size = total;
        return true;
    }

    if (type->kind == TYPE_CONTAINER)
    {
        size_t total_size = 0u;
        if ((type->container == NULL) || (type->container->fields == NULL) ||
            (type->container->field_count == 0u))
        {
            return false;
        }

        for (uint32_t i = 0u; i < type->container->field_count; i++)
        {
            const value_type_t *field_type = type->container->fields[i].type;
            size_t field_size = 0u;
            if ((field_type == NULL) || !value_type_is_fixed(field_type, &field_size))
            {
                return false;
            }
            if (add_overflow_size(total_size, field_size, &total_size))
            {
                return false;
            }
        }

        *out_size = total_size;
        return true;
    }

    return false;
}

static bool value_type_min_size(const value_type_t *type, size_t *out_min)
{
    size_t fixed_size;

    if ((type == NULL) || (out_min == NULL))
    {
        return false;
    }

    if (value_type_is_fixed(type, &fixed_size))
    {
        *out_min = fixed_size;
        return true;
    }

    if ((type->kind == TYPE_BITLIST) || (type->kind == TYPE_PROGRESSIVE_BITLIST))
    {
        *out_min = 1u;
        return true;
    }

    if ((type->kind == TYPE_BYTE_LIST) || (type->kind == TYPE_LIST) ||
        (type->kind == TYPE_PROGRESSIVE_LIST))
    {
        *out_min = 0u;
        return true;
    }

    if (type->kind == TYPE_VECTOR)
    {
        size_t elem_min = 0u;
        size_t total = 0u;

        if ((type->elem_type == NULL) || !value_type_min_size(type->elem_type, &elem_min))
        {
            return false;
        }
        if (type->param > (uint64_t)SIZE_MAX)
        {
            return false;
        }
        if (mul_overflow_size((size_t)type->param, elem_min, &total))
        {
            return false;
        }

        *out_min = total;
        return true;
    }

    if (type->kind == TYPE_CONTAINER)
    {
        size_t total = 0u;

        if ((type->container == NULL) || (type->container->fields == NULL))
        {
            return false;
        }

        for (uint32_t i = 0u; i < type->container->field_count; i++)
        {
            const value_type_t *field_type = type->container->fields[i].type;
            size_t field_fixed = 0u;
            size_t field_min = 0u;

            if (value_type_is_fixed(field_type, &field_fixed))
            {
                if (add_overflow_size(total, field_fixed, &total))
                {
                    return false;
                }
                continue;
            }

            if (!value_type_min_size(field_type, &field_min))
            {
                return false;
            }
            if (add_overflow_size(total, 4u, &total) || add_overflow_size(total, field_min, &total))
            {
                return false;
            }
        }

        *out_min = total;
        return true;
    }

    return false;
}

static void computed_value_reset(computed_value_t *value)
{
    if (value == NULL)
    {
        return;
    }

    free(value->bytes);
    value->bytes = NULL;
    value->len = 0u;
    memset(&value->root, 0, sizeof(value->root));
}

static ssz_error_t copy_bytes(const uint8_t *src, size_t len, uint8_t **out)
{
    uint8_t *copy;

    if ((src == NULL && len != 0u) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    copy = (uint8_t *)malloc(len == 0u ? 1u : len);
    if (copy == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (len != 0u)
    {
        memcpy(copy, src, len);
    }

    *out = copy;
    return SSZ_SUCCESS;
}

static ssz_error_t copy_root(const ssz_chunk_t *src, ssz_chunk_t *out)
{
    if ((src == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    memcpy(out, src, sizeof(ssz_chunk_t));
    return SSZ_SUCCESS;
}

static ssz_error_t parse_u64_node(const yaml_node_t *node, uint64_t *out)
{
    if ((node == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    return yaml_node_parse_u64(node, out) ? SSZ_SUCCESS : SSZ_ERR_TYPE_MISMATCH;
}

static ssz_error_t compute_uint_basic(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    uint8_t *bytes = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((type == NULL) || (node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (type->kind == TYPE_UINT8)
    {
        uint64_t parsed = 0u;
        if (parse_u64_node(node, &parsed) != SSZ_SUCCESS || parsed > UINT8_MAX)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        err = copy_bytes((const uint8_t[]){0u}, 1u, &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint8((uint8_t)parsed, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint8((uint8_t)parsed, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        out_value->bytes = bytes;
        out_value->len = 1u;
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_UINT16)
    {
        uint64_t parsed = 0u;
        if (parse_u64_node(node, &parsed) != SSZ_SUCCESS || parsed > UINT16_MAX)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        err = copy_bytes((const uint8_t[]){0u, 0u}, 2u, &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint16((uint16_t)parsed, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint16((uint16_t)parsed, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        out_value->bytes = bytes;
        out_value->len = 2u;
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_UINT32)
    {
        uint64_t parsed = 0u;
        if (parse_u64_node(node, &parsed) != SSZ_SUCCESS || parsed > UINT32_MAX)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        err = copy_bytes((const uint8_t[]){0u, 0u, 0u, 0u}, 4u, &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint32((uint32_t)parsed, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint32((uint32_t)parsed, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        out_value->bytes = bytes;
        out_value->len = 4u;
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_UINT64)
    {
        uint64_t parsed = 0u;
        if (parse_u64_node(node, &parsed) != SSZ_SUCCESS)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        err = copy_bytes((const uint8_t[]){0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, 8u, &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint64(parsed, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint64(parsed, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        out_value->bytes = bytes;
        out_value->len = 8u;
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_UINT128)
    {
        uint8_t decoded[16];
        if (!yaml_node_parse_decimal_le(node, decoded, sizeof(decoded)))
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }

        err = copy_bytes(decoded, sizeof(decoded), &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint128(decoded, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint128(decoded, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }

        out_value->bytes = bytes;
        out_value->len = sizeof(decoded);
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_UINT256)
    {
        uint8_t decoded[32];
        if (!yaml_node_parse_decimal_le(node, decoded, sizeof(decoded)))
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }

        err = copy_bytes(decoded, sizeof(decoded), &bytes);
        if (err != SSZ_SUCCESS)
        {
            return err;
        }
        err = ssz_serialize_uint256(decoded, bytes);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }
        err = ssz_hash_tree_root_uint256(decoded, &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(bytes);
            return err;
        }

        out_value->bytes = bytes;
        out_value->len = sizeof(decoded);
        return SSZ_SUCCESS;
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

static ssz_error_t compute_bitvector_like(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    uint8_t *encoded = NULL;
    size_t encoded_len = 0u;
    size_t bitfield_len = 0u;
    uint8_t *bits = NULL;
    size_t out_len = 0u;
    uint8_t *out_bytes = NULL;
    uint64_t bit_len = 0u;
    ssz_error_t err;

    if ((type == NULL) || (node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!yaml_node_parse_hex(node, &encoded, &encoded_len))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    if (type->kind == TYPE_BITVECTOR)
    {
        if (!bits_to_bytes(type->param, &bitfield_len))
        {
            free(encoded);
            return SSZ_ERR_OVERFLOW;
        }

        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            free(encoded);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_bitvector(encoded, encoded_len, type->param, bits, bitfield_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        err = ssz_serialize_bitvector(bits, bitfield_len, type->param, NULL, 0u, &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        out_bytes = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (out_bytes == NULL)
        {
            free(encoded);
            free(bits);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_serialize_bitvector(bits, bitfield_len, type->param, out_bytes, out_len, &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        err = ssz_hash_tree_root_bitvector(bits, bitfield_len, type->param, ssz_hash_default(),
            &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        out_value->bytes = out_bytes;
        out_value->len = out_len;

        free(encoded);
        free(bits);
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_BITLIST)
    {
        if (!bits_to_bytes(type->param, &bitfield_len))
        {
            free(encoded);
            return SSZ_ERR_OVERFLOW;
        }

        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            free(encoded);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_bitlist(encoded, encoded_len, type->param, bits, bitfield_len, &bit_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        err = ssz_serialize_bitlist(bits, bitfield_len, bit_len, type->param, NULL, 0u, &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        out_bytes = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (out_bytes == NULL)
        {
            free(encoded);
            free(bits);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_serialize_bitlist(
            bits,
            bitfield_len,
            bit_len,
            type->param,
            out_bytes,
            out_len,
            &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        err = ssz_hash_tree_root_bitlist(bits, bitfield_len, bit_len, type->param, ssz_hash_default(),
            &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        out_value->bytes = out_bytes;
        out_value->len = out_len;

        free(encoded);
        free(bits);
        return SSZ_SUCCESS;
    }

    if (type->kind == TYPE_PROGRESSIVE_BITLIST)
    {
        bitfield_len = encoded_len == 0u ? 1u : encoded_len;
        bits = (uint8_t *)malloc(bitfield_len);
        if (bits == NULL)
        {
            free(encoded);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_progressive_bitlist(encoded, encoded_len, bits, bitfield_len, &bit_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        err = ssz_serialize_progressive_bitlist(bits, bitfield_len, bit_len, NULL, 0u, &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            return err;
        }

        out_bytes = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (out_bytes == NULL)
        {
            free(encoded);
            free(bits);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_serialize_progressive_bitlist(bits, bitfield_len, bit_len, out_bytes, out_len, &out_len);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        err = ssz_hash_tree_root_progressive_bitlist(bits, bitfield_len, bit_len, ssz_hash_default(),
            &out_value->root);
        if (err != SSZ_SUCCESS)
        {
            free(encoded);
            free(bits);
            free(out_bytes);
            return err;
        }

        out_value->bytes = out_bytes;
        out_value->len = out_len;

        free(encoded);
        free(bits);
        return SSZ_SUCCESS;
    }

    free(encoded);
    return SSZ_ERR_SCHEMA_INVALID;
}

static ssz_error_t compute_byte_list(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    uint8_t *raw = NULL;
    size_t raw_len = 0u;
    size_t out_len = 0u;
    uint8_t *out_bytes = NULL;
    ssz_error_t err;

    if ((type == NULL) || (node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!yaml_node_parse_hex(node, &raw, &raw_len))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    err = ssz_serialize_list_fixed(raw, raw_len, type->param, 1u, NULL, 0u, &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(raw);
        return err;
    }

    out_bytes = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
    if (out_bytes == NULL)
    {
        free(raw);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_serialize_list_fixed(raw, raw_len, type->param, 1u, out_bytes, out_len, &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(raw);
        free(out_bytes);
        return err;
    }

    err = ssz_hash_tree_root_list_fixed(raw, raw_len, type->param, 1u, ssz_hash_default(), &out_value->root);
    if (err != SSZ_SUCCESS)
    {
        free(raw);
        free(out_bytes);
        return err;
    }

    out_value->bytes = out_bytes;
    out_value->len = out_len;

    free(raw);
    return SSZ_SUCCESS;
}

typedef struct
{
    const computed_value_t *values;
    size_t count;
} computed_array_ctx_t;

static ssz_error_t computed_write_cb(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const computed_array_ctx_t *array_ctx = (const computed_array_ctx_t *)ctx;
    const computed_value_t *value;

    if ((array_ctx == NULL) || (out_written == NULL) || (member_id >= array_ctx->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    value = &array_ctx->values[member_id];
    *out_written = value->len;

    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }

    if (out_cap < value->len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (value->len != 0u)
    {
        memcpy(out, value->bytes, value->len);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t computed_root_cb(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const computed_array_ctx_t *array_ctx = (const computed_array_ctx_t *)ctx;

    if ((array_ctx == NULL) || (out_root == NULL) || (member_id >= array_ctx->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return copy_root(&array_ctx->values[member_id].root, out_root);
}

static void free_computed_array(computed_value_t *values, size_t count)
{
    if (values == NULL)
    {
        return;
    }

    for (size_t i = 0u; i < count; i++)
    {
        computed_value_reset(&values[i]);
    }
    free(values);
}

static ssz_error_t compute_children(
    const value_type_t *elem_type,
    const yaml_node_t *sequence,
    computed_value_t **out_values,
    size_t *out_count)
{
    computed_value_t *values;
    size_t count;

    if ((elem_type == NULL) || (sequence == NULL) || (out_values == NULL) || (out_count == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (sequence->type != YAML_NODE_SEQUENCE)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    count = sequence->as.sequence.count;
    values = (computed_value_t *)calloc(count == 0u ? 1u : count, sizeof(computed_value_t));
    if (values == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < count; i++)
    {
        ssz_error_t err = compute_value_from_yaml(elem_type, sequence->as.sequence.items[i], &values[i]);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(values, count);
            return err;
        }
    }

    *out_values = values;
    *out_count = count;
    return SSZ_SUCCESS;
}

static ssz_error_t concat_fixed_children(
    const computed_value_t *children,
    size_t count,
    size_t elem_size,
    uint8_t **out_bytes,
    size_t *out_len)
{
    uint8_t *flat;
    size_t total_len = 0u;

    if ((children == NULL && count != 0u) || (out_bytes == NULL) || (out_len == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (mul_overflow_size(count, elem_size, &total_len))
    {
        return SSZ_ERR_OVERFLOW;
    }

    flat = (uint8_t *)malloc(total_len == 0u ? 1u : total_len);
    if (flat == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < count; i++)
    {
        if (children[i].len != elem_size)
        {
            free(flat);
            return SSZ_ERR_TYPE_MISMATCH;
        }
        if (elem_size != 0u)
        {
            memcpy(flat + i * elem_size, children[i].bytes, elem_size);
        }
    }

    *out_bytes = flat;
    *out_len = total_len;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_vector(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    computed_value_t *children = NULL;
    size_t child_count = 0u;
    bool elem_fixed;
    size_t elem_size = 0u;
    bool elem_basic;
    uint8_t *flat = NULL;
    size_t flat_len = 0u;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    ssz_error_t err;
    computed_array_ctx_t ctx;
    ssz_member_codec_t codec;

    if ((type == NULL) || (node == NULL) || (out_value == NULL) || (type->kind != TYPE_VECTOR))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if ((node->type != YAML_NODE_SEQUENCE) || (node->as.sequence.count != (size_t)type->param))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    err = compute_children(type->elem_type, node, &children, &child_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    elem_fixed = value_type_is_fixed(type->elem_type, &elem_size);
    elem_basic = value_type_is_basic(type->elem_type);

    if (elem_fixed)
    {
        err = concat_fixed_children(children, child_count, elem_size, &flat, &flat_len);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(children, child_count);
            return err;
        }

        err = ssz_serialize_vector_fixed(flat, type->param, elem_size, NULL, 0u, &serialized_len);
        if (err != SSZ_SUCCESS)
        {
            free(flat);
            free_computed_array(children, child_count);
            return err;
        }

        serialized = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
        if (serialized == NULL)
        {
            free(flat);
            free_computed_array(children, child_count);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_serialize_vector_fixed(
            flat,
            type->param,
            elem_size,
            serialized,
            serialized_len,
            &serialized_len);
        if (err != SSZ_SUCCESS)
        {
            free(flat);
            free(serialized);
            free_computed_array(children, child_count);
            return err;
        }
    }
    else
    {
        ctx.values = children;
        ctx.count = child_count;

        codec.ctx = &ctx;
        codec.write = computed_write_cb;
        codec.read = NULL;
        codec.root = computed_root_cb;

        err = ssz_serialize_vector_variable(type->param, &codec, NULL, 0u, &serialized_len);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(children, child_count);
            return err;
        }

        serialized = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
        if (serialized == NULL)
        {
            free_computed_array(children, child_count);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_serialize_vector_variable(type->param, &codec, serialized, serialized_len,
            &serialized_len);
        if (err != SSZ_SUCCESS)
        {
            free(serialized);
            free_computed_array(children, child_count);
            return err;
        }
    }

    ctx.values = children;
    ctx.count = child_count;
    codec.ctx = &ctx;
    codec.write = computed_write_cb;
    codec.read = NULL;
    codec.root = computed_root_cb;

    if (elem_basic && elem_fixed)
    {
        err = ssz_hash_tree_root_vector_fixed(flat, type->param, elem_size, ssz_hash_default(),
            &out_value->root);
    }
    else
    {
        err = ssz_hash_tree_root_vector_composite(type->param, &codec, ssz_hash_default(),
            &out_value->root);
    }

    free(flat);
    free_computed_array(children, child_count);

    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = serialized_len;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_list_like(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    computed_value_t *children = NULL;
    size_t child_count = 0u;
    bool elem_fixed;
    size_t elem_size = 0u;
    bool elem_basic;
    uint8_t *flat = NULL;
    size_t flat_len = 0u;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    uint64_t limit = (type->kind == TYPE_LIST) ? type->param : SSZ_NO_LIMIT;
    ssz_error_t err;
    computed_array_ctx_t ctx;
    ssz_member_codec_t codec;

    if ((type == NULL) || (node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if ((type->kind != TYPE_LIST) && (type->kind != TYPE_PROGRESSIVE_LIST))
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    err = compute_children(type->elem_type, node, &children, &child_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    elem_fixed = value_type_is_fixed(type->elem_type, &elem_size);
    elem_basic = value_type_is_basic(type->elem_type);

    if (elem_fixed)
    {
        err = concat_fixed_children(children, child_count, elem_size, &flat, &flat_len);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(children, child_count);
            return err;
        }

        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_serialize_progressive_list_fixed(
                flat,
                child_count,
                elem_size,
                NULL,
                0u,
                &serialized_len);
        }
        else
        {
            err = ssz_serialize_list_fixed(flat, child_count, limit, elem_size, NULL, 0u,
                &serialized_len);
        }
        if (err != SSZ_SUCCESS)
        {
            free(flat);
            free_computed_array(children, child_count);
            return err;
        }

        serialized = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
        if (serialized == NULL)
        {
            free(flat);
            free_computed_array(children, child_count);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_serialize_progressive_list_fixed(
                flat,
                child_count,
                elem_size,
                serialized,
                serialized_len,
                &serialized_len);
        }
        else
        {
            err = ssz_serialize_list_fixed(flat, child_count, limit, elem_size, serialized,
                serialized_len, &serialized_len);
        }
        if (err != SSZ_SUCCESS)
        {
            free(flat);
            free(serialized);
            free_computed_array(children, child_count);
            return err;
        }
    }
    else
    {
        ctx.values = children;
        ctx.count = child_count;
        codec.ctx = &ctx;
        codec.write = computed_write_cb;
        codec.read = NULL;
        codec.root = computed_root_cb;

        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_serialize_progressive_list_variable(child_count, &codec, NULL, 0u,
                &serialized_len);
        }
        else
        {
            err = ssz_serialize_list_variable(child_count, limit, &codec, NULL, 0u,
                &serialized_len);
        }
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(children, child_count);
            return err;
        }

        serialized = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
        if (serialized == NULL)
        {
            free_computed_array(children, child_count);
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_serialize_progressive_list_variable(child_count, &codec, serialized,
                serialized_len, &serialized_len);
        }
        else
        {
            err = ssz_serialize_list_variable(child_count, limit, &codec, serialized,
                serialized_len, &serialized_len);
        }
        if (err != SSZ_SUCCESS)
        {
            free(serialized);
            free_computed_array(children, child_count);
            return err;
        }
    }

    ctx.values = children;
    ctx.count = child_count;
    codec.ctx = &ctx;
    codec.write = computed_write_cb;
    codec.read = NULL;
    codec.root = computed_root_cb;

    if (elem_basic && elem_fixed)
    {
        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_hash_tree_root_progressive_list_fixed(flat, child_count, elem_size,
                ssz_hash_default(), &out_value->root);
        }
        else
        {
            err = ssz_hash_tree_root_list_fixed(flat, child_count, limit, elem_size,
                ssz_hash_default(), &out_value->root);
        }
    }
    else
    {
        if (type->kind == TYPE_PROGRESSIVE_LIST)
        {
            err = ssz_hash_tree_root_progressive_list_composite(child_count, &codec,
                ssz_hash_default(), &out_value->root);
        }
        else
        {
            err = ssz_hash_tree_root_list_composite(child_count, limit, &codec,
                ssz_hash_default(), &out_value->root);
        }
    }

    free(flat);
    free_computed_array(children, child_count);

    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = serialized_len;
    return SSZ_SUCCESS;
}

static bool build_container_fixed_sizes(const container_def_t *container, size_t *fixed_sizes)
{
    if ((container == NULL) || (fixed_sizes == NULL))
    {
        return false;
    }

    for (uint32_t i = 0u; i < container->field_count; i++)
    {
        size_t fixed = 0u;
        if (value_type_is_fixed(container->fields[i].type, &fixed))
        {
            fixed_sizes[i] = fixed;
        }
        else
        {
            fixed_sizes[i] = 0u;
        }
    }

    return true;
}

static ssz_error_t build_progressive_active_fields(
    const container_def_t *container,
    const yaml_node_t *node,
    uint8_t out_active_fields[32],
    size_t *out_active_fields_len)
{
    size_t max_index = 0u;
    bool have_field = false;

    if ((container == NULL) || (container->fields == NULL) || (node == NULL) ||
        (out_active_fields == NULL) || (out_active_fields_len == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (node->type != YAML_NODE_MAPPING)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    memset(out_active_fields, 0, 32u);

    for (uint32_t i = 0u; i < container->field_count; i++)
    {
        const yaml_node_t *field_node = yaml_mapping_get(node, container->fields[i].name);
        uint8_t bit_index;

        if (field_node == NULL)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }

        bit_index = container->fields[i].active_index;
        out_active_fields[bit_index / 8u] |= (uint8_t)(1u << (bit_index % 8u));
        if (!have_field || ((size_t)bit_index > max_index))
        {
            max_index = (size_t)bit_index;
        }
        have_field = true;
    }

    if (!have_field)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    *out_active_fields_len = (max_index / 8u) + 1u;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_progressive_container_root(
    const container_def_t *container,
    const computed_value_t *fields,
    const uint8_t *active_fields,
    size_t active_fields_len,
    ssz_chunk_t *out_root)
{
    size_t slot_count = 0u;
    ssz_chunk_t *roots = NULL;
    ssz_chunk_t data_root;
    ssz_error_t err;

    if ((container == NULL) || (fields == NULL) || (out_root == NULL) || (active_fields == NULL) ||
        (active_fields_len == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < container->field_count; i++)
    {
        size_t field_slot = (size_t)container->fields[i].active_index + 1u;
        if (field_slot > slot_count)
        {
            slot_count = field_slot;
        }
    }
    if (slot_count == 0u)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    roots = (ssz_chunk_t *)calloc(slot_count, sizeof(ssz_chunk_t));
    if (roots == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < container->field_count; i++)
    {
        size_t slot = (size_t)container->fields[i].active_index;
        roots[slot] = fields[i].root;
    }

    err = ssz_merkleize_progressive(roots, slot_count, ssz_hash_default(), &data_root);
    free(roots);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ssz_mix_in_active_fields(&data_root, active_fields, active_fields_len, ssz_hash_default(),
        out_root);
}

static ssz_error_t compute_container(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    const container_def_t *container;
    computed_value_t *fields = NULL;
    size_t *fixed_sizes = NULL;
    size_t serialized_len = 0u;
    uint8_t *serialized = NULL;
    computed_array_ctx_t ctx;
    ssz_member_codec_t codec;
    uint8_t active_fields[32];
    size_t active_fields_len = 0u;
    ssz_error_t err;

    if ((type == NULL) || (node == NULL) || (out_value == NULL) || (type->kind != TYPE_CONTAINER) ||
        (type->container == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (node->type != YAML_NODE_MAPPING)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    container = type->container;

    fields = (computed_value_t *)calloc(container->field_count, sizeof(computed_value_t));
    fixed_sizes = (size_t *)calloc(container->field_count, sizeof(size_t));
    if ((fields == NULL) || (fixed_sizes == NULL))
    {
        free(fields);
        free(fixed_sizes);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < container->field_count; i++)
    {
        const yaml_node_t *field_node = yaml_mapping_get(node, container->fields[i].name);
        if (field_node == NULL)
        {
            free_computed_array(fields, container->field_count);
            free(fixed_sizes);
            return SSZ_ERR_TYPE_MISMATCH;
        }

        err = compute_value_from_yaml(container->fields[i].type, field_node, &fields[i]);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(fields, container->field_count);
            free(fixed_sizes);
            return err;
        }
    }

    if (!build_container_fixed_sizes(container, fixed_sizes))
    {
        free_computed_array(fields, container->field_count);
        free(fixed_sizes);
        return SSZ_ERR_SCHEMA_INVALID;
    }

    ctx.values = fields;
    ctx.count = container->field_count;

    codec.ctx = &ctx;
    codec.write = computed_write_cb;
    codec.read = NULL;
    codec.root = computed_root_cb;

    if (container->is_progressive)
    {
        err = build_progressive_active_fields(container, node, active_fields, &active_fields_len);
        if (err != SSZ_SUCCESS)
        {
            free_computed_array(fields, container->field_count);
            free(fixed_sizes);
            return err;
        }

        err = ssz_serialize_progressive_container(
            fixed_sizes,
            container->field_count,
            &codec,
            NULL,
            0u,
            &serialized_len);
    }
    else
    {
        err = ssz_serialize_container(
            fixed_sizes,
            container->field_count,
            &codec,
            NULL,
            0u,
            &serialized_len);
    }
    if (err != SSZ_SUCCESS)
    {
        free_computed_array(fields, container->field_count);
        free(fixed_sizes);
        return err;
    }

    serialized = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
    if (serialized == NULL)
    {
        free_computed_array(fields, container->field_count);
        free(fixed_sizes);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (container->is_progressive)
    {
        err = ssz_serialize_progressive_container(
            fixed_sizes,
            container->field_count,
            &codec,
            serialized,
            serialized_len,
            &serialized_len);
    }
    else
    {
        err = ssz_serialize_container(
            fixed_sizes,
            container->field_count,
            &codec,
            serialized,
            serialized_len,
            &serialized_len);
    }
    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        free_computed_array(fields, container->field_count);
        free(fixed_sizes);
        return err;
    }

    if (container->is_progressive)
    {
        err = compute_progressive_container_root(container, fields, active_fields, active_fields_len,
            &out_value->root);
    }
    else
    {
        err = ssz_hash_tree_root_vector_composite(
            container->field_count,
            &codec,
            ssz_hash_default(),
            &out_value->root);
    }

    free_computed_array(fields, container->field_count);
    free(fixed_sizes);

    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = serialized_len;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_value_from_yaml(
    const value_type_t *type,
    const yaml_node_t *node,
    computed_value_t *out_value)
{
    if ((type == NULL) || (node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_value, 0, sizeof(*out_value));

    if ((type->kind == TYPE_UINT8) || (type->kind == TYPE_UINT16) || (type->kind == TYPE_UINT32) ||
        (type->kind == TYPE_UINT64) || (type->kind == TYPE_UINT128) ||
        (type->kind == TYPE_UINT256))
    {
        return compute_uint_basic(type, node, out_value);
    }

    if ((type->kind == TYPE_BITVECTOR) || (type->kind == TYPE_BITLIST) ||
        (type->kind == TYPE_PROGRESSIVE_BITLIST))
    {
        return compute_bitvector_like(type, node, out_value);
    }

    if (type->kind == TYPE_BYTE_LIST)
    {
        return compute_byte_list(type, node, out_value);
    }

    if (type->kind == TYPE_VECTOR)
    {
        return compute_vector(type, node, out_value);
    }

    if ((type->kind == TYPE_LIST) || (type->kind == TYPE_PROGRESSIVE_LIST))
    {
        return compute_list_like(type, node, out_value);
    }

    if (type->kind == TYPE_CONTAINER)
    {
        return compute_container(type, node, out_value);
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

typedef struct
{
    const value_type_t *elem_type;
} validate_elem_ctx_t;

static ssz_error_t validate_read_elem(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    validate_elem_ctx_t *elem_ctx = (validate_elem_ctx_t *)ctx;
    (void)member_id;

    if ((elem_ctx == NULL) || (elem_ctx->elem_type == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return validate_value_bytes(elem_ctx->elem_type, data, data_len);
}

typedef struct
{
    const container_def_t *container;
} validate_container_ctx_t;

static ssz_error_t validate_read_field(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    validate_container_ctx_t *field_ctx = (validate_container_ctx_t *)ctx;

    if ((field_ctx == NULL) || (field_ctx->container == NULL) ||
        (member_id >= field_ctx->container->field_count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    return validate_value_bytes(field_ctx->container->fields[member_id].type, data, data_len);
}

static ssz_error_t validate_value_bytes(
    const value_type_t *type,
    const uint8_t *bytes,
    size_t len)
{
    if (type == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (type->kind == TYPE_UINT8)
    {
        uint8_t out;
        if (len != 1u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint8(bytes, &out);
    }

    if (type->kind == TYPE_UINT16)
    {
        uint16_t out;
        if (len != 2u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint16(bytes, &out);
    }

    if (type->kind == TYPE_UINT32)
    {
        uint32_t out;
        if (len != 4u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint32(bytes, &out);
    }

    if (type->kind == TYPE_UINT64)
    {
        uint64_t out;
        if (len != 8u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint64(bytes, &out);
    }

    if (type->kind == TYPE_UINT128)
    {
        uint8_t out[16];
        if (len != sizeof(out))
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint128(bytes, out);
    }

    if (type->kind == TYPE_UINT256)
    {
        uint8_t out[32];
        if (len != sizeof(out))
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        return ssz_deserialize_uint256(bytes, out);
    }

    if (type->kind == TYPE_BITVECTOR)
    {
        size_t bitfield_len = 0u;
        uint8_t *bits;
        ssz_error_t err;

        if (!bits_to_bytes(type->param, &bitfield_len))
        {
            return SSZ_ERR_OVERFLOW;
        }

        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_bitvector(bytes, len, type->param, bits, bitfield_len);
        free(bits);
        return err;
    }

    if (type->kind == TYPE_BITLIST)
    {
        size_t bitfield_len = 0u;
        uint8_t *bits;
        uint64_t out_len;
        ssz_error_t err;

        if (!bits_to_bytes(type->param, &bitfield_len))
        {
            return SSZ_ERR_OVERFLOW;
        }

        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_bitlist(bytes, len, type->param, bits, bitfield_len, &out_len);
        free(bits);
        return err;
    }

    if (type->kind == TYPE_PROGRESSIVE_BITLIST)
    {
        size_t bitfield_len = len == 0u ? 1u : len;
        uint8_t *bits = (uint8_t *)malloc(bitfield_len);
        uint64_t out_len;
        ssz_error_t err;

        if (bits == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_progressive_bitlist(bytes, len, bits, bitfield_len, &out_len);
        free(bits);
        return err;
    }

    if (type->kind == TYPE_BYTE_LIST)
    {
        uint8_t *copy = (uint8_t *)malloc(len == 0u ? 1u : len);
        uint64_t count = 0u;
        ssz_error_t err;

        if (copy == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        err = ssz_deserialize_list_fixed(bytes, len, type->param, 1u, copy, len, &count);
        free(copy);
        return err;
    }

    if (type->kind == TYPE_VECTOR)
    {
        bool elem_fixed;
        size_t elem_size = 0u;

        elem_fixed = value_type_is_fixed(type->elem_type, &elem_size);
        if (elem_fixed)
        {
            size_t expected_len = 0u;
            if (type->param > (uint64_t)SIZE_MAX || mul_overflow_size((size_t)type->param, elem_size,
                    &expected_len))
            {
                return SSZ_ERR_OVERFLOW;
            }
            if (len != expected_len)
            {
                return SSZ_ERR_ENCODING_INVALID;
            }
            for (size_t i = 0u; i < (size_t)type->param; i++)
            {
                ssz_error_t err = validate_value_bytes(type->elem_type, bytes + i * elem_size, elem_size);
                if (err != SSZ_SUCCESS)
                {
                    return err;
                }
            }
            return SSZ_SUCCESS;
        }
        else
        {
            validate_elem_ctx_t ctx = { .elem_type = type->elem_type };
            ssz_member_codec_t codec = {
                .ctx = &ctx,
                .write = NULL,
                .read = validate_read_elem,
                .root = NULL,
            };
            size_t min_size = 0u;
            if (!value_type_min_size(type->elem_type, &min_size))
            {
                return SSZ_ERR_SCHEMA_INVALID;
            }
            return ssz_deserialize_vector_variable(bytes, len, type->param, min_size, &codec);
        }
    }

    if ((type->kind == TYPE_LIST) || (type->kind == TYPE_PROGRESSIVE_LIST))
    {
        bool elem_fixed;
        size_t elem_size = 0u;
        uint64_t limit = (type->kind == TYPE_LIST) ? type->param : SSZ_NO_LIMIT;

        elem_fixed = value_type_is_fixed(type->elem_type, &elem_size);
        if (elem_fixed)
        {
            uint8_t *copy = (uint8_t *)malloc(len == 0u ? 1u : len);
            uint64_t count = 0u;
            ssz_error_t err;

            if (copy == NULL)
            {
                return SSZ_ERR_INVALID_ARGUMENT;
            }

            if (type->kind == TYPE_PROGRESSIVE_LIST)
            {
                err = ssz_deserialize_progressive_list_fixed(bytes, len, elem_size, copy, len, &count);
            }
            else
            {
                err = ssz_deserialize_list_fixed(bytes, len, limit, elem_size, copy, len, &count);
            }
            if (err == SSZ_SUCCESS)
            {
                for (size_t i = 0u; i < (size_t)count; i++)
                {
                    err = validate_value_bytes(type->elem_type, copy + i * elem_size, elem_size);
                    if (err != SSZ_SUCCESS)
                    {
                        break;
                    }
                }
            }

            free(copy);
            return err;
        }
        else
        {
            validate_elem_ctx_t ctx = { .elem_type = type->elem_type };
            ssz_member_codec_t codec = {
                .ctx = &ctx,
                .write = NULL,
                .read = validate_read_elem,
                .root = NULL,
            };
            uint64_t count = 0u;
            size_t min_size = 0u;

            if (!value_type_min_size(type->elem_type, &min_size))
            {
                return SSZ_ERR_SCHEMA_INVALID;
            }

            if (type->kind == TYPE_PROGRESSIVE_LIST)
            {
                return ssz_deserialize_progressive_list_variable(bytes, len, min_size, &codec, &count);
            }
            else
            {
                return ssz_deserialize_list_variable(bytes, len, limit, min_size, &codec, &count);
            }
        }
    }

    if (type->kind == TYPE_CONTAINER)
    {
        validate_container_ctx_t ctx = { .container = type->container };
        ssz_member_codec_t codec = {
            .ctx = &ctx,
            .write = NULL,
            .read = validate_read_field,
            .root = NULL,
        };
        size_t *fixed_sizes;
        ssz_error_t err;

        if ((type->container == NULL) || (type->container->field_count == 0u))
        {
            return SSZ_ERR_SCHEMA_INVALID;
        }

        fixed_sizes = (size_t *)calloc(type->container->field_count, sizeof(size_t));
        if (fixed_sizes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }

        if (!build_container_fixed_sizes(type->container, fixed_sizes))
        {
            free(fixed_sizes);
            return SSZ_ERR_SCHEMA_INVALID;
        }

        if (type->container->is_progressive)
        {
            err = ssz_deserialize_progressive_container(
                bytes,
                len,
                fixed_sizes,
                type->container->field_count,
                &codec);
        }
        else
        {
            err = ssz_deserialize_container(bytes, len, fixed_sizes, type->container->field_count, &codec);
        }
        free(fixed_sizes);
        return err;
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

static bool parse_case_type_name(const char *case_name, char *out_name, size_t out_cap)
{
    size_t i = 0u;

    if ((case_name == NULL) || (out_name == NULL) || (out_cap < 2u))
    {
        return false;
    }

    while ((case_name[i] != '\0') && (case_name[i] != '_'))
    {
        if (i + 1u >= out_cap)
        {
            return false;
        }
        out_name[i] = case_name[i];
        i++;
    }

    if (i == 0u)
    {
        return false;
    }

    out_name[i] = '\0';
    return true;
}

static const value_type_t TYPE_UINT8_T = { .kind = TYPE_UINT8 };
static const value_type_t TYPE_UINT16_T = { .kind = TYPE_UINT16 };
static const value_type_t TYPE_UINT64_T = { .kind = TYPE_UINT64 };

static const value_type_t TYPE_LIST_UINT16_1024 = {
    .kind = TYPE_LIST,
    .elem_type = &TYPE_UINT16_T,
    .param = 1024u,
};

static const value_type_t TYPE_LIST_UINT16_123 = {
    .kind = TYPE_LIST,
    .elem_type = &TYPE_UINT16_T,
    .param = 123u,
};

static const value_type_t TYPE_PROGRESSIVE_BITLIST_T = {
    .kind = TYPE_PROGRESSIVE_BITLIST,
};

static const container_def_t CONTAINER_SMALL;
static const value_type_t TYPE_CONTAINER_SMALL;
static const container_def_t CONTAINER_VAR;
static const value_type_t TYPE_CONTAINER_VAR;
static const container_def_t CONTAINER_PROGRESSIVE_SINGLE_FIELD;
static const value_type_t TYPE_CONTAINER_PROGRESSIVE_SINGLE_FIELD;
static const container_def_t CONTAINER_PROGRESSIVE_SINGLE_LIST;
static const value_type_t TYPE_CONTAINER_PROGRESSIVE_SINGLE_LIST;
static const container_def_t CONTAINER_PROGRESSIVE_VAR;
static const value_type_t TYPE_CONTAINER_PROGRESSIVE_VAR;
static const container_def_t CONTAINER_PROGRESSIVE_COMPLEX;
static const value_type_t TYPE_CONTAINER_PROGRESSIVE_COMPLEX;

static const value_type_t TYPE_PROGRESSIVE_LIST_U64 = {
    .kind = TYPE_PROGRESSIVE_LIST,
    .elem_type = &TYPE_UINT64_T,
    .param = SSZ_NO_LIMIT,
};

static const value_type_t TYPE_PROGRESSIVE_LIST_SMALL = {
    .kind = TYPE_PROGRESSIVE_LIST,
    .elem_type = &TYPE_CONTAINER_SMALL,
    .param = SSZ_NO_LIMIT,
};

static const value_type_t TYPE_PROGRESSIVE_LIST_VAR = {
    .kind = TYPE_PROGRESSIVE_LIST,
    .elem_type = &TYPE_CONTAINER_VAR,
    .param = SSZ_NO_LIMIT,
};

static const value_type_t TYPE_PROGRESSIVE_LIST_PROGRESSIVE_VAR = {
    .kind = TYPE_PROGRESSIVE_LIST,
    .elem_type = &TYPE_PROGRESSIVE_LIST_VAR,
    .param = SSZ_NO_LIMIT,
};

static const value_type_t TYPE_LIST_PROGRESSIVE_SINGLE_FIELD_10 = {
    .kind = TYPE_LIST,
    .elem_type = &TYPE_CONTAINER_PROGRESSIVE_SINGLE_FIELD,
    .param = 10u,
};

static const value_type_t TYPE_PROGRESSIVE_LIST_PROGRESSIVE_VAR_CONTAINER = {
    .kind = TYPE_PROGRESSIVE_LIST,
    .elem_type = &TYPE_CONTAINER_PROGRESSIVE_VAR,
    .param = SSZ_NO_LIMIT,
};

static const field_def_t FIELDS_SMALL[] = {
    {"A", &TYPE_UINT16_T, 0u},
    {"B", &TYPE_UINT16_T, 1u},
};

static const field_def_t FIELDS_VAR[] = {
    {"A", &TYPE_UINT16_T, 0u},
    {"B", &TYPE_LIST_UINT16_1024, 1u},
    {"C", &TYPE_UINT8_T, 2u},
};

static const field_def_t FIELDS_PROGRESSIVE_SINGLE_FIELD[] = {
    {"A", &TYPE_UINT8_T, 0u},
};

static const field_def_t FIELDS_PROGRESSIVE_SINGLE_LIST[] = {
    {"C", &TYPE_PROGRESSIVE_BITLIST_T, 4u},
};

static const field_def_t FIELDS_PROGRESSIVE_VAR[] = {
    {"A", &TYPE_UINT8_T, 0u},
    {"B", &TYPE_LIST_UINT16_123, 2u},
    {"C", &TYPE_PROGRESSIVE_BITLIST_T, 4u},
};

static const field_def_t FIELDS_PROGRESSIVE_COMPLEX[] = {
    {"A", &TYPE_UINT8_T, 0u},
    {"B", &TYPE_LIST_UINT16_123, 2u},
    {"C", &TYPE_PROGRESSIVE_BITLIST_T, 4u},
    {"D", &TYPE_PROGRESSIVE_LIST_U64, 8u},
    {"E", &TYPE_PROGRESSIVE_LIST_SMALL, 12u},
    {"F", &TYPE_PROGRESSIVE_LIST_PROGRESSIVE_VAR, 13u},
    {"G", &TYPE_LIST_PROGRESSIVE_SINGLE_FIELD_10, 20u},
    {"H", &TYPE_PROGRESSIVE_LIST_PROGRESSIVE_VAR_CONTAINER, 21u},
};

static const container_def_t CONTAINER_SMALL = {
    .name = "SmallTestStruct",
    .fields = FIELDS_SMALL,
    .field_count = 2u,
    .is_progressive = false,
};

static const container_def_t CONTAINER_VAR = {
    .name = "VarTestStruct",
    .fields = FIELDS_VAR,
    .field_count = 3u,
    .is_progressive = false,
};

static const container_def_t CONTAINER_PROGRESSIVE_SINGLE_FIELD = {
    .name = "ProgressiveSingleFieldContainerTestStruct",
    .fields = FIELDS_PROGRESSIVE_SINGLE_FIELD,
    .field_count = 1u,
    .is_progressive = true,
};

static const container_def_t CONTAINER_PROGRESSIVE_SINGLE_LIST = {
    .name = "ProgressiveSingleListContainerTestStruct",
    .fields = FIELDS_PROGRESSIVE_SINGLE_LIST,
    .field_count = 1u,
    .is_progressive = true,
};

static const container_def_t CONTAINER_PROGRESSIVE_VAR = {
    .name = "ProgressiveVarTestStruct",
    .fields = FIELDS_PROGRESSIVE_VAR,
    .field_count = 3u,
    .is_progressive = true,
};

static const container_def_t CONTAINER_PROGRESSIVE_COMPLEX = {
    .name = "ProgressiveComplexTestStruct",
    .fields = FIELDS_PROGRESSIVE_COMPLEX,
    .field_count = 8u,
    .is_progressive = true,
};

static const value_type_t TYPE_CONTAINER_SMALL = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_SMALL,
};

static const value_type_t TYPE_CONTAINER_VAR = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_VAR,
};

static const value_type_t TYPE_CONTAINER_PROGRESSIVE_SINGLE_FIELD = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_PROGRESSIVE_SINGLE_FIELD,
};

static const value_type_t TYPE_CONTAINER_PROGRESSIVE_SINGLE_LIST = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_PROGRESSIVE_SINGLE_LIST,
};

static const value_type_t TYPE_CONTAINER_PROGRESSIVE_VAR = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_PROGRESSIVE_VAR,
};

static const value_type_t TYPE_CONTAINER_PROGRESSIVE_COMPLEX = {
    .kind = TYPE_CONTAINER,
    .container = &CONTAINER_PROGRESSIVE_COMPLEX,
};

static const value_type_t *container_type_from_name(const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }

    if (strcmp(name, CONTAINER_PROGRESSIVE_SINGLE_FIELD.name) == 0)
    {
        return &TYPE_CONTAINER_PROGRESSIVE_SINGLE_FIELD;
    }
    if (strcmp(name, CONTAINER_PROGRESSIVE_SINGLE_LIST.name) == 0)
    {
        return &TYPE_CONTAINER_PROGRESSIVE_SINGLE_LIST;
    }
    if (strcmp(name, CONTAINER_PROGRESSIVE_VAR.name) == 0)
    {
        return &TYPE_CONTAINER_PROGRESSIVE_VAR;
    }
    if (strcmp(name, CONTAINER_PROGRESSIVE_COMPLEX.name) == 0)
    {
        return &TYPE_CONTAINER_PROGRESSIVE_COMPLEX;
    }

    return NULL;
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char type_name[64];
    const value_type_t *container_type;
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    yaml_node_t *value_doc = NULL;
    computed_value_t computed = {0};
    uint8_t expected_root[32];

    report->total_valid++;

    if (!parse_case_type_name(case_name, type_name, sizeof(type_name)))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_name, "failed to parse container type from case name");
        return;
    }

    container_type = container_type_from_name(type_name);
    if (container_type == NULL)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_name, "unsupported container type in case name");
        return;
    }

    case_path = spec_join_path(suite_dir, case_name);
    serialized_path = spec_join_path(case_path, "serialized.ssz_snappy");
    value_path = spec_join_path(case_path, "value.yaml");
    meta_path = spec_join_path(case_path, "meta.yaml");

    if ((case_path == NULL) || (serialized_path == NULL) || (value_path == NULL) || (meta_path == NULL))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_name, "failed to allocate case paths");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    value_doc = yaml_parse_file(value_path);
    if (value_doc == NULL)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse value.yaml");
        goto done;
    }

    if (!spec_parse_root_from_meta(meta_path, expected_root))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse root from meta.yaml");
        goto done;
    }

    if (validate_value_bytes(container_type, serialized, serialized_len) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization validation failed");
        goto done;
    }

    if (compute_value_from_yaml(container_type, value_doc, &computed) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "failed to serialize/hash from value.yaml");
        goto done;
    }

    if ((computed.len != serialized_len) || (memcmp(computed.bytes, serialized, serialized_len) != 0))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "round-trip serialization mismatch");
        goto done;
    }

    if (memcmp(computed.root.bytes, expected_root, 32u) != 0)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "hash_tree_root mismatch vs meta.yaml");
        goto done;
    }

    report->valid_passed++;

done:
    free(case_path);
    free(serialized_path);
    free(value_path);
    free(meta_path);
    free(serialized);
    yaml_node_free(value_doc);
    computed_value_reset(&computed);
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char type_name[64];
    const value_type_t *container_type;
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    ssz_error_t err;

    report->total_invalid++;

    if (!parse_case_type_name(case_name, type_name, sizeof(type_name)))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_name, "failed to parse container type from case name");
        return;
    }

    container_type = container_type_from_name(type_name);
    if (container_type == NULL)
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_name, "unsupported container type in case name");
        return;
    }

    case_path = spec_join_path(suite_dir, case_name);
    serialized_path = spec_join_path(case_path, "serialized.ssz_snappy");

    if ((case_path == NULL) || (serialized_path == NULL))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_name, "failed to allocate case paths");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    err = validate_value_bytes(container_type, serialized, serialized_len);
    if (err == SSZ_SUCCESS)
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "invalid input unexpectedly deserialized");
        goto done;
    }

    report->invalid_passed++;

done:
    free(case_path);
    free(serialized_path);
    free(serialized);
}

int main(void)
{
    spec_report_t report;
    char *valid_dir;
    char *invalid_dir;
    char **cases = NULL;
    size_t case_count = 0u;

    spec_report_init(&report);

    valid_dir = spec_join_path(TESTS_DIR, "valid");
    invalid_dir = spec_join_path(TESTS_DIR, "invalid");

    if ((valid_dir == NULL) || (invalid_dir == NULL))
    {
        spec_report_record_failure(&report, TESTS_DIR, "failed to allocate suite paths");
        report.valid_failed++;
        goto done;
    }

    if (!spec_list_subdirs(valid_dir, &cases, &case_count))
    {
        spec_report_record_failure(&report, valid_dir, "failed to list valid cases");
        report.valid_failed++;
        goto done;
    }

    for (size_t i = 0u; i < case_count; i++)
    {
        run_valid_case(&report, valid_dir, cases[i]);
    }

    spec_free_string_array(cases, case_count);
    cases = NULL;
    case_count = 0u;

    if (!spec_list_subdirs(invalid_dir, &cases, &case_count))
    {
        spec_report_record_failure(&report, invalid_dir, "failed to list invalid cases");
        report.invalid_failed++;
        goto done;
    }

    for (size_t i = 0u; i < case_count; i++)
    {
        run_invalid_case(&report, invalid_dir, cases[i]);
    }

done:
    spec_free_string_array(cases, case_count);
    free(valid_dir);
    free(invalid_dir);

    {
        int rc = spec_report_print("ssz_generic/progressive_containers", &report);
        spec_report_free(&report);
        return rc;
    }
}
