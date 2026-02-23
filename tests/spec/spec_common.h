#ifndef TESTS_SPEC_COMMON_H
#define TESTS_SPEC_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "snappy_decode.h"
#include "yaml_parser.h"

typedef struct
{
    size_t total_valid;
    size_t valid_passed;
    size_t valid_failed;
    size_t total_invalid;
    size_t invalid_passed;
    size_t invalid_failed;
    char **failure_messages;
    size_t failure_count;
} spec_report_t;

void spec_report_init(spec_report_t *report);
void spec_report_free(spec_report_t *report);
void spec_report_record_failure(spec_report_t *report, const char *case_path, const char *reason);
int spec_report_print(const char *label, const spec_report_t *report);

bool spec_read_binary_file(const char *path, uint8_t **out_bytes, size_t *out_len);
bool spec_read_snappy_file(const char *path, uint8_t **out_bytes, size_t *out_len);

bool spec_list_subdirs(const char *dir_path, char ***out_names, size_t *out_count);
void spec_free_string_array(char **items, size_t count);

char *spec_join_path(const char *base, const char *name);

bool spec_parse_root_from_meta(const char *meta_path, uint8_t out_root[32]);

#endif
