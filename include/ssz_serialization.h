#ifndef SSZ_SERIALIZATION_H
#define SSZ_SERIALIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../include/ssz_types.h"

// Serializes a uintN (8,16,32,64,128,256 bits). 
// 'bit_size' is N, 'value' is a pointer to the integer, and 'out_buf' is the destination.
ssz_error_t ssz_serialize_uintN(
    const void *value,
    size_t bit_size,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a boolean to a single byte of 0x00 or 0x01.
ssz_error_t ssz_serialize_boolean(
    bool value,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a bitvector of fixed length 'num_bits'.
ssz_error_t ssz_serialize_bitvector(
    const bool *bits,
    size_t num_bits,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a bitlist of length 'num_bits' (plus boundary bit).
ssz_error_t ssz_serialize_bitlist(
    const bool *bits,
    size_t num_bits,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a union by writing the selector byte and 
// optionally the serialized data for that union variant.
ssz_error_t ssz_serialize_union(
    const ssz_union_t *u,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a vector of elements that may be either fixed or variable size. 
// An empty vector is invalid by SSZ rules, so this returns an error if 'element_count' is zero.
ssz_error_t ssz_serialize_vector(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size
);

// Serializes a list of elements that may be either fixed or variable size. 
// An empty list is valid, so this can produce zero bytes of output if 'element_count' is zero.
ssz_error_t ssz_serialize_list(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size
);

#endif /* SSZ_SERIALIZATION_H */
