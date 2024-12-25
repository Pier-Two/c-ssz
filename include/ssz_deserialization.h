#ifndef SSZ_DESERIALIZATION_H
#define SSZ_DESERIALIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../include/ssz_types.h"

// Deserializes a uintN (up to 64 bits).
ssz_error_t ssz_deserialize_uintN(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t bit_size,
    void *out_value
);

// Deserializes a boolean (0x00 => false, 0x01 => true).
ssz_error_t ssz_deserialize_boolean(
    const uint8_t *buffer,
    size_t buffer_size,
    bool *out_value
);

// Deserializes a bitvector of exactly num_bits length.
ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t num_bits,
    bool *out_bits
);

// Deserializes a bitlist (max_bits). Finds the boundary bit, returns actual bits.
ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_bits,
    bool *out_bits,
    size_t *out_actual_bits
);

// Deserializes a union (placeholder for sub-data).
ssz_error_t ssz_deserialize_union(
    const uint8_t *buffer,
    size_t buffer_size,
    ssz_union_t *out_union
);

/* 
 * Deserializes a vector of elements. Vectors cannot be zero-length. 
 * Uses field_sizes[i] to know how many bytes each element needs for fixed or variable size. 
 * If is_variable_size=false, each element is field_sizes[i] bytes. 
 * If is_variable_size=true, read 4-byte offsets for each element, then copy its data slice. 
 * The number of elements is element_count (must match what’s in the data for variable-size).
 */
ssz_error_t ssz_deserialize_vector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    bool is_variable_size,
    void *out_elements
);

/* 
 * Deserializes a list of elements. Lists can be zero-length, up to element_count. 
 * If is_variable_size=false, each element is field_sizes[i] bytes. 
 * If is_variable_size=true, read 4-byte offsets for each element, then copy its data slice. 
 * On success, out_actual_count is how many elements got parsed.
 */
ssz_error_t ssz_deserialize_list(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    bool is_variable_size,
    void *out_elements,
    size_t *out_actual_count
);

/*
 * Deserializes a container of field_count fields, each with a size in field_sizes[i] 
 * or an offset if variable. Writes data into out_container_data.
 */
ssz_error_t ssz_deserialize_container(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t field_count,
    const bool *field_is_variable_size,
    const size_t *field_sizes,
    void *out_container_data
);

#endif /* SSZ_DESERIALIZATION_H */
