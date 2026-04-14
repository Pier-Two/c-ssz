#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "spec/ssz_static_runtime.h"
#include "ssz.h"

#ifndef SSZ_STATIC_FIXTURES_DIR
#define SSZ_STATIC_FIXTURES_DIR "tests/fixtures"
#endif

static bool parse_root_yaml(const char *path, uint8_t out_root[32])
{
    bool ok = false;
    yaml_node_t *doc = NULL;
    const yaml_node_t *root_node = NULL;
    uint8_t *parsed = NULL;
    size_t parsed_len = 0u;

    if ((path == NULL) || (out_root == NULL))
    {
        return false;
    }

    doc = yaml_parse_file(path);
    if ((doc != NULL) && (doc->type == YAML_NODE_MAPPING))
    {
        root_node = yaml_mapping_get(doc, "root");
        if (yaml_node_parse_hex(root_node, &parsed, &parsed_len) && (parsed_len == 32u))
        {
            (void)memcpy(out_root, parsed, 32u);
            ok = true;
        }
    }

    free(parsed);
    yaml_node_free(doc);
    return ok;
}

static bool suite_is_invalid(const char *suite_name)
{
    return (suite_name != NULL) && (strstr(suite_name, "invalid") != NULL);
}

static bool matches_filter(const char *filter_name, const char *value)
{
    const char *filter = getenv(filter_name);
    return (filter == NULL) || ((value != NULL) && (strcmp(filter, value) == 0));
}

static size_t first_mismatch_offset(const uint8_t *left, const uint8_t *right, size_t len)
{
    for (size_t i = 0u; i < len; i++)
    {
        if (left[i] != right[i])
        {
            return i;
        }
    }

    return len;
}

static void record_missing_schema(spec_report_t *report, const char *type_path)
{
    spec_report_record_failure(report, type_path, "missing generated schema entry");
}

static void run_valid_case(
    spec_report_t *report,
    const ssz_static_schema_type_t *type,
    const char *case_path)
{
    char *serialized_path = NULL;
    char *roots_path = NULL;
    uint8_t *decoded = NULL;
    size_t decoded_len = 0u;
    uint8_t expected_root_bytes[32];
    ssz_static_computed_value_t computed = {0};
    ssz_error_t err = SSZ_SUCCESS;

    if ((report == NULL) || (type == NULL) || (case_path == NULL))
    {
        return;
    }

    report->total_valid++;

    serialized_path = spec_join_path(case_path, "serialized.ssz_snappy");
    roots_path = spec_join_path(case_path, "roots.yaml");
    if ((serialized_path == NULL) || (roots_path == NULL))
    {
        spec_report_record_failure(report, case_path, "failed to build case paths");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &decoded, &decoded_len))
    {
        spec_report_record_failure(report, case_path, "failed to read serialized.ssz_snappy");
        goto done;
    }
    if (!parse_root_yaml(roots_path, expected_root_bytes))
    {
        spec_report_record_failure(report, case_path, "failed to parse roots.yaml");
        goto done;
    }

    err = ssz_static_compute_from_bytes(type, decoded, decoded_len, &computed);
    if (err != SSZ_SUCCESS)
    {
        char reason[96];
        (void)snprintf(
            reason,
            sizeof(reason),
            "deserialize/root failed: %s (%d)",
            ssz_error_string(err),
            (int)err);
        spec_report_record_failure(report, case_path, reason);
        goto done;
    }

    if (memcmp(computed.root.bytes, expected_root_bytes, sizeof(expected_root_bytes)) != 0)
    {
        spec_report_record_failure(report, case_path, "hash_tree_root mismatch");
        goto done;
    }
    if (computed.len != decoded_len)
    {
        char reason[96];
        (void)snprintf(
            reason,
            sizeof(reason),
            "round-trip re-serialize length mismatch: expected %zu got %zu",
            decoded_len,
            computed.len);
        spec_report_record_failure(report, case_path, reason);
        goto done;
    }
    if ((decoded_len != 0u) && (memcmp(computed.bytes, decoded, decoded_len) != 0))
    {
        size_t offset = first_mismatch_offset(computed.bytes, decoded, decoded_len);
        char reason[96];
        (void)snprintf(
            reason,
            sizeof(reason),
            "round-trip re-serialize mismatch at offset %zu",
            offset);
        spec_report_record_failure(report, case_path, reason);
        goto done;
    }

    report->valid_passed++;

