#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/compatible_unions"
#endif

typedef enum
{
    UNION_FAMILY_A = 0,
    UNION_FAMILY_BC,
    UNION_FAMILY_ABCA,
} union_family_t;

typedef enum
{
    PAYLOAD_SINGLE_FIELD = 0,
    PAYLOAD_SINGLE_LIST,
    PAYLOAD_VAR,
} payload_kind_t;

typedef struct
{
    uint8_t *bytes;
    size_t len;
    ssz_chunk_t root;
} computed_value_t;

typedef struct
{
    const computed_value_t *values;
    uint32_t count;
} computed_field_array_t;

typedef struct
{
    union_family_t family;
    uint8_t selector;
    const computed_value_t *expected_payload;
} union_valid_read_ctx_t;

typedef struct
{
    union_family_t family;
} union_invalid_read_ctx_t;

typedef struct
{
    uint8_t selector;
    const computed_value_t *payload;
} union_write_ctx_t;

static const uint8_t ALLOWED_SELECTORS_A[] = {1u};
static const uint8_t ALLOWED_SELECTORS_BC[] = {2u, 3u};
static const uint8_t ALLOWED_SELECTORS_ABCA[] = {1u, 2u, 3u, 4u};

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

static bool parse_case_family(const char *case_name, union_family_t *out_family)
{
    if ((case_name == NULL) || (out_family == NULL))
    {
        return false;
    }

    if (strncmp(case_name, "CompatibleUnionABCA_", 20u) == 0)
    {
        *out_family = UNION_FAMILY_ABCA;
        return true;
    }
    if (strncmp(case_name, "CompatibleUnionBC_", 18u) == 0)
    {
        *out_family = UNION_FAMILY_BC;
        return true;
    }
    if (strncmp(case_name, "CompatibleUnionA_", 17u) == 0)
    {
        *out_family = UNION_FAMILY_A;
        return true;
    }

    return false;
}

static bool allowed_selectors_for_family(
    union_family_t family,
    const uint8_t **out_selectors,
    uint32_t *out_count)
{
    if ((out_selectors == NULL) || (out_count == NULL))
    {
        return false;
    }

    if (family == UNION_FAMILY_A)
    {
        *out_selectors = ALLOWED_SELECTORS_A;
        *out_count = 1u;
        return true;
    }
    if (family == UNION_FAMILY_BC)
    {
        *out_selectors = ALLOWED_SELECTORS_BC;
        *out_count = 2u;
        return true;
    }

    *out_selectors = ALLOWED_SELECTORS_ABCA;
    *out_count = 4u;
    return true;
}

static bool selector_to_payload_kind(
    union_family_t family,
    uint8_t selector,
    payload_kind_t *out_kind)
{
    if (out_kind == NULL)
    {
        return false;
    }

    if (family == UNION_FAMILY_A)
    {
        if (selector == 1u)
        {
            *out_kind = PAYLOAD_SINGLE_FIELD;
            return true;
        }
        return false;
    }

    if (family == UNION_FAMILY_BC)
    {
        if (selector == 2u)
        {
            *out_kind = PAYLOAD_SINGLE_LIST;
            return true;
        }
        if (selector == 3u)
        {
            *out_kind = PAYLOAD_VAR;
            return true;
        }
        return false;
    }

    if (selector == 1u)
    {
        *out_kind = PAYLOAD_SINGLE_FIELD;
        return true;
    }
    if (selector == 2u)
    {
        *out_kind = PAYLOAD_SINGLE_LIST;
        return true;
    }
    if (selector == 3u)
    {
        *out_kind = PAYLOAD_VAR;
        return true;
    }
    if (selector == 4u)
    {
        *out_kind = PAYLOAD_SINGLE_FIELD;
        return true;
    }

    return false;
}

