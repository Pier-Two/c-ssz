#ifndef SSZ_TYPES_H
#define SSZ_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Enumerates error codes used throughout the library.
 * These codes indicate success or specific types of failures.
 */
typedef enum
{
    SSZ_SUCCESS,                /**< Operation completed successfully. */
    SSZ_ERROR_INVALID_OFFSET,   /**< An invalid offset was encountered. */
    SSZ_ERROR_OUT_OF_RANGE,     /**< A value was out of the acceptable range. */
    SSZ_ERROR_DESERIALIZATION,  /**< An error occurred during deserialization. */
    SSZ_ERROR_SERIALIZATION,    /**< An error occurred during serialization. */
    SSZ_ERROR_MERKLEIZATION     /**< An error occurred during merkleization. */
} ssz_error_t;

/**
 * Defines a function pointer type for serializing union data.
 * 
 * @param data Pointer to the data to serialize.
 * @param out_buf Pointer to the output buffer to write the serialized data.
 * @param out_size Pointer to the size of the output buffer. Updated with the number of bytes written.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
typedef ssz_error_t (*ssz_union_data_serialize_fn)(
    const void *data,
    uint8_t *out_buf,
    size_t *out_size
);

/**
 * Defines a function pointer type for deserializing union data.
 * 
 * @param buffer Pointer to the input buffer containing the serialized data.
 * @param buffer_size The size of the input buffer in bytes.
 * @param out_obj Pointer to store the deserialized object.
 * @return SSZ_SUCCESS on success, or an error code on failure.
 */
typedef ssz_error_t (*ssz_union_data_deserialize_fn)(
    const uint8_t *buffer,
    size_t buffer_size,
    void **out_obj
);

/**
 * Represents a union structure for the SSZ "Union" type.
 * This structure includes a selector (an 8-bit value), a pointer to the associated data,
 * and function pointers for serialization and deserialization of the data.
 */
typedef struct
{
    uint8_t selector;                              /**< The union selector value. */
    void *data;                                    /**< Pointer to the union's data. */
    ssz_union_data_serialize_fn serialize_fn;      /**< Function pointer for serialization. */
    ssz_union_data_deserialize_fn deserialize_fn;  /**< Function pointer for deserialization. */
} ssz_union_t;

#endif /* SSZ_TYPES_H */
