#ifndef SSZ_DESERIALIZE_H
#define SSZ_DESERIALIZE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Deserializes an 8-bit unsigned integer from a single byte.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 8-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint8(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a 16-bit unsigned integer from two bytes.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 16-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint16(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a 32-bit unsigned integer from four bytes.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 32-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint32(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a 64-bit unsigned integer from eight bytes.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 64-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint64(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a 128-bit unsigned integer from sixteen bytes.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 128-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint128(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a 256-bit unsigned integer from thirty-two bytes.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param out_value Pointer to store the deserialized 256-bit value.
 * @return SSZ_SUCCESS on success, SSZ_ERROR_DESERIALIZATION on failure.
 */
ssz_error_t ssz_deserialize_uint256(const uint8_t *buffer, size_t buffer_size, void *out_value);

/**
 * Deserializes a boolean value from a single byte in the buffer.
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
 * Deserializes a bitlist from the buffer. 
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
 * Deserializes a union type from the buffer. 
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
 * Deserializes a fixed-size vector of 8-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 8-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint8(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    uint8_t *out_elements
);

/**
 * Deserializes a fixed-size vector of 16-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 16-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint16(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    uint16_t *out_elements
);

/**
 * Deserializes a fixed-size vector of 32-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 32-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint32(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    uint32_t *out_elements
);

/**
 * Deserializes a fixed-size vector of 64-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 64-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint64(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    uint64_t *out_elements
);

/**
 * Deserializes a fixed-size vector of 128-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 128-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint128(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    void *out_elements
);

/**
 * Deserializes a fixed-size vector of 256-bit elements.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of elements in the vector.
 * @param out_elements Pointer to store the deserialized 256-bit elements.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_uint256(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    void *out_elements
);

/**
 * Deserializes a fixed-size vector of booleans.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size Size of the input buffer in bytes.
 * @param element_count The number of boolean elements in the vector.
 * @param out_elements Pointer to store the deserialized boolean values.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_vector_bool(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t element_count,
    bool *out_elements
);

/**
 * Deserializes a list of 8-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 8-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint8(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    uint8_t *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of 16-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 16-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint16(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    uint16_t *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of 32-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 32-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint32(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    uint32_t *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of 64-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 64-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint64(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    uint64_t *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of 128-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 128-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint128(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    void *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of 256-bit unsigned integers from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized 256-bit elements.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_uint256(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    void *out_elements,
    size_t *out_actual_count
);

/**
 * Deserializes a list of boolean values from the buffer.
 *
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param max_length The maximum number of elements allowed in the list.
 * @param out_elements Pointer to store the deserialized boolean values.
 * @param out_actual_count Pointer to store the actual number of elements deserialized.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_deserialize_list_bool(
    const uint8_t *buffer,
    size_t buffer_size,
    size_t max_length,
    bool *out_elements,
    size_t *out_actual_count
);

#endif /* SSZ_DESERIALIZE_H */