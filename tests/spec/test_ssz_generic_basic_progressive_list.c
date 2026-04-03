#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/basic_progressive_list"
#endif

typedef enum
{
    LIST_ELEM_BOOL = 0,
    LIST_ELEM_UINT8,
    LIST_ELEM_UINT16,
    LIST_ELEM_UINT32,
    LIST_ELEM_UINT64,
    LIST_ELEM_UINT128,
    LIST_ELEM_UINT256,
} list_elem_kind_t;

static bool parse_case_descriptor(const char *case_name, list_elem_kind_t *out_kind, size_t *out_elem_size)
{
    char type_name[32];

    if ((case_name == NULL) || (out_kind == NULL) || (out_elem_size == NULL))
    {
        return false;
    }

    if (sscanf(case_name, "proglist_%31[^_]_", type_name) != 1)
    {
        return false;
    }

    if (strcmp(type_name, "bool") == 0)
    {
        *out_kind = LIST_ELEM_BOOL;
        *out_elem_size = 1u;
    }
    else if (strcmp(type_name, "uint8") == 0)
    {
        *out_kind = LIST_ELEM_UINT8;
        *out_elem_size = 1u;
    }
    else if (strcmp(type_name, "uint16") == 0)
    {
        *out_kind = LIST_ELEM_UINT16;
        *out_elem_size = 2u;
    }
    else if (strcmp(type_name, "uint32") == 0)
    {
        *out_kind = LIST_ELEM_UINT32;
        *out_elem_size = 4u;
    }
    else if (strcmp(type_name, "uint64") == 0)
    {
        *out_kind = LIST_ELEM_UINT64;
        *out_elem_size = 8u;
    }
    else if (strcmp(type_name, "uint128") == 0)
    {
        *out_kind = LIST_ELEM_UINT128;
        *out_elem_size = 16u;
    }
    else if (strcmp(type_name, "uint256") == 0)
    {
        *out_kind = LIST_ELEM_UINT256;
        *out_elem_size = 32u;
    }
    else
    {
        return false;
    }

    return true;
}

static bool load_expected_list(
    const char *value_path,
    list_elem_kind_t kind,
    size_t elem_size,
    uint8_t **out_bytes,
    size_t *out_len,
    uint64_t *out_count)
{
    yaml_node_t *doc = NULL;
    uint8_t *bytes = NULL;
    size_t count;
    size_t total_len;

    if ((value_path == NULL) || (out_bytes == NULL) || (out_len == NULL) || (out_count == NULL) ||
        (elem_size == 0u))
    {
        return false;
    }

    *out_bytes = NULL;
    *out_len = 0u;
    *out_count = 0u;

    doc = yaml_parse_file(value_path);
    if ((doc == NULL) || (doc->type != YAML_NODE_SEQUENCE))
    {
        yaml_node_free(doc);
        return false;
    }

    count = doc->as.sequence.count;
    if ((count != 0u) && (count > (SIZE_MAX / elem_size)))
    {
        yaml_node_free(doc);
        return false;
    }

    total_len = count * elem_size;

    bytes = (uint8_t *)malloc(total_len == 0u ? 1u : total_len);
    if (bytes == NULL)
    {
        yaml_node_free(doc);
        return false;
    }

    for (size_t i = 0u; i < count; i++)
    {
        const yaml_node_t *item = doc->as.sequence.items[i];
        uint8_t *dst = bytes + i * elem_size;

        if (kind == LIST_ELEM_BOOL)
        {
            if ((item == NULL) || (item->type != YAML_NODE_BOOL))
            {
                free(bytes);
                yaml_node_free(doc);
                return false;
            }
            dst[0] = item->as.bool_value ? 1u : 0u;
            continue;
        }

        if ((kind == LIST_ELEM_UINT8) || (kind == LIST_ELEM_UINT16) ||
            (kind == LIST_ELEM_UINT32) || (kind == LIST_ELEM_UINT64))
        {
            uint64_t value = 0u;

            if (!yaml_node_parse_u64(item, &value))
            {
                free(bytes);
                yaml_node_free(doc);
                return false;
            }

            if (kind == LIST_ELEM_UINT8)
            {
                if (value > UINT8_MAX)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
                if (ssz_serialize_uint8((uint8_t)value, dst) != SSZ_SUCCESS)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
            }
            else if (kind == LIST_ELEM_UINT16)
            {
                if (value > UINT16_MAX)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
                if (ssz_serialize_uint16((uint16_t)value, dst) != SSZ_SUCCESS)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
            }
            else if (kind == LIST_ELEM_UINT32)
            {
                if (value > UINT32_MAX)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
                if (ssz_serialize_uint32((uint32_t)value, dst) != SSZ_SUCCESS)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
            }
            else
            {
                if (ssz_serialize_uint64(value, dst) != SSZ_SUCCESS)
                {
                    free(bytes);
                    yaml_node_free(doc);
                    return false;
                }
            }

            continue;
        }

        if (!yaml_node_parse_decimal_le(item, dst, elem_size))
        {
            free(bytes);
            yaml_node_free(doc);
            return false;
        }
    }

    yaml_node_free(doc);
    *out_bytes = bytes;
    *out_len = total_len;
    *out_count = (uint64_t)count;
    return true;
}