static ssz_error_t parse_yaml_u8(const yaml_node_t *node, uint8_t *out_value)
{
    uint64_t value = 0u;

    if ((node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (!yaml_node_parse_u64(node, &value) || (value > UINT8_MAX))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    *out_value = (uint8_t)value;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_uint8_value(const yaml_node_t *node, computed_value_t *out_value)
{
    uint8_t value = 0u;
    uint8_t encoded[1];
    ssz_error_t err;

    if ((node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = parse_yaml_u8(node, &value);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = ssz_serialize_uint8(value, encoded);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = copy_bytes(encoded, sizeof(encoded), &out_value->bytes);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    out_value->len = sizeof(encoded);
    return ssz_hash_tree_root_uint8(value, &out_value->root);
}

static ssz_error_t compute_list_u16_123_value(const yaml_node_t *node, computed_value_t *out_value)
{
    uint8_t *flat = NULL;
    size_t flat_len = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    ssz_error_t err;

    if ((node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (node->type != YAML_NODE_SEQUENCE)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }
    if (node->as.sequence.count > 123u)
    {
        return SSZ_ERR_LIMIT_EXCEEDED;
    }

    if ((node->as.sequence.count != 0u) &&
        (node->as.sequence.count > (SIZE_MAX / sizeof(uint16_t))))
    {
        return SSZ_ERR_OVERFLOW;
    }

    flat_len = node->as.sequence.count * sizeof(uint16_t);
    flat = (uint8_t *)malloc(flat_len == 0u ? 1u : flat_len);
    if (flat == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < node->as.sequence.count; i++)
    {
        uint64_t parsed = 0u;

        if (!yaml_node_parse_u64(node->as.sequence.items[i], &parsed) || (parsed > UINT16_MAX))
        {
            free(flat);
            return SSZ_ERR_TYPE_MISMATCH;
        }

        err = ssz_serialize_uint16((uint16_t)parsed, flat + i * sizeof(uint16_t));
        if (err != SSZ_SUCCESS)
        {
            free(flat);
            return err;
        }
    }

    err = ssz_serialize_list_fixed(flat, node->as.sequence.count, 123u, sizeof(uint16_t), NULL, 0u,
        &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(flat);
        return err;
    }

    serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
    if (serialized == NULL)
    {
        free(flat);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_serialize_list_fixed(flat, node->as.sequence.count, 123u, sizeof(uint16_t), serialized,
        out_len, &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(flat);
        free(serialized);
        return err;
    }

    err = ssz_hash_tree_root_list_fixed(flat, node->as.sequence.count, 123u, sizeof(uint16_t),
        ssz_hash_default(), &out_value->root);
    free(flat);

    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = out_len;
    return SSZ_SUCCESS;
}

static ssz_error_t compute_progressive_bitlist_value(const yaml_node_t *node, computed_value_t *out_value)
{
    uint8_t *encoded = NULL;
    size_t encoded_len = 0u;
    size_t bitfield_len = 0u;
    uint8_t *bits = NULL;
    uint64_t bit_len = 0u;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    ssz_error_t err;

    if ((node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!yaml_node_parse_hex(node, &encoded, &encoded_len))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

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

    serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
    if (serialized == NULL)
    {
        free(encoded);
        free(bits);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_serialize_progressive_bitlist(bits, bitfield_len, bit_len, serialized, out_len, &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(encoded);
        free(bits);
        free(serialized);
        return err;
    }

    err = ssz_hash_tree_root_progressive_bitlist(bits, bitfield_len, bit_len, ssz_hash_default(),
        &out_value->root);

    free(encoded);
    free(bits);

    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = out_len;
    return SSZ_SUCCESS;
}

static ssz_error_t computed_write_cb(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const computed_field_array_t *field_ctx = (const computed_field_array_t *)ctx;
    const computed_value_t *field;

    if ((field_ctx == NULL) || (out_written == NULL) || (member_id >= field_ctx->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    field = &field_ctx->values[member_id];
    *out_written = field->len;

    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < field->len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (field->len != 0u)
    {
        memcpy(out, field->bytes, field->len);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t computed_root_cb(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const computed_field_array_t *field_ctx = (const computed_field_array_t *)ctx;

    if ((field_ctx == NULL) || (out_root == NULL) || (member_id >= field_ctx->count))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memcpy(out_root, &field_ctx->values[member_id].root, sizeof(*out_root));
    return SSZ_SUCCESS;
}

static ssz_error_t compute_progressive_container(
    const computed_value_t *fields,
    uint32_t field_count,
    const size_t *fixed_sizes,
    const uint8_t *field_slots,
    const uint8_t *active_fields,
    size_t active_fields_len,
    computed_value_t *out_value)
{
    computed_field_array_t field_ctx;
    ssz_member_codec_t codec;
    size_t out_len = 0u;
    uint8_t *serialized = NULL;
    size_t slot_count = 0u;
    ssz_chunk_t *roots = NULL;
    ssz_chunk_t data_root;
    ssz_error_t err;

    if ((fields == NULL) || (fixed_sizes == NULL) || (field_slots == NULL) || (active_fields == NULL) ||
        (out_value == NULL) || (field_count == 0u) || (active_fields_len == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    field_ctx.values = fields;
    field_ctx.count = field_count;

    codec.ctx = &field_ctx;
    codec.write = computed_write_cb;
    codec.read = NULL;
    codec.root = computed_root_cb;

    err = ssz_serialize_progressive_container(fixed_sizes, field_count, &codec, NULL, 0u, &out_len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    serialized = (uint8_t *)malloc(out_len == 0u ? 1u : out_len);
    if (serialized == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_serialize_progressive_container(
        fixed_sizes,
        field_count,
        &codec,
        serialized,
        out_len,
        &out_len);
    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    err = ssz_hash_tree_root_progressive_container(
        field_count,
        active_fields,
        active_fields_len,
        &codec,
        ssz_hash_default(),
        &out_value->root);
    if (err != SSZ_SUCCESS)
    {
        free(serialized);
        return err;
    }

    out_value->bytes = serialized;
    out_value->len = out_len;

    for (uint32_t i = 0u; i < field_count; i++)
    {
        size_t slot = (size_t)field_slots[i] + 1u;
        if (slot > slot_count)
        {
            slot_count = slot;
        }
    }
    if (slot_count == 0u)
    {
        computed_value_reset(out_value);
        return SSZ_ERR_SCHEMA_INVALID;
    }

    roots = (ssz_chunk_t *)calloc(slot_count, sizeof(ssz_chunk_t));
    if (roots == NULL)
    {
        computed_value_reset(out_value);
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < field_count; i++)
    {
        roots[field_slots[i]] = fields[i].root;
    }

    err = ssz_merkleize_progressive(roots, slot_count, ssz_hash_default(), &data_root);
    free(roots);
    if (err != SSZ_SUCCESS)
    {
        computed_value_reset(out_value);
        return err;
    }

    err = ssz_mix_in_active_fields(&data_root, active_fields, active_fields_len, ssz_hash_default(),
        &out_value->root);
    if (err != SSZ_SUCCESS)
    {
        computed_value_reset(out_value);
        return err;
    }

    return SSZ_SUCCESS;
}

static ssz_error_t compute_payload_single_field(const yaml_node_t *data_node, computed_value_t *out_value)
{
    computed_value_t fields[1];
    const yaml_node_t *a_node;
    const size_t fixed_sizes[1] = {1u};
    const uint8_t field_slots[1] = {0u};
    const uint8_t active_fields[1] = {0x01u};
    ssz_error_t err;

    if ((data_node == NULL) || (out_value == NULL) || (data_node->type != YAML_NODE_MAPPING))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    memset(fields, 0, sizeof(fields));
    a_node = yaml_mapping_get(data_node, "A");
    if (a_node == NULL)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    err = compute_uint8_value(a_node, &fields[0]);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = compute_progressive_container(fields, 1u, fixed_sizes, field_slots, active_fields,
        sizeof(active_fields), out_value);
    computed_value_reset(&fields[0]);
    return err;
}

static ssz_error_t compute_payload_single_list(const yaml_node_t *data_node, computed_value_t *out_value)
{
    computed_value_t fields[1];
    const yaml_node_t *c_node;
    const size_t fixed_sizes[1] = {0u};
    const uint8_t field_slots[1] = {4u};
    const uint8_t active_fields[1] = {0x10u};
    ssz_error_t err;

    if ((data_node == NULL) || (out_value == NULL) || (data_node->type != YAML_NODE_MAPPING))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    memset(fields, 0, sizeof(fields));
    c_node = yaml_mapping_get(data_node, "C");
    if (c_node == NULL)
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    err = compute_progressive_bitlist_value(c_node, &fields[0]);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = compute_progressive_container(fields, 1u, fixed_sizes, field_slots, active_fields,
        sizeof(active_fields), out_value);
    computed_value_reset(&fields[0]);
    return err;
}

static ssz_error_t compute_payload_var(const yaml_node_t *data_node, computed_value_t *out_value)
{
    computed_value_t fields[3];
    const yaml_node_t *a_node;
    const yaml_node_t *b_node;
    const yaml_node_t *c_node;
    const size_t fixed_sizes[3] = {1u, 0u, 0u};
    const uint8_t field_slots[3] = {0u, 2u, 4u};
    const uint8_t active_fields[1] = {0x15u};
    ssz_error_t err;

    if ((data_node == NULL) || (out_value == NULL) || (data_node->type != YAML_NODE_MAPPING))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    memset(fields, 0, sizeof(fields));
    a_node = yaml_mapping_get(data_node, "A");
    b_node = yaml_mapping_get(data_node, "B");
    c_node = yaml_mapping_get(data_node, "C");
    if ((a_node == NULL) || (b_node == NULL) || (c_node == NULL))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    err = compute_uint8_value(a_node, &fields[0]);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    err = compute_list_u16_123_value(b_node, &fields[1]);
    if (err != SSZ_SUCCESS)
    {
        computed_value_reset(&fields[0]);
        return err;
    }

    err = compute_progressive_bitlist_value(c_node, &fields[2]);
    if (err != SSZ_SUCCESS)
    {
        computed_value_reset(&fields[0]);
        computed_value_reset(&fields[1]);
        return err;
    }

    err = compute_progressive_container(fields, 3u, fixed_sizes, field_slots, active_fields,
        sizeof(active_fields), out_value);

    computed_value_reset(&fields[0]);
    computed_value_reset(&fields[1]);
    computed_value_reset(&fields[2]);

    return err;
}

static ssz_error_t compute_payload_from_yaml(
    payload_kind_t kind,
    const yaml_node_t *data_node,
    computed_value_t *out_value)
{
    if ((data_node == NULL) || (out_value == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    memset(out_value, 0, sizeof(*out_value));

    if (kind == PAYLOAD_SINGLE_FIELD)
    {
        return compute_payload_single_field(data_node, out_value);
    }
    if (kind == PAYLOAD_SINGLE_LIST)
    {
        return compute_payload_single_list(data_node, out_value);
    }

    return compute_payload_var(data_node, out_value);
}

static ssz_error_t validate_progressive_bitlist_bytes(const uint8_t *data, size_t data_len)
{
    uint8_t *bits;
    size_t bitfield_cap = data_len == 0u ? 1u : data_len;
    uint64_t bit_len = 0u;
    ssz_error_t err;

    if (data == NULL && data_len != 0u)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    bits = (uint8_t *)malloc(bitfield_cap);
    if (bits == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_deserialize_progressive_bitlist(data, data_len, bits, bitfield_cap, &bit_len);
    free(bits);
    return err;
}

static ssz_error_t validate_list_u16_123_bytes(const uint8_t *data, size_t data_len)
{
    uint8_t *copy;
    uint64_t count = 0u;
    ssz_error_t err;

    if (data == NULL && data_len != 0u)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    copy = (uint8_t *)malloc(data_len == 0u ? 1u : data_len);
    if (copy == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_deserialize_list_fixed(data, data_len, 123u, sizeof(uint16_t), copy, data_len, &count);
    free(copy);
    return err;
}

typedef struct
{
    bool seen;
} validate_single_field_ctx_t;

static ssz_error_t validate_single_field_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    validate_single_field_ctx_t *field_ctx = (validate_single_field_ctx_t *)ctx;
    uint8_t out = 0u;

    if ((field_ctx == NULL) || (member_id != 0u) || (data_len != 1u))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    field_ctx->seen = true;
    return ssz_deserialize_uint8(data, &out);
}

static ssz_error_t validate_payload_single_field(const uint8_t *data, size_t data_len)
{
    validate_single_field_ctx_t ctx = {0};
    ssz_member_codec_t codec;
    size_t fixed_sizes[1] = {1u};
    ssz_error_t err;

    codec.ctx = &ctx;
    codec.write = NULL;
    codec.read = validate_single_field_read;
    codec.root = NULL;

    err = ssz_deserialize_progressive_container(data, data_len, fixed_sizes, 1u, &codec);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ctx.seen ? SSZ_SUCCESS : SSZ_ERR_TYPE_MISMATCH;
}

typedef struct
{
    bool seen;
} validate_single_list_ctx_t;

static ssz_error_t validate_single_list_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    validate_single_list_ctx_t *field_ctx = (validate_single_list_ctx_t *)ctx;

    if ((field_ctx == NULL) || (member_id != 0u))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    field_ctx->seen = true;
    return validate_progressive_bitlist_bytes(data, data_len);
}

static ssz_error_t validate_payload_single_list(const uint8_t *data, size_t data_len)
{
    validate_single_list_ctx_t ctx = {0};
    ssz_member_codec_t codec;
    size_t fixed_sizes[1] = {0u};
    ssz_error_t err;

    codec.ctx = &ctx;
    codec.write = NULL;
    codec.read = validate_single_list_read;
    codec.root = NULL;

    err = ssz_deserialize_progressive_container(data, data_len, fixed_sizes, 1u, &codec);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return ctx.seen ? SSZ_SUCCESS : SSZ_ERR_TYPE_MISMATCH;
}

typedef struct
{
    bool seen_a;
    bool seen_b;
    bool seen_c;
} validate_var_ctx_t;

static ssz_error_t validate_var_read(void *ctx, uint64_t member_id, const uint8_t *data, size_t data_len)
{
    validate_var_ctx_t *field_ctx = (validate_var_ctx_t *)ctx;
    uint8_t out = 0u;

    if (field_ctx == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (member_id == 0u)
    {
        if (data_len != 1u)
        {
            return SSZ_ERR_TYPE_MISMATCH;
        }
        field_ctx->seen_a = true;
        return ssz_deserialize_uint8(data, &out);
    }
    if (member_id == 1u)
    {
        field_ctx->seen_b = true;
        return validate_list_u16_123_bytes(data, data_len);
    }
    if (member_id == 2u)
    {
        field_ctx->seen_c = true;
        return validate_progressive_bitlist_bytes(data, data_len);
    }

    return SSZ_ERR_TYPE_MISMATCH;
}

static ssz_error_t validate_payload_var(const uint8_t *data, size_t data_len)
{
    validate_var_ctx_t ctx = {0};
    ssz_member_codec_t codec;
    size_t fixed_sizes[3] = {1u, 0u, 0u};
    ssz_error_t err;

    codec.ctx = &ctx;
    codec.write = NULL;
    codec.read = validate_var_read;
    codec.root = NULL;

    err = ssz_deserialize_progressive_container(data, data_len, fixed_sizes, 3u, &codec);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    return (ctx.seen_a && ctx.seen_b && ctx.seen_c) ? SSZ_SUCCESS : SSZ_ERR_TYPE_MISMATCH;
}

static ssz_error_t validate_payload_bytes(payload_kind_t kind, const uint8_t *data, size_t data_len)
{
    if (kind == PAYLOAD_SINGLE_FIELD)
    {
        return validate_payload_single_field(data, data_len);
    }
    if (kind == PAYLOAD_SINGLE_LIST)
    {
        return validate_payload_single_list(data, data_len);
    }

    return validate_payload_var(data, data_len);
}

static ssz_error_t union_serialize_write(
    const void *ctx,
    uint64_t member_id,
    uint8_t *out,
    size_t out_cap,
    size_t *out_written)
{
    const union_write_ctx_t *union_ctx = (const union_write_ctx_t *)ctx;

    if ((union_ctx == NULL) || (union_ctx->payload == NULL) || (out_written == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id != union_ctx->selector)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    *out_written = union_ctx->payload->len;

    if (out == NULL)
    {
        return SSZ_SUCCESS;
    }
    if (out_cap < union_ctx->payload->len)
    {
        return SSZ_ERR_BUFFER_TOO_SMALL;
    }

    if (union_ctx->payload->len != 0u)
    {
        memcpy(out, union_ctx->payload->bytes, union_ctx->payload->len);
    }

    return SSZ_SUCCESS;
}

static ssz_error_t union_serialize_root(const void *ctx, uint64_t member_id, ssz_chunk_t *out_root)
{
    const union_write_ctx_t *union_ctx = (const union_write_ctx_t *)ctx;

    if ((union_ctx == NULL) || (union_ctx->payload == NULL) || (out_root == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id != union_ctx->selector)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    memcpy(out_root, &union_ctx->payload->root, sizeof(*out_root));
    return SSZ_SUCCESS;
}

static ssz_error_t union_valid_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    union_valid_read_ctx_t *read_ctx = (union_valid_read_ctx_t *)ctx;
    payload_kind_t kind;

    if ((read_ctx == NULL) || (read_ctx->expected_payload == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (member_id != read_ctx->selector)
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }
    if (!selector_to_payload_kind(read_ctx->family, (uint8_t)member_id, &kind))
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    if ((data_len != read_ctx->expected_payload->len) ||
        ((data_len != 0u) && (memcmp(data, read_ctx->expected_payload->bytes, data_len) != 0)))
    {
        return SSZ_ERR_TYPE_MISMATCH;
    }

    return validate_payload_bytes(kind, data, data_len);
}

static ssz_error_t union_invalid_read(
    void *ctx,
    uint64_t member_id,
    const uint8_t *data,
    size_t data_len)
{
    union_invalid_read_ctx_t *read_ctx = (union_invalid_read_ctx_t *)ctx;
    payload_kind_t kind;

    if (read_ctx == NULL)
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (!selector_to_payload_kind(read_ctx->family, (uint8_t)member_id, &kind))
    {
        return SSZ_ERR_SELECTOR_INVALID;
    }

    return validate_payload_bytes(kind, data, data_len);
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    union_family_t family;
    const uint8_t *allowed_selectors = NULL;
    uint32_t allowed_selector_count = 0u;
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    yaml_node_t *value_doc = NULL;
    const yaml_node_t *selector_node;
    const yaml_node_t *data_node;
    uint64_t parsed_selector = 0u;
    uint8_t selector = 0u;
    payload_kind_t payload_kind;
    computed_value_t payload = {0};
    uint8_t expected_root[32];
    uint8_t decoded_selector = 0u;
    union_valid_read_ctx_t read_ctx;
    ssz_member_codec_t read_codec;
    union_write_ctx_t write_ctx;
    ssz_member_codec_t write_codec;
    size_t reencoded_len = 0u;
    uint8_t *reencoded = NULL;
    ssz_chunk_t actual_root;
    ssz_error_t err;

    report->total_valid++;

    if (!parse_case_family(case_name, &family) ||
        !allowed_selectors_for_family(family, &allowed_selectors, &allowed_selector_count))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_name, "could not parse union family from case name");
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
    if ((value_doc == NULL) || (value_doc->type != YAML_NODE_MAPPING))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse value.yaml");
        goto done;
    }

    selector_node = yaml_mapping_get(value_doc, "selector");
    data_node = yaml_mapping_get(value_doc, "data");
    if ((selector_node == NULL) || (data_node == NULL) || !yaml_node_parse_u64(selector_node, &parsed_selector) ||
        (parsed_selector > UINT8_MAX))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "invalid selector/data in value.yaml");
        goto done;
    }

    selector = (uint8_t)parsed_selector;
    if (!selector_to_payload_kind(family, selector, &payload_kind))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "selector is not valid for union family");
        goto done;
    }

    if (!spec_parse_root_from_meta(meta_path, expected_root))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse root from meta.yaml");
        goto done;
    }

    err = compute_payload_from_yaml(payload_kind, data_node, &payload);
    if (err != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "failed to compute payload from value.yaml");
        goto done;
    }

    read_ctx.family = family;
    read_ctx.selector = selector;
    read_ctx.expected_payload = &payload;

    read_codec.ctx = &read_ctx;
    read_codec.write = NULL;
    read_codec.read = union_valid_read;
    read_codec.root = NULL;

    err = ssz_deserialize_compatible_union(
        serialized,
        serialized_len,
        allowed_selectors,
        allowed_selector_count,
        &read_codec,
        &decoded_selector);
    if (err != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization failed");
        goto done;
    }

    if (decoded_selector != selector)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "decoded selector mismatch");
        goto done;
    }

    write_ctx.selector = selector;
    write_ctx.payload = &payload;

    write_codec.ctx = &write_ctx;
    write_codec.write = union_serialize_write;
    write_codec.read = NULL;
    write_codec.root = union_serialize_root;

    err = ssz_serialize_compatible_union(
        selector,
        allowed_selectors,
        allowed_selector_count,
        &write_codec,
        NULL,
        0u,
        &reencoded_len);
    if (err != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "serialize size query failed");
        goto done;
    }

    reencoded = (uint8_t *)malloc(reencoded_len == 0u ? 1u : reencoded_len);
    if (reencoded == NULL)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "failed to allocate serialization buffer");
        goto done;
    }

    err = ssz_serialize_compatible_union(
        selector,
        allowed_selectors,
        allowed_selector_count,
        &write_codec,
        reencoded,
        reencoded_len,
        &reencoded_len);
    if (err != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "serialization failed");
        goto done;
    }

    if ((reencoded_len != serialized_len) || (memcmp(reencoded, serialized, serialized_len) != 0))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "round-trip serialization mismatch");
        goto done;
    }

    err = ssz_hash_tree_root_union(selector, false, &write_codec, ssz_hash_default(), &actual_root);
    if (err != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "hash_tree_root failed");
        goto done;
    }

    if (memcmp(actual_root.bytes, expected_root, 32u) != 0)
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
    computed_value_reset(&payload);
    free(reencoded);
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    union_family_t family;
    const uint8_t *allowed_selectors = NULL;
    uint32_t allowed_selector_count = 0u;
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    union_invalid_read_ctx_t read_ctx;
    ssz_member_codec_t read_codec;
    uint8_t decoded_selector = 0u;
    ssz_error_t err;

    report->total_invalid++;

    if (!parse_case_family(case_name, &family) ||
        !allowed_selectors_for_family(family, &allowed_selectors, &allowed_selector_count))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_name, "could not parse union family from case name");
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

    read_ctx.family = family;

    read_codec.ctx = &read_ctx;
    read_codec.write = NULL;
    read_codec.read = union_invalid_read;
    read_codec.root = NULL;

    err = ssz_deserialize_compatible_union(
        serialized,
        serialized_len,
        allowed_selectors,
        allowed_selector_count,
        &read_codec,
        &decoded_selector);
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
        int rc = spec_report_print("ssz_generic/compatible_unions", &report);
        spec_report_free(&report);
        return rc;
    }
}
