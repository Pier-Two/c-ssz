#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/boolean"
#endif

static bool load_expected_boolean(const char *value_path, uint8_t *out_value)
{
    yaml_node_t *doc;

    if ((value_path == NULL) || (out_value == NULL))
    {
        return false;
    }

    doc = yaml_parse_file(value_path);
    if ((doc == NULL) || (doc->type != YAML_NODE_BOOL))
    {
        yaml_node_free(doc);
        return false;
    }

    *out_value = doc->as.bool_value ? 1u : 0u;
    yaml_node_free(doc);
    return true;
}

static ssz_error_t deserialize_boolean_checked(const uint8_t *bytes, size_t len, uint8_t *out)
{
    if ((bytes == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }
    if (len != 1u)
    {
        return SSZ_ERR_ENCODING_INVALID;
    }
    return ssz_deserialize_boolean(bytes, len, out);
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    uint8_t expected_value = 0u;
    uint8_t actual_value = 0u;
    uint8_t expected_root[32];
    uint8_t reencoded[1];
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

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    if (!load_expected_boolean(value_path, &expected_value))
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

    if (deserialize_boolean_checked(serialized, serialized_len, &actual_value) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization failed");
        goto done;
    }

    if (actual_value != expected_value)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialized value does not match value.yaml");
        goto done;
    }

    if (ssz_serialize_boolean(actual_value, reencoded) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "serialization failed");
        goto done;
    }

    if ((serialized_len != 1u) || (reencoded[0] != serialized[0]))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "round-trip serialization mismatch");
        goto done;
    }

    if (ssz_hash_tree_root_boolean(actual_value, &actual_root) != SSZ_SUCCESS)
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
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    uint8_t out_value = 0u;
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

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    err = deserialize_boolean_checked(serialized, serialized_len, &out_value);
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
        int rc = spec_report_print("ssz_generic/boolean", &report);
        spec_report_free(&report);
        return rc;
    }
}
