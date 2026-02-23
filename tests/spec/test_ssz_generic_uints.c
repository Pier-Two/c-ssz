#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec/spec_common.h"
#include "ssz.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/uints"
#endif

static bool parse_case_bits(const char *case_name, unsigned int *out_bits)
{
    return (case_name != NULL) && (out_bits != NULL) && (sscanf(case_name, "uint_%u", out_bits) == 1);
}

static ssz_error_t deserialize_uint_checked(
    unsigned int bits,
    const uint8_t *serialized,
    size_t serialized_len,
    uint64_t *out_u64,
    uint8_t out_wide[32])
{
    if ((serialized == NULL) || (out_u64 == NULL) || (out_wide == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (bits == 8u)
    {
        uint8_t value = 0u;
        if (serialized_len != 1u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        if (ssz_deserialize_uint8(serialized, &value) != SSZ_SUCCESS)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = value;
        memset(out_wide, 0, 32u);
        out_wide[0] = value;
        return SSZ_SUCCESS;
    }

    if (bits == 16u)
    {
        uint16_t value = 0u;
        if (serialized_len != 2u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        if (ssz_deserialize_uint16(serialized, &value) != SSZ_SUCCESS)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = value;
        memset(out_wide, 0, 32u);
        out_wide[0] = serialized[0];
        out_wide[1] = serialized[1];
        return SSZ_SUCCESS;
    }

    if (bits == 32u)
    {
        uint32_t value = 0u;
        if (serialized_len != 4u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        if (ssz_deserialize_uint32(serialized, &value) != SSZ_SUCCESS)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = value;
        memset(out_wide, 0, 32u);
        memcpy(out_wide, serialized, 4u);
        return SSZ_SUCCESS;
    }

    if (bits == 64u)
    {
        uint64_t value = 0u;
        if (serialized_len != 8u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        if (ssz_deserialize_uint64(serialized, &value) != SSZ_SUCCESS)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = value;
        memset(out_wide, 0, 32u);
        memcpy(out_wide, serialized, 8u);
        return SSZ_SUCCESS;
    }

    if (bits == 128u)
    {
        if (serialized_len != 16u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = 0u;
        memset(out_wide, 0, 32u);
        return ssz_deserialize_uint128(serialized, out_wide);
    }

    if (bits == 256u)
    {
        if (serialized_len != 32u)
        {
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_u64 = 0u;
        memset(out_wide, 0, 32u);
        return ssz_deserialize_uint256(serialized, out_wide);
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

static ssz_error_t serialize_uint(
    unsigned int bits,
    uint64_t value_u64,
    const uint8_t wide[32],
    uint8_t **out_bytes,
    size_t *out_len)
{
    uint8_t *bytes;

    if ((wide == NULL) || (out_bytes == NULL) || (out_len == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    *out_bytes = NULL;
    *out_len = 0u;

    if (bits == 8u)
    {
        bytes = (uint8_t *)malloc(1u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint8((uint8_t)value_u64, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 1u;
        return SSZ_SUCCESS;
    }

    if (bits == 16u)
    {
        bytes = (uint8_t *)malloc(2u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint16((uint16_t)value_u64, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 2u;
        return SSZ_SUCCESS;
    }

    if (bits == 32u)
    {
        bytes = (uint8_t *)malloc(4u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint32((uint32_t)value_u64, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 4u;
        return SSZ_SUCCESS;
    }

    if (bits == 64u)
    {
        bytes = (uint8_t *)malloc(8u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint64(value_u64, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 8u;
        return SSZ_SUCCESS;
    }

    if (bits == 128u)
    {
        bytes = (uint8_t *)malloc(16u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint128(wide, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 16u;
        return SSZ_SUCCESS;
    }

    if (bits == 256u)
    {
        bytes = (uint8_t *)malloc(32u);
        if (bytes == NULL)
        {
            return SSZ_ERR_INVALID_ARGUMENT;
        }
        if (ssz_serialize_uint256(wide, bytes) != SSZ_SUCCESS)
        {
            free(bytes);
            return SSZ_ERR_ENCODING_INVALID;
        }
        *out_bytes = bytes;
        *out_len = 32u;
        return SSZ_SUCCESS;
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

static ssz_error_t root_uint(unsigned int bits, uint64_t value_u64, const uint8_t wide[32], ssz_chunk_t *out)
{
    if ((wide == NULL) || (out == NULL))
    {
        return SSZ_ERR_INVALID_ARGUMENT;
    }

    if (bits == 8u)
    {
        return ssz_hash_tree_root_uint8((uint8_t)value_u64, out);
    }
    if (bits == 16u)
    {
        return ssz_hash_tree_root_uint16((uint16_t)value_u64, out);
    }
    if (bits == 32u)
    {
        return ssz_hash_tree_root_uint32((uint32_t)value_u64, out);
    }
    if (bits == 64u)
    {
        return ssz_hash_tree_root_uint64(value_u64, out);
    }
    if (bits == 128u)
    {
        return ssz_hash_tree_root_uint128(wide, out);
    }
    if (bits == 256u)
    {
        return ssz_hash_tree_root_uint256(wide, out);
    }

    return SSZ_ERR_SCHEMA_INVALID;
}

static bool load_expected_value(const char *value_path, unsigned int bits, uint64_t *out_u64, uint8_t out_wide[32])
{
    yaml_node_t *doc;

    if ((value_path == NULL) || (out_u64 == NULL) || (out_wide == NULL))
    {
        return false;
    }

    doc = yaml_parse_file(value_path);
    if (doc == NULL)
    {
        return false;
    }

    memset(out_wide, 0, 32u);
    *out_u64 = 0u;

    if (bits <= 64u)
    {
        uint64_t parsed = 0u;
        if (!yaml_node_parse_u64(doc, &parsed))
        {
            yaml_node_free(doc);
            return false;
        }
        *out_u64 = parsed;
    }
    else
    {
        if (!yaml_node_parse_decimal_le(doc, out_wide, bits / 8u))
        {
            yaml_node_free(doc);
            return false;
        }
    }

    yaml_node_free(doc);
    return true;
}

static bool values_match(unsigned int bits, uint64_t actual_u64, const uint8_t actual_wide[32],
    uint64_t expected_u64, const uint8_t expected_wide[32])
{
    if (bits <= 64u)
    {
        return actual_u64 == expected_u64;
    }

    return memcmp(actual_wide, expected_wide, bits / 8u) == 0;
}

static void run_valid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    char *value_path = NULL;
    char *meta_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    unsigned int bits = 0u;
    uint64_t expected_u64 = 0u;
    uint8_t expected_wide[32];
    uint64_t actual_u64 = 0u;
    uint8_t actual_wide[32];
    uint8_t expected_root[32];
    uint8_t *reencoded = NULL;
    size_t reencoded_len = 0u;
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

    if (!parse_case_bits(case_name, &bits))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not parse uint size from case name");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    if (!load_expected_value(value_path, bits, &expected_u64, expected_wide))
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

    if (deserialize_uint_checked(bits, serialized, serialized_len, &actual_u64, actual_wide) != SSZ_SUCCESS)
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialization failed");
        goto done;
    }

    if (!values_match(bits, actual_u64, actual_wide, expected_u64, expected_wide))
    {
        report->valid_failed++;
        spec_report_record_failure(report, case_path, "deserialized value does not match value.yaml");
        goto done;
    }

    if (serialize_uint(bits, actual_u64, actual_wide, &reencoded, &reencoded_len) != SSZ_SUCCESS)
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

    if (root_uint(bits, actual_u64, actual_wide, &actual_root) != SSZ_SUCCESS)
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
    free(reencoded);
}

static void run_invalid_case(spec_report_t *report, const char *suite_dir, const char *case_name)
{
    char *case_path = NULL;
    char *serialized_path = NULL;
    uint8_t *serialized = NULL;
    size_t serialized_len = 0u;
    unsigned int bits = 0u;
    uint64_t out_u64 = 0u;
    uint8_t out_wide[32];
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

    if (!parse_case_bits(case_name, &bits))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not parse uint size from case name");
        goto done;
    }

    if (!spec_read_snappy_file(serialized_path, &serialized, &serialized_len))
    {
        report->invalid_failed++;
        spec_report_record_failure(report, case_path, "could not decode serialized.ssz_snappy");
        goto done;
    }

    err = deserialize_uint_checked(bits, serialized, serialized_len, &out_u64, out_wide);
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
        int rc = spec_report_print("ssz_generic/uints", &report);
        spec_report_free(&report);
        return rc;
    }
}