done:
    if (report->valid_passed < report->total_valid)
    {
        report->valid_failed = report->total_valid - report->valid_passed;
    }

    free(decoded);
    free(serialized_path);
    free(roots_path);
    ssz_static_computed_value_reset(&computed);
}

static void run_invalid_case(
    spec_report_t *report,
    const ssz_static_schema_type_t *type,
    const char *case_path)
{
    char *serialized_path = NULL;
    uint8_t *decoded = NULL;
    size_t decoded_len = 0u;
    ssz_chunk_t root;
    ssz_error_t err = SSZ_SUCCESS;

    if ((report == NULL) || (type == NULL) || (case_path == NULL))
    {
        return;
    }

    report->total_invalid++;

    serialized_path = spec_join_path(case_path, "serialized.ssz_snappy");
    if (serialized_path == NULL)
    {
        spec_report_record_failure(report, case_path, "failed to build serialized path");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &decoded, &decoded_len))
    {
        spec_report_record_failure(report, case_path, "failed to read serialized.ssz_snappy");
        goto done;
    }

    err = ssz_static_validate_and_root(type, decoded, decoded_len, &root);
    if (err == SSZ_SUCCESS)
    {
        spec_report_record_failure(report, case_path, "invalid case decoded successfully");
        goto done;
    }

    report->invalid_passed++;

done:
    if (report->invalid_passed < report->total_invalid)
    {
        report->invalid_failed = report->total_invalid - report->invalid_passed;
    }

    free(decoded);
    free(serialized_path);
}

static void run_suite(
    spec_report_t *report,
    const ssz_static_schema_type_t *type,
    const char *suite_path,
    const char *suite_name)
{
    char **case_names = NULL;
    size_t case_count = 0u;
    bool invalid_suite = suite_is_invalid(suite_name);

    if ((report == NULL) || (type == NULL) || (suite_path == NULL) || (suite_name == NULL))
    {
        return;
    }

    if (!spec_list_subdirs(suite_path, &case_names, &case_count))
    {
        spec_report_record_failure(report, suite_path, "failed to list case directories");
        return;
    }

    for (size_t i = 0u; i < case_count; i++)
    {
        char *case_path = spec_join_path(suite_path, case_names[i]);

        if (case_path == NULL)
        {
            spec_report_record_failure(report, suite_path, "failed to build case path");
        }
        else if (invalid_suite)
        {
            run_invalid_case(report, type, case_path);
        }
        else
        {
            run_valid_case(report, type, case_path);
        }

        free(case_path);
    }

    spec_free_string_array(case_names, case_count);
}

static void run_handler(
    spec_report_t *report,
    const char *preset,
    const char *fork,
    const char *ssz_static_path,
    const char *handler_name)
{
    char **suite_names = NULL;
    size_t suite_count = 0u;
    char *handler_path = NULL;
    const ssz_static_schema_type_t *type = NULL;

    if ((report == NULL) || (preset == NULL) || (fork == NULL) || (ssz_static_path == NULL) ||
        (handler_name == NULL))
    {
        return;
    }
    if (!matches_filter("SSZ_STATIC_HANDLER", handler_name))
    {
        return;
    }

    handler_path = spec_join_path(ssz_static_path, handler_name);
    if (handler_path == NULL)
    {
        spec_report_record_failure(report, ssz_static_path, "failed to build handler path");
        return;
    }

    type = ssz_static_find_root_type(preset, fork, handler_name);
    if (type == NULL)
    {
        record_missing_schema(report, handler_path);
        free(handler_path);
        return;
    }

    if (!spec_list_subdirs(handler_path, &suite_names, &suite_count))
    {
        spec_report_record_failure(report, handler_path, "failed to list suites");
        free(handler_path);
        return;
    }

    for (size_t i = 0u; i < suite_count; i++)
    {
        char *suite_path = spec_join_path(handler_path, suite_names[i]);

        if (suite_path == NULL)
        {
            spec_report_record_failure(report, handler_path, "failed to build suite path");
        }
        else
        {
            run_suite(report, type, suite_path, suite_names[i]);
        }

        free(suite_path);
    }

    spec_free_string_array(suite_names, suite_count);
    free(handler_path);
}

