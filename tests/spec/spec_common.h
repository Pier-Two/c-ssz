#ifndef TESTS_SPEC_COMMON_H
#define TESTS_SPEC_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz.h"
#include "snappy_decode.h"
#include "yaml_parser.h"

static ssz_chunk_t g_spec_merkle_scratch_chunks[SSZ_MERKLE_SCRATCH_MAX_CHUNKS];
static const ssz_merkle_scratch_t g_spec_merkle_scratch = {
    .chunks = g_spec_merkle_scratch_chunks,
    .chunk_count = SSZ_MERKLE_SCRATCH_MAX_CHUNKS,
};

#define ssz_hash_tree_root_bitvector(bits_le, bits_le_len, bit_count, hash_fn, out_root)           \
    ssz_hash_tree_root_bitvector(                                                                    \
        (bits_le),                                                                                   \
        (bits_le_len),                                                                               \
        (bit_count),                                                                                 \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_bitlist(bits_le, bits_le_len, bit_len, bit_limit, hash_fn, out_root)    \
    ssz_hash_tree_root_bitlist(                                                                      \
        (bits_le),                                                                                   \
        (bits_le_len),                                                                               \
        (bit_len),                                                                                   \
        (bit_limit),                                                                                 \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_vector_fixed(elements, element_count, element_size, hash_fn, out_root)   \
    ssz_hash_tree_root_vector_fixed(                                                                 \
        (elements),                                                                                  \
        (element_count),                                                                             \
        (element_size),                                                                              \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_vector_composite(element_count, codec, hash_fn, out_root)                \
    ssz_hash_tree_root_vector_composite(                                                             \
        (element_count),                                                                             \
        (codec),                                                                                     \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_list_fixed(elements, element_count, element_limit, element_size, hash_fn, out_root) \
    ssz_hash_tree_root_list_fixed(                                                                   \
        (elements),                                                                                  \
        (element_count),                                                                             \
        (element_limit),                                                                             \
        (element_size),                                                                              \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_list_composite(element_count, element_limit, codec, hash_fn, out_root)   \
    ssz_hash_tree_root_list_composite(                                                               \
        (element_count),                                                                             \
        (element_limit),                                                                             \
        (codec),                                                                                     \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_merkleize_progressive(chunks, chunk_count, hash_fn, out_root)                           \
    ssz_merkleize_progressive(                                                                       \
        (chunks),                                                                                    \
        (chunk_count),                                                                               \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_progressive_container(field_count, active_fields, active_fields_len, codec, hash_fn, out_root) \
    ssz_hash_tree_root_progressive_container(                                                        \
        (field_count),                                                                               \
        (active_fields),                                                                             \
        (active_fields_len),                                                                         \
        (codec),                                                                                     \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_progressive_list_fixed(elements, element_count, element_size, hash_fn, out_root) \
    ssz_hash_tree_root_progressive_list_fixed(                                                       \
        (elements),                                                                                  \
        (element_count),                                                                             \
        (element_size),                                                                              \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_progressive_list_composite(element_count, codec, hash_fn, out_root)      \
    ssz_hash_tree_root_progressive_list_composite(                                                   \
        (element_count),                                                                             \
        (codec),                                                                                     \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))
#define ssz_hash_tree_root_progressive_bitlist(bits_le, bits_le_len, bit_len, hash_fn, out_root)   \
    ssz_hash_tree_root_progressive_bitlist(                                                          \
        (bits_le),                                                                                   \
        (bits_le_len),                                                                               \
        (bit_len),                                                                                   \
        &g_spec_merkle_scratch,                                                                      \
        (hash_fn),                                                                                   \
        (out_root))

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
