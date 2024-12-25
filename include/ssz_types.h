#ifndef SSZ_TYPES_H
#define SSZ_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// We define a SSZ boolean alias here just for clarity, though we can directly use 'bool' too.
typedef bool ssz_boolean;

// An example of some error codes that we might use in the library.
// We'll keep this simple for now, and we can expand as needed.
typedef enum
{
    SSZ_SUCCESS = 0,
    SSZ_ERROR_INVALID_OFFSET,
    SSZ_ERROR_OUT_OF_RANGE,
    SSZ_ERROR_DESERIALIZATION,
    SSZ_ERROR_SERIALIZATION
} ssz_error_t;

// Define a function pointer type for serializing union data.
typedef ssz_error_t (*ssz_union_data_serialize_fn)(
    const void *data,
    uint8_t *out_buf,
    size_t *out_size
);

typedef ssz_error_t (*ssz_union_data_deserialize_fn)(
    const uint8_t *buffer,
    size_t buffer_size,
    void **out_obj
);

// A union structure is needed for the SSZ "Union" type. We'll store the selector
// (an 8-bit value), a pointer to whatever data we want to keep, and a function pointer
// to handle serialization for that data if the selector != 0.
typedef struct
{
    uint8_t selector;
    void *data;
    ssz_union_data_serialize_fn serialize_fn;   // For serialization
    ssz_union_data_deserialize_fn deserialize_fn; // For deserialization
} ssz_union_t;

#endif /* SSZ_TYPES_H */