static void run_fork(
    spec_report_t *report,
    const char *preset_path,
    const char *preset_name,
    const char *fork_name)
{
    char *fork_path = NULL;
    char *ssz_static_path = NULL;
    char **handler_names = NULL;
    size_t handler_count = 0u;

    if ((report == NULL) || (preset_path == NULL) || (preset_name == NULL) || (fork_name == NULL))
    {
        return;
    }
    if (!matches_filter("SSZ_STATIC_FORK", fork_name))
    {
        return;
    }

    fork_path = spec_join_path(preset_path, fork_name);
    ssz_static_path = spec_join_path(fork_path, "ssz_static");
    if ((fork_path == NULL) || (ssz_static_path == NULL))
    {
        spec_report_record_failure(report, preset_name, "failed to build fork paths");
        goto done;
    }

    if (!spec_list_subdirs(ssz_static_path, &handler_names, &handler_count))
    {
        goto done;
    }

    for (size_t i = 0u; i < handler_count; i++)
    {
        run_handler(report, preset_name, fork_name, ssz_static_path, handler_names[i]);
    }

done:
    spec_free_string_array(handler_names, handler_count);
    free(ssz_static_path);
    free(fork_path);
}

static void run_preset(spec_report_t *report, const char *fixtures_root, const char *preset_name)
{
    char *preset_path = NULL;
    char **fork_names = NULL;
    size_t fork_count = 0u;

    if ((report == NULL) || (fixtures_root == NULL) || (preset_name == NULL))
    {
        return;
    }
    if (!matches_filter("SSZ_STATIC_PRESET", preset_name))
    {
        return;
    }

    preset_path = spec_join_path(fixtures_root, preset_name);
    if (preset_path == NULL)
    {
        spec_report_record_failure(report, fixtures_root, "failed to build preset path");
        return;
    }

    if (!spec_list_subdirs(preset_path, &fork_names, &fork_count))
    {
        free(preset_path);
        return;
    }

    for (size_t i = 0u; i < fork_count; i++)
    {
        run_fork(report, preset_path, preset_name, fork_names[i]);
    }

    spec_free_string_array(fork_names, fork_count);
    free(preset_path);
}

int main(void)
{
    spec_report_t report;
    char **preset_names = NULL;
    size_t preset_count = 0u;
    int exit_code = 0;

    spec_report_init(&report);

    if (!spec_list_subdirs(SSZ_STATIC_FIXTURES_DIR, &preset_names, &preset_count))
    {
        spec_report_record_failure(&report, SSZ_STATIC_FIXTURES_DIR, "failed to list fixture presets");
        exit_code = spec_report_print("ssz_static", &report);
        spec_report_free(&report);
        return exit_code;
    }

    for (size_t i = 0u; i < preset_count; i++)
    {
        if ((strcmp(preset_names[i], "mainnet") == 0) || (strcmp(preset_names[i], "minimal") == 0))
        {
            run_preset(&report, SSZ_STATIC_FIXTURES_DIR, preset_names[i]);
        }
    }

    exit_code = spec_report_print("ssz_static", &report);

    spec_free_string_array(preset_names, preset_count);
    spec_report_free(&report);
    return exit_code;
}
