#ifndef TESTS_SPEC_SSZ_STATIC_RUNTIME_H
#define TESTS_SPEC_SSZ_STATIC_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#include "ssz_static_schema_generated.h"
#include "ssz.h"

typedef struct
{
    const ssz_static_schema_type_t *type;
    uint8_t *bytes;
    size_t len;
    ssz_chunk_t root;
} ssz_static_computed_value_t;

const ssz_static_schema_type_t *ssz_static_find_root_type(
    const char *preset,
    const char *fork,
    const char *handler);

void ssz_static_computed_value_reset(ssz_static_computed_value_t *value);

ssz_error_t ssz_static_compute_from_bytes(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_static_computed_value_t *out_value);

ssz_error_t ssz_static_validate_and_root(
    const ssz_static_schema_type_t *type,
    const uint8_t *bytes,
    size_t byte_len,
    ssz_chunk_t *out_root);

#endif
