#ifndef SSZ_SERIALIZE_H
#define SSZ_SERIALIZE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Serializes an unsigned integer of a specified bit size into a buffer.
 * Supports bit sizes of 8, 16, 32, 64, 128, and 256. The serialized value
 * is written in little-endian format.
 * 
 * @param value Pointer to the integer value to serialize.
 * @param bit_size The size of the integer in bits.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uintN(
    const void *value,
    size_t bit_size,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a boolean value into a single byte.
 * 
 * @param value The boolean value to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_boolean(
    bool value,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a bitvector into a compact byte array. Each bit in the input
 * is packed into the output buffer, with unused bits in the last byte set to 0.
 * 
 * @param bits Pointer to the input bit array.
 * @param num_bits The number of bits to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_bitvector(
    const bool *bits,
    size_t num_bits,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a bitlist into a compact byte array. A bitlist is a variable-length
 * collection of bits, with a boundary bit indicating the end of the list.
 * 
 * @param bits Pointer to the input bit array.
 * @param num_bits The number of bits in the bitlist (excluding the boundary bit).
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_bitlist(
    const bool *bits,
    size_t num_bits,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a union by writing the selector byte and optionally the serialized
 * data for the selected union variant.
 * 
 * @param u Pointer to the union to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_union(
    const ssz_union_t *u,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a vector of elements into a buffer. A vector is a fixed-length
 * collection of elements, which may be either fixed-size or variable-size.
 * 
 * @param elements Pointer to the input elements.
 * @param element_count The number of elements in the vector.
 * @param element_sizes Array of sizes for each element.
 * @param is_variable_size Indicates whether the elements are variable-size.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Serializes a list of elements into a buffer. A list is a variable-length
 * collection of elements, which may be either fixed-size or variable-size.
 * 
 * @param elements Pointer to the input elements.
 * @param element_count The number of elements in the list.
 * @param element_sizes Array of sizes for each element.
 * @param is_variable_size Indicates whether the elements are variable-size.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list(
    const void *elements,
    size_t element_count,
    const size_t *element_sizes,
    bool is_variable_size,
    uint8_t *out_buf,
    size_t *out_size
);

#endif /* SSZ_SERIALIZE_H */
