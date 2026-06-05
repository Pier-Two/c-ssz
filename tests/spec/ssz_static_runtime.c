#include "spec/ssz_static_runtime.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"

typedef struct
{
    const ssz_static_schema_type_t *type;
    ssz_chunk_t *roots;
} ssz_static_container_ctx_t;

typedef struct
{
    const ssz_static_schema_type_t *elem_type;
    ssz_chunk_t *roots;
    size_t capacity;
    size_t count;
} ssz_static_sequence_ctx_t;

typedef struct
{
    const ssz_static_computed_value_t *values;
    size_t count;
} ssz_static_computed_array_ctx_t;

typedef struct
{
    const ssz_static_schema_type_t *type;
    ssz_static_computed_value_t *values;
} ssz_static_decode_container_ctx_t;

typedef struct
{
    const ssz_static_schema_type_t *elem_type;
    ssz_static_computed_value_t *values;
    size_t capacity;
    size_t count;
} ssz_static_decode_sequence_ctx_t;

static bool ssz_static_mul_overflow_size(size_t a, size_t b, size_t *out)
{
    bool overflow = false;

    if ((a != 0u) && (b > (SIZE_MAX / a)))
    {
        overflow = true;
    }
    else if (out != NULL)
    {
        *out = a * b;
    }
    else
    {
        /* Intentionally empty. */
    }

    return overflow;
}

static bool ssz_static_u64_to_size(uint64_t value, size_t *out)
{
    bool converted = false;

    if (value <= (uint64_t)SIZE_MAX)
    {
        if (out != NULL)
        {
            *out = (size_t)value;
        }
        converted = true;
    }

    return converted;
}

static bool ssz_static_bits_to_bytes(uint64_t bit_count, size_t *out_bytes)
{
    uint64_t bytes_u64 = bit_count / 8u;

    if ((bit_count % 8u) != 0u)
    {
        bytes_u64++;
    }

    return ssz_static_u64_to_size(bytes_u64, out_bytes);
}

static const ssz_static_schema_type_t *ssz_static_type_at(uint32_t type_index)
{
    const ssz_static_schema_type_t *type = NULL;

    if ((size_t)type_index < g_ssz_static_schema_type_count)
    {
        type = &g_ssz_static_schema_types[type_index];
    }

    return type;
}

static bool ssz_static_kind_is_basic(ssz_static_schema_kind_t kind)
{
    return (kind >= SSZ_STATIC_SCHEMA_KIND_BOOL) && (kind <= SSZ_STATIC_SCHEMA_KIND_UINT256);
}

static const uint8_t *ssz_static_input_bytes(const uint8_t *bytes, size_t byte_len)
{
    static const uint8_t zero_byte = 0u;
    const uint8_t *input = bytes;

    if ((input == NULL) && (byte_len == 0u))
    {
        input = &zero_byte;
    }

    return input;
}

static ssz_error_t ssz_static_root_from_small_bytes(
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_hash_tree_root_vector_fixed(input, (uint64_t)byte_len, 1u, ssz_hash_default(), out_root);
    }

    return err;
}

static ssz_chunk_t *ssz_static_alloc_roots(size_t count)
{
    size_t alloc_count = (count == 0u) ? 1u : count;
    return (ssz_chunk_t *)calloc(alloc_count, sizeof(ssz_chunk_t));
}

void ssz_static_computed_value_reset(ssz_static_computed_value_t *value)
{
    if (value != NULL)
    {
        value->type = NULL;
        free(value->bytes);
        value->bytes = NULL;
        value->len = 0u;
        (void)memset(&value->root, 0, sizeof(value->root));
    }
}

static void ssz_static_free_computed_array(ssz_static_computed_value_t *values, size_t count)
{
    if (values != NULL)
    {
        for (size_t i = 0u; i < count; i++)
        {
            ssz_static_computed_value_reset(&values[i]);
        }
        free(values);
    }
}

static ssz_error_t ssz_static_copy_root(const ssz_chunk_t *source, ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((source == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *out_root = *source;
    }

    return err;
}

static ssz_error_t ssz_static_computed_write_cb(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const ssz_static_computed_array_ctx_t *array_ctx = (const ssz_static_computed_array_ctx_t *)ctx;
    const ssz_static_computed_value_t *value = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((array_ctx == NULL) || (out_written == NULL) || (member_id >= array_ctx->count))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        value = &array_ctx->values[member_id];
        *out_written = value->len;

        if (out == NULL)
        {
            /* Intentionally empty. */
        }
        else if (out_cap < value->len)
        {
            err = SSZ_ERR_BUFFER_TOO_SMALL;
        }
        else if (value->len != 0u)
        {
            (void)memcpy(out, value->bytes, value->len);
        }
        else
        {
            /* Intentionally empty. */
        }
    }

    return err;
}

static ssz_error_t ssz_static_computed_root_cb(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const ssz_static_computed_array_ctx_t *array_ctx = (const ssz_static_computed_array_ctx_t *)ctx;
    ssz_error_t err = SSZ_SUCCESS;

    if ((array_ctx == NULL) || (out_root == NULL) || (member_id >= array_ctx->count))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = ssz_static_copy_root(&array_ctx->values[member_id].root, out_root);
    }

    return err;
}

