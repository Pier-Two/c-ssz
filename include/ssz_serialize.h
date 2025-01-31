#ifndef SSZ_SERIALIZE_H
#define SSZ_SERIALIZE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssz_types.h"

/**
 * Serializes an 8-bit unsigned integer into a single byte.
 *
 * @param value Pointer to the 8-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint8(const void *value, uint8_t *out_buf, size_t *out_size);

/**
 * Serializes a 16-bit unsigned integer into two bytes.
 *
 * @param value Pointer to the 16-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint16(const void *value, uint8_t *out_buf, size_t *out_size);

/**
 * Serializes a 32-bit unsigned integer into four bytes.
 *
 * @param value Pointer to the 32-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint32(const void *value, uint8_t *out_buf, size_t *out_size);

/**
 * Serializes a 64-bit unsigned integer into eight bytes.
 *
 * @param value Pointer to the 64-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint64(const void *value, uint8_t *out_buf, size_t *out_size);

/**
 * Serializes a 128-bit unsigned integer into sixteen bytes.
 *
 * @param value Pointer to the 128-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint128(const void *value, uint8_t *out_buf, size_t *out_size);

/**
 * Serializes a 256-bit unsigned integer into thirty-two bytes.
 *
 * @param value Pointer to the 256-bit unsigned integer to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_uint256(const void *value, uint8_t *out_buf, size_t *out_size);

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
 * Serializes a bitvector into a compact byte array. 
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
 * Serializes a bitlist into a compact byte array. 
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
 * Serializes a vector of uint8 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint8(
    const uint8_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of uint16 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint16(
    const uint16_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of uint32 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint32(
    const uint32_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of uint64 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint64(
    const uint64_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of uint128 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint128(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of uint256 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_uint256(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a vector of bool elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the vector.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_vector_bool(
    const bool *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint8 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint8(
    const uint8_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint16 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint16(
    const uint16_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint32 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint32(
    const uint32_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint64 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint64(
    const uint64_t *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint128 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint128(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of uint256 elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_uint256(
    const void *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

/**
 * Serializes a list of bool elements into consecutive bytes.
 *
 * @param elements Pointer to the elements.
 * @param element_count The number of elements in the list.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
ssz_error_t ssz_serialize_list_bool(
    const bool *restrict elements,
    size_t element_count,
    uint8_t *restrict out_buf,
    size_t *restrict out_size
);

#endif /* SSZ_SERIALIZE_H */