static ssz_error_t deserialize_progressive_list_checked(
    list_elem_kind_t kind,
    const uint8_t *serialized,
    size_t serialized_len,
    size_t elem_size,
    uint8_t *out,
    size_t out_cap,
    uint64_t *out_count)
{
    ssz_error_t err;

    if ((serialized == NULL && serialized_len != 0u) || (out == NULL) || (out_count == NULL) ||
        (elem_size == 0u))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    err = ssz_deserialize_list_fixed(
        serialized,
        serialized_len,
        SSZ_NO_LIMIT,
        elem_size,
        out,
        out_cap,
        out_count);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }

    if (kind == LIST_ELEM_BOOL)
    {
        for (size_t i = 0u; i < (size_t)(*out_count); i++)
        {
            uint8_t value = 0u;
            err = ssz_deserialize_boolean(out + i, &value);
            if (err != SSZ_SUCCESS)
            {
                return err;
            }
        }
    }

    return SSZ_SUCCESS;
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    list_elem_kind_t kind;
    size_t elem_size = 0u;
    uint8_t *expected = NULL;
    size_t expected_len = 0u;
    uint64_t expected_count = 0u;
    uint8_t *decoded = NULL;
    uint64_t decoded_count = 0u;
    size_t reencoded_len = 0u;
    uint8_t *reencoded = NULL;
    uint8_t expected_root[32];
    ssz_chunk_t actual_root;

    report->total_valid++;

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

    if (!parse_case_descriptor(case_name, &kind, &elem_size))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse progressive list descriptor");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    if (!load_expected_list(
            value_path,
            kind,
            elem_size,
            &expected,
            &expected_len,
            &expected_count))
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

    decoded = (uint8_t *)malloc(expected_len == 0u ? 1u : expected_len);
    if (decoded == NULL)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "failed to allocate decoded buffer");
        goto done;
    }

    if (deserialize_progressive_list_checked(
            kind,
            serialized,
            serialized_len,
            elem_size,
            decoded,
            expected_len == 0u ? 1u : expected_len,
            &decoded_count) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization failed");
        goto done;
    }

    if (decoded_count != expected_count)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialized element count mismatch");
        goto done;
    }

    if ((expected_len != 0u) && (memcmp(decoded, expected, expected_len) != 0))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialized value does not match value.yaml");
        goto done;
    }

    if (ssz_serialize_list_fixed(decoded, decoded_count, SSZ_NO_LIMIT, elem_size, NULL, 0u,
            &reencoded_len) != SSZ_SUCCESS)
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

    if (ssz_serialize_list_fixed(decoded, decoded_count, SSZ_NO_LIMIT, elem_size, reencoded,
            reencoded_len, &reencoded_len) != SSZ_SUCCESS)
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

    if (ssz_hash_tree_root_progressive_list_fixed(decoded, decoded_count, elem_size, ssz_hash_default(),
            &actual_root) != SSZ_SUCCESS)
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
    free(expected);
    free(decoded);
    free(reencoded);
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    list_elem_kind_t kind;
    size_t elem_size = 0u;
    uint8_t *decoded = NULL;
    uint64_t decoded_count = 0u;
    ssz_error_t err;

    report->total_invalid++;

    case_path = spec_join_path(suite_dir, case_name);
    serialized_path = spec_join_path(case_path, "serialized.ssz_snappy");

    if ((case_path == NULL) || (serialized_path == NULL))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_name, "failed to allocate case paths");
        goto done;
    }

    if (!parse_case_descriptor(case_name, &kind, &elem_size))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not parse progressive list descriptor");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    decoded = (uint8_t *)malloc(serialized_len == 0u ? 1u : serialized_len);
    if (decoded == NULL)
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "failed to allocate decoded buffer");
        goto done;
    }

    err = deserialize_progressive_list_checked(
        kind,
        serialized,
        serialized_len,
        elem_size,
        decoded,
        serialized_len == 0u ? 1u : serialized_len,
        &decoded_count);
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
    free(decoded);
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
        int rc = spec_report_print("ssz_generic/basic_progressive_list", &report);
        spec_report_free(&report);
        return rc;
    }
}
