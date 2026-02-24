#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/bitlist"
#endif

static bool parse_case_limit(const char *case_name, uint64_t *out_limit)
{
    unsigned long long parsed = 0ULL;
    if ((case_name == NULL) || (out_limit == NULL))
    {
        return false;
    }
    if (sscanf(case_name, "bitlist_%llu", &parsed) != 1)
    {
        return false;
    }
    *out_limit = (uint64_t)parsed;
    return true;
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

static bool load_expected_hex(const char *value_path, uint8_t **out_bytes, size_t *out_len)
{
    yaml_node_t *doc;
    bool ok;

    if ((value_path == NULL) || (out_bytes == NULL) || (out_len == NULL))
    {
        return false;
    }

    doc = yaml_parse_file(value_path);
    if (doc == NULL)
    {
        return false;
    }

    ok = yaml_node_parse_hex(doc, out_bytes, out_len);
    yaml_node_free(doc);
    return ok;
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    uint8_t *yaml_bytes = NULL;
    size_t yaml_len = 0u;
    uint64_t bit_limit = 0u;
    size_t bitfield_cap = 0u;
    uint8_t *bits = NULL;
    uint64_t bit_len = 0u;
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

    if (!parse_case_limit(case_name, &bit_limit) || !bits_to_bytes(bit_limit, &bitfield_cap))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse bitlist limit");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    if (!load_expected_hex(value_path, &yaml_bytes, &yaml_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse value.yaml");
        goto done;
    }

    if ((yaml_len != serialized_len) || (memcmp(yaml_bytes, serialized, serialized_len) != 0))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "value.yaml does not match serialized data");
        goto done;
    }

    if (!spec_parse_root_from_meta(meta_path, expected_root))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse root from meta.yaml");
        goto done;
    }

    bits = (uint8_t *)malloc(bitfield_cap == 0u ? 1u : bitfield_cap);
    if (bits == NULL)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "failed to allocate bit buffer");
        goto done;
    }

    if (ssz_deserialize_bitlist(serialized, serialized_len, bit_limit, bits, bitfield_cap, &bit_len) !=
        SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization failed");
        goto done;
    }

    if (ssz_serialize_bitlist(bits, bitfield_cap, bit_len, bit_limit, NULL, 0u, &reencoded_len) !=
        SSZ_SUCCESS)
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

    if (ssz_serialize_bitlist(bits, bitfield_cap, bit_len, bit_limit, reencoded, reencoded_len,
            &reencoded_len) != SSZ_SUCCESS)
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

    if (ssz_hash_tree_root_bitlist(bits, bitfield_cap, bit_len, bit_limit, ssz_hash_default(), &actual_root) !=
        SSZ_SUCCESS)
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
    free(yaml_bytes);
    free(bits);
    free(reencoded);
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    uint64_t bit_limit = 0u;
    size_t bitfield_cap = 0u;
    uint8_t *bits = NULL;
    uint64_t bit_len = 0u;
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

    if (!parse_case_limit(case_name, &bit_limit) || !bits_to_bytes(bit_limit, &bitfield_cap))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not parse bitlist limit");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    bits = (uint8_t *)malloc(bitfield_cap == 0u ? 1u : bitfield_cap);
    if (bits == NULL)
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "failed to allocate bit buffer");
        goto done;
    }

    err = ssz_deserialize_bitlist(serialized, serialized_len, bit_limit, bits, bitfield_cap, &bit_len);
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
    free(bits);
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
        int rc = spec_report_print("ssz_generic/bitlist", &report);
        spec_report_free(&report);
        return rc;
    }
}
