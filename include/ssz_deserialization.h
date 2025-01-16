#ifndef SSZ_DESERIALIZATION_H
#define SSZ_DESERIALIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Deserializes an unsigned integer of a specified bit size from a buffer.
 * Supports bit sizes of up to 64 bits. The deserialized value is stored
 * in the provided output pointer.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param bit_size The size of the integer in bits.
 * @param out_value Pointer to store the deserialized integer value.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_uintN(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t bit_size,
    void *out_value
);

/**
 * Deserializes a boolean value from a single byte in the buffer.
 * A value of 0x00 is interpreted as false, and 0x01 as true.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized boolean value.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_boolean(
    const uint8_t *buffer,
    size_t buffer_size,
    bool *out_value
);

/**
 * Deserializes a bitvector of a specified length from the buffer.
 * The bitvector must have exactly num_bits length, and the deserialized
 * bits are stored in the provided output array.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param num_bits The number of bits in the bitvector.
 * @param out_bits Pointer to store the deserialized bits.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_bitvector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t num_bits,
    bool *out_bits
);

/**
 * Deserializes a bitlist from the buffer. A bitlist is a variable-length
 * collection of bits, with a boundary bit indicating the end of the list.
 * The deserialized bits are stored in the provided output array, and the
 * actual number of bits is returned.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_bits The maximum number of bits in the bitlist.
 * @param out_bits Pointer to store the deserialized bits.
 * @param out_actual_bits Pointer to store the actual number of bits deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_bitlist(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_bits,
    bool *out_bits,
    size_t *out_actual_bits
);

/**
 * Deserializes a union type from the buffer. The union includes a selector
 * and optional data, which is deserialized using the provided output structure.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param out_union Pointer to the structure to store the deserialized union.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_union(
    const uint8_t *buffer,
    size_t buffer_size,
    ssz_union_t *out_union
);

/**
 * Deserializes a vector of elements from the buffer. A vector is a fixed-length
 * collection of elements. If the elements are variable-sized, offsets are read
 * from the buffer to locate each element's data slice.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param field_sizes Array of sizes for each element.
 * @param is_variable_size Whether the elements are variable-sized.
 * @param out_elements Pointer to store the deserialized elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    const size_t *field_sizes,
    bool is_variable_size,
    void *out_elements
);

/**
 * Deserializes a list of elements from the buffer. A list is a variable-length
 * collection of elements, which can be zero-length. If the elements are
 * variable-sized, offsets are read from the buffer to locate each element's
 * data slice. The actual number of elements deserialized is returned.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param element_count The maximum number of elements in the list.
 * @param field_sizes Array of sizes for each element.
 * @param is_variable_size Whether the elements are variable-sized.
 * @param out_elements Pointer to store the deserialized elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
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

#endif /* SSZ_DESERIALIZATION_H */