static ssz_error_t ssz_static_concat_fixed_children(
    const ssz_static_computed_value_t *children,
    size_t count,
    size_t elem_size,
    uint8_t **out_bytes,
    size_t *out_len)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *flat = NULL;
    size_t total_len = 0u;

    if (((children == NULL) && (count != 0u)) || (out_bytes == NULL) || (out_len == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_static_mul_overflow_size(count, elem_size, &total_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        flat = (uint8_t *)malloc(total_len == 0u ? 1u : total_len);
        if (flat == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            for (size_t i = 0u; (i < count) && (err == SSZ_SUCCESS); i++)
            {
                if (children[i].len != elem_size)
                {
                    err = SSZ_ERR_TYPE_MISMATCH;
                }
                else if (elem_size != 0u)
                {
                    (void)memcpy(flat + (i * elem_size), children[i].bytes, elem_size);
                }
                else
                {
                    /* Intentionally empty. */
                }
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        *out_bytes = flat;
        *out_len = total_len;
    }
    else
    {
        free(flat);
    }

    return err;
}

static ssz_error_t ssz_static_validate_and_root_internal(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root);

static ssz_error_t ssz_static_compute_from_bytes_internal(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value);

static ssz_error_t ssz_static_capture_container_field(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    ssz_static_container_ctx_t *container_ctx = (ssz_static_container_ctx_t *)ctx;
    const ssz_static_schema_type_t *field_type = NULL;

    if ((container_ctx == NULL) || (container_ctx->type == NULL) || (container_ctx->roots == NULL) ||
        (member_id >= container_ctx->type->field_count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    field_type = ssz_static_type_at(
        g_ssz_static_schema_fields[container_ctx->type->field_index + member_id].child_type_index);
    if (field_type == NULL)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    return ssz_static_validate_and_root_internal(
        field_type,
        data,
        data_len,
        &container_ctx->roots[member_id]);
}

static ssz_error_t ssz_static_ensure_sequence_capacity(ssz_static_sequence_ctx_t *ctx, size_t required)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (ctx == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (required <= ctx->capacity)
    {
        /* Intentionally empty. */
    }
    else
    {
        size_t new_capacity = ctx->capacity == 0u ? 4u : ctx->capacity;

        while ((new_capacity < required) && (err == SSZ_SUCCESS))
        {
            if (new_capacity > (SIZE_MAX / 2u))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                new_capacity *= 2u;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            ssz_chunk_t *new_roots = (ssz_chunk_t *)realloc(ctx->roots, new_capacity * sizeof(*new_roots));
            if (new_roots == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                size_t added = new_capacity - ctx->capacity;
                (void)memset(&new_roots[ctx->capacity], 0, added * sizeof(*new_roots));
                ctx->roots = new_roots;
                ctx->capacity = new_capacity;
            }
        }
    }

    return err;
}

static ssz_error_t ssz_static_capture_sequence_elem(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    ssz_static_sequence_ctx_t *sequence_ctx = (ssz_static_sequence_ctx_t *)ctx;
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((sequence_ctx == NULL) || (sequence_ctx->elem_type == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_u64_to_size(member_id + 1u, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_static_ensure_sequence_capacity(sequence_ctx, required);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_static_validate_and_root_internal(
                sequence_ctx->elem_type,
                data,
                data_len,
                &sequence_ctx->roots[member_id]);
            if ((err == SSZ_SUCCESS) && (required > sequence_ctx->count))
            {
                sequence_ctx->count = required;
            }
        }
    }

    return err;
}

static ssz_error_t ssz_static_decode_sequence_ensure_capacity(
    ssz_static_decode_sequence_ctx_t *ctx,
    size_t required)
{
    ssz_error_t err = SSZ_SUCCESS;

    if (ctx == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (required <= ctx->capacity)
    {
        /* Intentionally empty. */
    }
    else
    {
        size_t new_capacity = ctx->capacity == 0u ? 4u : ctx->capacity;

        while ((new_capacity < required) && (err == SSZ_SUCCESS))
        {
            if (new_capacity > (SIZE_MAX / 2u))
            {
                err = SSZ_ERR_OVERFLOW;
            }
            else
            {
                new_capacity *= 2u;
            }
        }

        if (err == SSZ_SUCCESS)
        {
            ssz_static_computed_value_t *new_values =
                (ssz_static_computed_value_t *)realloc(ctx->values, new_capacity * sizeof(*new_values));
            if (new_values == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                size_t added = new_capacity - ctx->capacity;
                (void)memset(&new_values[ctx->capacity], 0, added * sizeof(*new_values));
                ctx->values = new_values;
                ctx->capacity = new_capacity;
            }
        }
    }

    return err;
}

static ssz_error_t ssz_static_capture_sequence_value(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    ssz_static_decode_sequence_ctx_t *sequence_ctx = (ssz_static_decode_sequence_ctx_t *)ctx;
    size_t required = 0u;
    ssz_error_t err = SSZ_SUCCESS;

    if ((sequence_ctx == NULL) || (sequence_ctx->elem_type == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_u64_to_size(member_id + 1u, &required))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        err = ssz_static_decode_sequence_ensure_capacity(sequence_ctx, required);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_static_compute_from_bytes_internal(
                sequence_ctx->elem_type,
                data,
                data_len,
                &sequence_ctx->values[member_id]);
            if ((err == SSZ_SUCCESS) && (required > sequence_ctx->count))
            {
                sequence_ctx->count = required;
            }
        }
    }

    return err;
}

static ssz_error_t ssz_static_capture_container_value(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    ssz_static_decode_container_ctx_t *container_ctx = (ssz_static_decode_container_ctx_t *)ctx;
    const ssz_static_schema_type_t *field_type = NULL;

    if ((container_ctx == NULL) || (container_ctx->type == NULL) || (container_ctx->values == NULL) ||
        (member_id >= container_ctx->type->field_count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    field_type = ssz_static_type_at(
        g_ssz_static_schema_fields[container_ctx->type->field_index + member_id].child_type_index);
    if (field_type == NULL)
    {
        return SSZ_ERR_SCHEMA_INVALID;
    }

    return ssz_static_compute_from_bytes_internal(
        field_type,
        data,
        data_len,
        &container_ctx->values[member_id]);
}

static ssz_error_t ssz_static_store_serialized(
    ssz_static_computed_value_t *value,
    const uint8_t *bytes,
    size_t byte_len)
{
    uint8_t *copy = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if (value == NULL)
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        copy = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            if (byte_len != 0u)
            {
                (void)memcpy(copy, bytes, byte_len);
            }
            value->bytes = copy;
            value->len = byte_len;
        }
    }

    return err;
}

static ssz_error_t ssz_static_compute_basic_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        out_value->type = type;

        if (type->kind == SSZ_STATIC_SCHEMA_KIND_BOOL)
        {
            uint8_t value = 0u;
            uint8_t encoded[1];

            err = ssz_deserialize_boolean(input, byte_len, &value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_boolean(value, &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_boolean(value, encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT8)
        {
            uint8_t value = 0u;
            uint8_t encoded[1];

            err = ssz_deserialize_uint8(input, byte_len, &value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint8(value, &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint8(value, encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT16)
        {
            uint16_t value = 0u;
            uint8_t encoded[2];

            err = ssz_deserialize_uint16(input, byte_len, &value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint16(value, &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint16(value, encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT32)
        {
            uint32_t value = 0u;
            uint8_t encoded[4];

            err = ssz_deserialize_uint32(input, byte_len, &value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint32(value, &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint32(value, encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT64)
        {
            uint64_t value = 0u;
            uint8_t encoded[8];

            err = ssz_deserialize_uint64(input, byte_len, &value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint64(value, &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint64(value, encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT128)
        {
            uint8_t value[16];
            uint8_t encoded[16];

            err = ssz_deserialize_uint128(input, byte_len, value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint128(value, sizeof(value), &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint128(value, sizeof(value), encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT256)
        {
            uint8_t value[32];
            uint8_t encoded[32];

            err = ssz_deserialize_uint256(input, byte_len, value);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_uint256(value, sizeof(value), &out_value->root);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_uint256(value, sizeof(value), encoded);
            }
            if (err == SSZ_SUCCESS)
            {
                err = ssz_static_store_serialized(out_value, encoded, sizeof(encoded));
            }
        }
        else
        {
            err = SSZ_ERR_SCHEMA_INVALID;
        }
    }

    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_byte_vector_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *copy = NULL;
    size_t expected_len = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_u64_to_size(type->param, &expected_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        out_value->type = type;
        copy = (uint8_t *)malloc(expected_len == 0u ? 1u : expected_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_vector_fixed(input, byte_len, type->param, 1u, copy, expected_len);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_vector_fixed(copy, type->param, 1u, ssz_hash_default(), &out_value->root);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_serialize_vector_fixed(copy, type->param, 1u, NULL, 0u, &out_len);
    }
    if (err == SSZ_SUCCESS)
    {
        serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (serialized == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_serialize_vector_fixed(copy, type->param, 1u, serialized, out_len, &out_len);
        }
    }
    if (err == SSZ_SUCCESS)
    {
        out_value->bytes = serialized;
        out_value->len = out_len;
        serialized = NULL;
    }

    free(serialized);
    free(copy);
    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_byte_list_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *copy = NULL;
    uint64_t count = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        out_value->type = type;
        copy = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_list_fixed(input, byte_len, type->param, 1u, copy, byte_len, &count);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_list_fixed(copy, count, type->param, 1u, ssz_hash_default(), &out_value->root);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_serialize_list_fixed(copy, count, type->param, 1u, NULL, 0u, &out_len);
    }
    if (err == SSZ_SUCCESS)
    {
        serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (serialized == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_serialize_list_fixed(copy, count, type->param, 1u, serialized, out_len, &out_len);
        }
    }
    if (err == SSZ_SUCCESS)
    {
        out_value->bytes = serialized;
        out_value->len = out_len;
        serialized = NULL;
    }

    free(serialized);
    free(copy);
    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_bit_vector_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *bits = NULL;
    size_t bitfield_len = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_bits_to_bytes(type->param, &bitfield_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        out_value->type = type;
        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_bitvector(input, byte_len, type->param, bits, bitfield_len);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_bitvector(bits, bitfield_len, type->param, ssz_hash_default(), &out_value->root);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_serialize_bitvector(bits, bitfield_len, type->param, NULL, 0u, &out_len);
    }
    if (err == SSZ_SUCCESS)
    {
        serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (serialized == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_serialize_bitvector(bits, bitfield_len, type->param, serialized, out_len, &out_len);
        }
    }
    if (err == SSZ_SUCCESS)
    {
        out_value->bytes = serialized;
        out_value->len = out_len;
        serialized = NULL;
    }

    free(serialized);
    free(bits);
    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_bit_list_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *bits = NULL;
    size_t bitfield_len = byte_len == 0u ? 1u : byte_len;
    size_t bits_len = 0u;
    uint64_t bit_len = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        out_value->type = type;
        bits = (uint8_t *)malloc(bitfield_len);
        if (bits == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_bitlist(input, byte_len, type->param, bits, bitfield_len, &bit_len);
        }
    }

    if ((err == SSZ_SUCCESS) && !ssz_static_bits_to_bytes(bit_len, &bits_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_bitlist(bits, bitfield_len, bit_len, type->param, ssz_hash_default(), &out_value->root);
    }
    if (err == SSZ_SUCCESS)
    {
        err = ssz_serialize_bitlist(bits, bits_len, bit_len, type->param, NULL, 0u, &out_len);
    }
    if (err == SSZ_SUCCESS)
    {
        serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
        if (serialized == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_serialize_bitlist(bits, bits_len, bit_len, type->param, serialized, out_len, &out_len);
        }
    }
    if (err == SSZ_SUCCESS)
    {
        out_value->bytes = serialized;
        out_value->len = out_len;
        serialized = NULL;
    }

    free(serialized);
    free(bits);
    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_decode_fixed_children(
    const ssz_static_schema_type_t *elem_type,
    const uint8_t *bytes,
    size_t elem_size,
    size_t count,
    ssz_static_computed_value_t **out_values)
{
    ssz_static_computed_value_t *values = NULL;
    ssz_error_t err = SSZ_SUCCESS;

    if ((elem_type == NULL) || ((bytes == NULL) && (count != 0u)) || (out_values == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    values = (ssz_static_computed_value_t *)calloc(count == 0u ? 1u : count, sizeof(*values));
    if (values == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; (i < count) && (err == SSZ_SUCCESS); i++)
    {
        err = ssz_static_compute_from_bytes_internal(
            elem_type,
            bytes + (i * elem_size),
            elem_size,
            &values[i]);
    }

    if (err == SSZ_SUCCESS)
    {
        *out_values = values;
    }
    else
    {
        ssz_static_free_computed_array(values, count);
    }

    return err;
}

static ssz_error_t ssz_static_validate_basic(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BOOL)
    {
        uint8_t value = 0u;
        err = ssz_deserialize_boolean(input, byte_len, &value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT8)
    {
        uint8_t value = 0u;
        err = ssz_deserialize_uint8(input, byte_len, &value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT16)
    {
        uint16_t value = 0u;
        err = ssz_deserialize_uint16(input, byte_len, &value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT32)
    {
        uint32_t value = 0u;
        err = ssz_deserialize_uint32(input, byte_len, &value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT64)
    {
        uint64_t value = 0u;
        err = ssz_deserialize_uint64(input, byte_len, &value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT128)
    {
        uint8_t value[16];
        err = ssz_deserialize_uint128(input, byte_len, value);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_UINT256)
    {
        uint8_t value[32];
        err = ssz_deserialize_uint256(input, byte_len, value);
    }
    else
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_static_root_from_small_bytes(bytes, byte_len, out_root);
    }

    return err;
}

static ssz_error_t ssz_static_validate_byte_vector(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    uint8_t *copy = NULL;
    size_t expected_len = 0u;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_u64_to_size(type->param, &expected_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        copy = (uint8_t *)malloc(expected_len == 0u ? 1u : expected_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_vector_fixed(input, byte_len, type->param, 1u, copy, expected_len);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_vector_fixed(copy, type->param, 1u, ssz_hash_default(), out_root);
    }

    free(copy);
    return err;
}

static ssz_error_t ssz_static_validate_byte_list(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    uint8_t *copy = NULL;
    uint64_t count = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        copy = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_list_fixed(input, byte_len, type->param, 1u, copy, byte_len, &count);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_list_fixed(copy, count, type->param, 1u, ssz_hash_default(), out_root);
    }

    free(copy);
    return err;
}

static ssz_error_t ssz_static_validate_bit_vector(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    uint8_t *bits = NULL;
    size_t bitfield_len = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (!ssz_static_bits_to_bytes(type->param, &bitfield_len))
    {
        err = SSZ_ERR_OVERFLOW;
    }
    else
    {
        bits = (uint8_t *)malloc(bitfield_len == 0u ? 1u : bitfield_len);
        if (bits == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_bitvector(input, byte_len, type->param, bits, bitfield_len);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_bitvector(bits, bitfield_len, type->param, ssz_hash_default(), out_root);
    }

    free(bits);
    return err;
}

static ssz_error_t ssz_static_validate_bit_list(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    uint8_t *bits = NULL;
    size_t bitfield_len = byte_len == 0u ? 1u : byte_len;
    uint64_t bit_len = 0u;
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        bits = (uint8_t *)malloc(bitfield_len);
        if (bits == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_bitlist(input, byte_len, type->param, bits, bitfield_len, &bit_len);
        }
    }

    if (err == SSZ_SUCCESS)
    {
        err = ssz_hash_tree_root_bitlist(bits, bitfield_len, bit_len, type->param, ssz_hash_default(), out_root);
    }

    free(bits);
    return err;
}

static ssz_error_t ssz_static_validate_fixed_children(
    const ssz_static_schema_type_t *elem_type,
    const uint8_t *bytes,
    size_t elem_size,
    size_t count,
    ssz_chunk_t *out_roots)
{
    ssz_error_t err = SSZ_SUCCESS;

    for (size_t i = 0u; (i < count) && (err == SSZ_SUCCESS); i++)
    {
        ssz_chunk_t child_root;
        ssz_chunk_t *root_ptr = (out_roots != NULL) ? &out_roots[i] : &child_root;

        err = ssz_static_validate_and_root_internal(
            elem_type,
            bytes + (i * elem_size),
            elem_size,
            root_ptr);
    }

    return err;
}

static ssz_error_t ssz_static_validate_vector(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    const ssz_static_schema_type_t *elem_type = ssz_static_type_at(type->child_type_index);
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL) || (elem_type == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (elem_type->fixed_size != SSZ_STATIC_SCHEMA_DYNAMIC_SIZE)
    {
        size_t elem_size = elem_type->fixed_size;
        size_t element_count = 0u;
        size_t total_len = 0u;
        uint8_t *copy = NULL;
        ssz_chunk_t *roots = NULL;

        if (!ssz_static_u64_to_size(type->param, &element_count) ||
            ssz_static_mul_overflow_size(element_count, elem_size, &total_len))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            copy = (uint8_t *)malloc(total_len == 0u ? 1u : total_len);
            if (copy == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                if (elem_type->kind == SSZ_STATIC_SCHEMA_KIND_BOOL)
                {
                    err = ssz_deserialize_vector_boolean(input, byte_len, type->param, copy, total_len);
                }
                else
                {
                    err = ssz_deserialize_vector_fixed(input, byte_len, type->param, elem_size, copy, total_len);
                }
            }
        }

        if ((err == SSZ_SUCCESS) && ssz_static_kind_is_basic(elem_type->kind))
        {
            err = ssz_static_validate_fixed_children(elem_type, copy, elem_size, element_count, NULL);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_vector_fixed(
                    copy,
                    type->param,
                    elem_size,
                    ssz_hash_default(),
                    out_root);
            }
        }
        else if (err == SSZ_SUCCESS)
        {
            roots = ssz_static_alloc_roots(element_count);
            if (roots == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = ssz_static_validate_fixed_children(elem_type, copy, elem_size, element_count, roots);
                if (err == SSZ_SUCCESS)
                {
                    err = ssz_hash_tree_root_vector_roots(
                        roots,
                        type->param,
                        &g_spec_merkle_scratch,
                        ssz_hash_default(),
                        out_root);
                }
            }
        }

        free(roots);
        free(copy);
    }
    else
    {
        ssz_static_sequence_ctx_t ctx = {
            .elem_type = elem_type,
            .roots = NULL,
            .capacity = 0u,
            .count = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &ctx,
            .write = NULL,
            .read = ssz_static_capture_sequence_elem,
            .root = NULL,
        };

        err = ssz_deserialize_vector_variable(input, byte_len, type->param, elem_type->min_size, &codec);
        if ((err == SSZ_SUCCESS) && (ctx.count != (size_t)type->param))
        {
            err = SSZ_ERR_ENCODING_INVALID;
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_hash_tree_root_vector_roots(
                ctx.roots,
                type->param,
                &g_spec_merkle_scratch,
                ssz_hash_default(),
                out_root);
        }

        free(ctx.roots);
    }

    return err;
}

static ssz_error_t ssz_static_validate_list(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    const ssz_static_schema_type_t *elem_type = ssz_static_type_at(type->child_type_index);
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL) || (elem_type == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (elem_type->fixed_size != SSZ_STATIC_SCHEMA_DYNAMIC_SIZE)
    {
        size_t elem_size = elem_type->fixed_size;
        uint8_t *copy = NULL;
        uint64_t count = 0u;
        ssz_chunk_t *roots = NULL;

        copy = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            if (elem_type->kind == SSZ_STATIC_SCHEMA_KIND_BOOL)
            {
                err = ssz_deserialize_list_boolean(input, byte_len, type->param, copy, byte_len, &count);
            }
            else
            {
                err = ssz_deserialize_list_fixed(input, byte_len, type->param, elem_size, copy, byte_len, &count);
            }
        }

        if ((err == SSZ_SUCCESS) && ssz_static_kind_is_basic(elem_type->kind))
        {
            err = ssz_static_validate_fixed_children(elem_type, copy, elem_size, (size_t)count, NULL);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_hash_tree_root_list_fixed(
                    copy,
                    count,
                    type->param,
                    elem_size,
                    ssz_hash_default(),
                    out_root);
            }
        }
        else if (err == SSZ_SUCCESS)
        {
            roots = ssz_static_alloc_roots((size_t)count);
            if (roots == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = ssz_static_validate_fixed_children(elem_type, copy, elem_size, (size_t)count, roots);
                if (err == SSZ_SUCCESS)
                {
                    err = ssz_hash_tree_root_list_roots(
                        roots,
                        count,
                        type->param,
                        &g_spec_merkle_scratch,
                        ssz_hash_default(),
                        out_root);
                }
            }
        }

        free(roots);
        free(copy);
    }
    else
    {
        ssz_static_sequence_ctx_t ctx = {
            .elem_type = elem_type,
            .roots = NULL,
            .capacity = 0u,
            .count = 0u,
        };
        ssz_member_codec_t codec = {
            .ctx = &ctx,
            .write = NULL,
            .read = ssz_static_capture_sequence_elem,
            .root = NULL,
        };
        uint64_t count = 0u;

        err = ssz_deserialize_list_variable(
            input,
            byte_len,
            type->param,
            elem_type->min_size,
            &codec,
            &count);
        if ((err == SSZ_SUCCESS) && (ctx.count != (size_t)count))
        {
            err = SSZ_ERR_ENCODING_INVALID;
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_hash_tree_root_list_roots(
                ctx.roots,
                count,
                type->param,
                &g_spec_merkle_scratch,
                ssz_hash_default(),
                out_root);
        }

        free(ctx.roots);
    }

    return err;
}

static ssz_error_t ssz_static_validate_container(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    ssz_chunk_t *roots = NULL;
    size_t *fixed_sizes = NULL;
    ssz_container_schema_t schema = {0};
    ssz_static_container_ctx_t ctx;
    ssz_member_codec_t codec;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (type->field_count == 0u)
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }
    else
    {
        roots = ssz_static_alloc_roots(type->field_count);
        fixed_sizes = (size_t *)calloc(type->field_count == 0u ? 1u : type->field_count, sizeof(size_t));
        if ((roots == NULL) || (fixed_sizes == NULL))
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
    }

    if (err == SSZ_SUCCESS)
    {
        ctx.type = type;
        ctx.roots = roots;

        codec.ctx = &ctx;
        codec.write = NULL;
        codec.read = ssz_static_capture_container_field;
        codec.root = NULL;

        for (uint32_t i = 0u; i < type->field_count; i++)
        {
            size_t fixed_size = g_ssz_static_schema_fields[type->field_index + i].fixed_size;
            fixed_sizes[i] = (fixed_size == SSZ_STATIC_SCHEMA_DYNAMIC_SIZE) ? 0u : fixed_size;
        }
        schema = ssz_container_schema_init(fixed_sizes, type->field_count);

        err = ssz_deserialize_container(input, byte_len, &schema, &codec);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_hash_tree_root_vector_roots(
                roots,
                type->field_count,
                &g_spec_merkle_scratch,
                ssz_hash_default(),
                out_root);
        }
    }

    free(fixed_sizes);
    free(roots);
    return err;
}

static ssz_error_t ssz_static_validate_and_root_internal(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_root == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else if (ssz_static_kind_is_basic(type->kind))
    {
        err = ssz_static_validate_basic(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BYTE_VECTOR)
    {
        err = ssz_static_validate_byte_vector(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BYTE_LIST)
    {
        err = ssz_static_validate_byte_list(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BIT_VECTOR)
    {
        err = ssz_static_validate_bit_vector(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BIT_LIST)
    {
        err = ssz_static_validate_bit_list(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_VECTOR)
    {
        err = ssz_static_validate_vector(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_LIST)
    {
        err = ssz_static_validate_list(type, bytes, byte_len, out_root);
    }
    else if (type->kind == SSZ_STATIC_SCHEMA_KIND_CONTAINER)
    {
        err = ssz_static_validate_container(type, bytes, byte_len, out_root);
    }
    else
    {
        err = SSZ_ERR_SCHEMA_INVALID;
    }

    return err;
}

static ssz_error_t ssz_static_compute_vector_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    const ssz_static_schema_type_t *elem_type = ssz_static_type_at(type->child_type_index);
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL) || (elem_type == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    out_value->type = type;

    if (elem_type->fixed_size != SSZ_STATIC_SCHEMA_DYNAMIC_SIZE)
    {
        size_t elem_size = elem_type->fixed_size;
        size_t element_count = 0u;
        size_t total_len = 0u;
        uint8_t *copy = NULL;
        uint8_t *flat = NULL;
        size_t flat_len = 0u;
        ssz_static_computed_value_t *children = NULL;
        ssz_static_computed_array_ctx_t ctx = {0};
        ssz_member_codec_t codec = {0};
        size_t out_len = 0u;
        uint8_t *serialized = NULL;

        if (!ssz_static_u64_to_size(type->param, &element_count) ||
            ssz_static_mul_overflow_size(element_count, elem_size, &total_len))
        {
            err = SSZ_ERR_OVERFLOW;
        }
        else
        {
            copy = (uint8_t *)malloc(total_len == 0u ? 1u : total_len);
            if (copy == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = ssz_deserialize_vector_fixed(input, byte_len, type->param, elem_size, copy, total_len);
            }
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_static_decode_fixed_children(elem_type, copy, elem_size, element_count, &children);
        }
        if (err == SSZ_SUCCESS)
        {
            ctx.values = children;
            ctx.count = element_count;
            codec.ctx = &ctx;
            codec.write = ssz_static_computed_write_cb;
            codec.read = NULL;
            codec.root = ssz_static_computed_root_cb;
            err = ssz_static_concat_fixed_children(children, element_count, elem_size, &flat, &flat_len);
        }
        if ((err == SSZ_SUCCESS) && ssz_static_kind_is_basic(elem_type->kind))
        {
            err = ssz_hash_tree_root_vector_fixed(
                flat,
                type->param,
                elem_size,
                ssz_hash_default(),
                &out_value->root);
        }
        else if (err == SSZ_SUCCESS)
        {
            err = ssz_hash_tree_root_vector_composite(type->param, &codec, ssz_hash_default(), &out_value->root);
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_serialize_vector_fixed(flat, type->param, elem_size, NULL, 0u, &out_len);
            if (err == SSZ_SUCCESS)
            {
                serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
                if (serialized == NULL)
                {
                    err = SSZ_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    err = ssz_serialize_vector_fixed(
                        flat,
                        type->param,
                        elem_size,
                        serialized,
                        out_len,
                        &out_len);
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            out_value->bytes = serialized;
            out_value->len = out_len;
            serialized = NULL;
        }

        free(serialized);
        free(flat);
        free(copy);
        ssz_static_free_computed_array(children, element_count);
    }
    else
    {
        ssz_static_decode_sequence_ctx_t decode_ctx = {
            .elem_type = elem_type,
            .values = NULL,
            .capacity = 0u,
            .count = 0u,
        };
        ssz_member_codec_t decode_codec = {
            .ctx = &decode_ctx,
            .write = NULL,
            .read = ssz_static_capture_sequence_value,
            .root = NULL,
        };
        ssz_static_computed_array_ctx_t computed_ctx = {0};
        ssz_member_codec_t codec = {0};
        size_t out_len = 0u;
        uint8_t *serialized = NULL;

        err = ssz_deserialize_vector_variable(input, byte_len, type->param, elem_type->min_size, &decode_codec);
        if ((err == SSZ_SUCCESS) && (decode_ctx.count != (size_t)type->param))
        {
            err = SSZ_ERR_ENCODING_INVALID;
        }

        if (err == SSZ_SUCCESS)
        {
            computed_ctx.values = decode_ctx.values;
            computed_ctx.count = decode_ctx.count;
            codec.ctx = &computed_ctx;
            codec.write = ssz_static_computed_write_cb;
            codec.read = NULL;
            codec.root = ssz_static_computed_root_cb;

            err = ssz_hash_tree_root_vector_composite(type->param, &codec, ssz_hash_default(), &out_value->root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_vector_variable(type->param, &codec, NULL, 0u, &out_len);
            }
            if (err == SSZ_SUCCESS)
            {
                serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
                if (serialized == NULL)
                {
                    err = SSZ_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    err = ssz_serialize_vector_variable(
                        type->param,
                        &codec,
                        serialized,
                        out_len,
                        &out_len);
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            out_value->bytes = serialized;
            out_value->len = out_len;
            serialized = NULL;
        }

        free(serialized);
        ssz_static_free_computed_array(decode_ctx.values, decode_ctx.count);
    }

    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_list_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    const ssz_static_schema_type_t *elem_type = ssz_static_type_at(type->child_type_index);
    ssz_error_t err = SSZ_SUCCESS;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL) || (elem_type == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    out_value->type = type;

    if (elem_type->fixed_size != SSZ_STATIC_SCHEMA_DYNAMIC_SIZE)
    {
        size_t elem_size = elem_type->fixed_size;
        uint8_t *copy = NULL;
        uint64_t count = 0u;
        uint8_t *flat = NULL;
        size_t flat_len = 0u;
        ssz_static_computed_value_t *children = NULL;
        ssz_static_computed_array_ctx_t ctx = {0};
        ssz_member_codec_t codec = {0};
        size_t out_len = 0u;
        uint8_t *serialized = NULL;

        copy = (uint8_t *)malloc(byte_len == 0u ? 1u : byte_len);
        if (copy == NULL)
        {
            err = SSZ_ERR_INVALID_ARGUMENT;
        }
        else
        {
            err = ssz_deserialize_list_fixed(input, byte_len, type->param, elem_size, copy, byte_len, &count);
        }

        if (err == SSZ_SUCCESS)
        {
            err = ssz_static_decode_fixed_children(elem_type, copy, elem_size, (size_t)count, &children);
        }
        if (err == SSZ_SUCCESS)
        {
            ctx.values = children;
            ctx.count = (size_t)count;
            codec.ctx = &ctx;
            codec.write = ssz_static_computed_write_cb;
            codec.read = NULL;
            codec.root = ssz_static_computed_root_cb;
            err = ssz_static_concat_fixed_children(children, (size_t)count, elem_size, &flat, &flat_len);
        }
        if ((err == SSZ_SUCCESS) && ssz_static_kind_is_basic(elem_type->kind))
        {
            err = ssz_hash_tree_root_list_fixed(
                flat,
                count,
                type->param,
                elem_size,
                ssz_hash_default(),
                &out_value->root);
        }
        else if (err == SSZ_SUCCESS)
        {
            err = ssz_hash_tree_root_list_composite(
                count,
                type->param,
                &codec,
                ssz_hash_default(),
                &out_value->root);
        }
        if (err == SSZ_SUCCESS)
        {
            err = ssz_serialize_list_fixed(flat, count, type->param, elem_size, NULL, 0u, &out_len);
            if (err == SSZ_SUCCESS)
            {
                serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
                if (serialized == NULL)
                {
                    err = SSZ_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    err = ssz_serialize_list_fixed(
                        flat,
                        count,
                        type->param,
                        elem_size,
                        serialized,
                        out_len,
                        &out_len);
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            out_value->bytes = serialized;
            out_value->len = out_len;
            serialized = NULL;
        }

        free(serialized);
        free(flat);
        free(copy);
        ssz_static_free_computed_array(children, (size_t)count);
    }
    else
    {
        ssz_static_decode_sequence_ctx_t decode_ctx = {
            .elem_type = elem_type,
            .values = NULL,
            .capacity = 0u,
            .count = 0u,
        };
        ssz_member_codec_t decode_codec = {
            .ctx = &decode_ctx,
            .write = NULL,
            .read = ssz_static_capture_sequence_value,
            .root = NULL,
        };
        ssz_static_computed_array_ctx_t computed_ctx = {0};
        ssz_member_codec_t codec = {0};
        uint64_t count = 0u;
        size_t out_len = 0u;
        uint8_t *serialized = NULL;

        err = ssz_deserialize_list_variable(
            input,
            byte_len,
            type->param,
            elem_type->min_size,
            &decode_codec,
            &count);
        if ((err == SSZ_SUCCESS) && (decode_ctx.count != (size_t)count))
        {
            err = SSZ_ERR_ENCODING_INVALID;
        }

        if (err == SSZ_SUCCESS)
        {
            computed_ctx.values = decode_ctx.values;
            computed_ctx.count = decode_ctx.count;
            codec.ctx = &computed_ctx;
            codec.write = ssz_static_computed_write_cb;
            codec.read = NULL;
            codec.root = ssz_static_computed_root_cb;

            err = ssz_hash_tree_root_list_composite(count, type->param, &codec, ssz_hash_default(), &out_value->root);
            if (err == SSZ_SUCCESS)
            {
                err = ssz_serialize_list_variable(count, type->param, &codec, NULL, 0u, &out_len);
            }
            if (err == SSZ_SUCCESS)
            {
                serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
                if (serialized == NULL)
                {
                    err = SSZ_ERR_INVALID_ARGUMENT;
                }
                else
                {
                    err = ssz_serialize_list_variable(
                        count,
                        type->param,
                        &codec,
                        serialized,
                        out_len,
                        &out_len);
                }
            }
        }

        if (err == SSZ_SUCCESS)
        {
            out_value->bytes = serialized;
            out_value->len = out_len;
            serialized = NULL;
        }

        free(serialized);
        ssz_static_free_computed_array(decode_ctx.values, decode_ctx.count);
    }

    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_container_value(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;
    ssz_static_computed_value_t *children = NULL;
    size_t *fixed_sizes = NULL;
    ssz_container_schema_t schema = {0};
    ssz_static_decode_container_ctx_t decode_ctx;
    ssz_static_computed_array_ctx_t computed_ctx = {0};
    ssz_member_codec_t decode_codec = {0};
    ssz_member_codec_t codec = {0};
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    const uint8_t *input = ssz_static_input_bytes(bytes, byte_len);

    if ((type == NULL) || (input == NULL) || (out_value == NULL) || (type->field_count == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    out_value->type = type;

    children = (ssz_static_computed_value_t *)calloc(type->field_count == 0u ? 1u : type->field_count,
                                                     sizeof(*children));
    fixed_sizes = (size_t *)calloc(type->field_count == 0u ? 1u : type->field_count, sizeof(*fixed_sizes));
    if ((children == NULL) || (fixed_sizes == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }

    if (err == SSZ_SUCCESS)
    {
        for (uint32_t i = 0u; i < type->field_count; i++)
        {
            size_t fixed_size = g_ssz_static_schema_fields[type->field_index + i].fixed_size;
            fixed_sizes[i] = (fixed_size == SSZ_STATIC_SCHEMA_DYNAMIC_SIZE) ? 0u : fixed_size;
        }
        schema = ssz_container_schema_init(fixed_sizes, type->field_count);

        decode_ctx.type = type;
        decode_ctx.values = children;

        decode_codec.ctx = &decode_ctx;
        decode_codec.write = NULL;
        decode_codec.read = ssz_static_capture_container_value;
        decode_codec.root = NULL;

        err = ssz_deserialize_container(input, byte_len, &schema, &decode_codec);
    }

    if (err == SSZ_SUCCESS)
    {
        computed_ctx.values = children;
        computed_ctx.count = type->field_count;
        codec.ctx = &computed_ctx;
        codec.write = ssz_static_computed_write_cb;
        codec.read = NULL;
        codec.root = ssz_static_computed_root_cb;

        err = ssz_hash_tree_root_vector_composite(type->field_count, &codec, ssz_hash_default(), &out_value->root);
        if (err == SSZ_SUCCESS)
        {
            err = ssz_serialize_container(&schema, &codec, NULL, 0u, &out_len);
        }
        if (err == SSZ_SUCCESS)
        {
            serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
            if (serialized == NULL)
            {
                err = SSZ_ERR_INVALID_ARGUMENT;
            }
            else
            {
                err = ssz_serialize_container(
                    &schema,
                    &codec,
                    serialized,
                    out_len,
                    &out_len);
            }
        }
    }

    if (err == SSZ_SUCCESS)
    {
        out_value->bytes = serialized;
        out_value->len = out_len;
        serialized = NULL;
    }

    free(serialized);
    free(fixed_sizes);
    ssz_static_free_computed_array(children, type->field_count);

    if (err != SSZ_SUCCESS)
    {
        ssz_static_computed_value_reset(out_value);
    }

    return err;
}

static ssz_error_t ssz_static_compute_from_bytes_internal(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    ssz_error_t err = SSZ_SUCCESS;

    if ((type == NULL) || (out_value == NULL))
    {
        err = SSZ_ERR_INVALID_ARGUMENT;
    }
    else
    {
        (void)memset(out_value, 0, sizeof(*out_value));

        if (ssz_static_kind_is_basic(type->kind))
        {
            err = ssz_static_compute_basic_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BYTE_VECTOR)
        {
            err = ssz_static_compute_byte_vector_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BYTE_LIST)
        {
            err = ssz_static_compute_byte_list_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BIT_VECTOR)
        {
            err = ssz_static_compute_bit_vector_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_BIT_LIST)
        {
            err = ssz_static_compute_bit_list_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_VECTOR)
        {
            err = ssz_static_compute_vector_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_LIST)
        {
            err = ssz_static_compute_list_value(type, bytes, byte_len, out_value);
        }
        else if (type->kind == SSZ_STATIC_SCHEMA_KIND_CONTAINER)
        {
            err = ssz_static_compute_container_value(type, bytes, byte_len, out_value);
        }
        else
        {
            err = SSZ_ERR_SCHEMA_INVALID;
        }
    }

    return err;
}

const ssz_static_schema_type_t *ssz_static_find_root_type(
    const char *preset,
    const char *fork,
    const char *handler)
{
    const ssz_static_schema_type_t *type = NULL;

    if ((preset == NULL) || (fork == NULL) || (handler == NULL))
    {
        return NULL;
    }

    for (size_t i = 0u; i < g_ssz_static_schema_registry_count; i++)
    {
        if ((strcmp(g_ssz_static_schema_registry[i].preset, preset) == 0) &&
            (strcmp(g_ssz_static_schema_registry[i].fork, fork) == 0) &&
            (strcmp(g_ssz_static_schema_registry[i].handler, handler) == 0))
        {
            type = ssz_static_type_at(g_ssz_static_schema_registry[i].type_index);
            break;
        }
    }

    return type;
}

ssz_error_t ssz_static_compute_from_bytes(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value)
{
    return ssz_static_compute_from_bytes_internal(type, bytes, byte_len, out_value);
}

ssz_error_t ssz_static_validate_and_root(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root)
{
    return ssz_static_validate_and_root_internal(type, bytes, byte_len, out_root);
}
