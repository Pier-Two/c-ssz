#ifndef SSZ_MACROS_H
#define SSZ_MACROS_H

#include <stdint.h>
#include <stdlib.h>
#include "ssz_types.h"

#define DEFINE_DESERIALIZE_CONTAINER(ContainerType, CONTAINER_FIELDS)                    \
    ssz_error_t deserialize_##ContainerType(const unsigned char *data, size_t data_size, \
                                            ContainerType *obj)                          \
    {                                                                                    \
        (void)data_size;                                                                 \
        size_t offset = 0;                                                               \
        CONTAINER_FIELDS                                                                 \
        return SSZ_SUCCESS;                                                              \
    }

#define DEFINE_DESERIALIZE_LIST(ListType, ElementType, ElementSize, deser_func)                                \
    ssz_error_t deserialize_##ListType(const unsigned char *data, size_t data_size, ListType *list) {           \
        if (data_size % (ElementSize) != 0) {                                                                  \
            return SSZ_ERROR_DESERIALIZATION;                                                                \
        }                                                                                                    \
        uint64_t num = data_size / (ElementSize);                                                            \
        list->length = num;                                                                                  \
        if (num == 0) {                                                                                      \
            list->data = NULL;                                                                               \
            return SSZ_SUCCESS;                                                                              \
        }                                                                                                    \
        list->data = malloc(num * sizeof(ElementType));                                                      \
        if (!list->data) {                                                                                   \
            return SSZ_ERROR_DESERIALIZATION;                                                                \
        }                                                                                                    \
                                                                                                             \
        const unsigned char *p = data;                                                                       \
        ElementType *dest = list->data;                                                                      \
        uint64_t unroll_count = num / 4;                                                                     \
        uint64_t remainder = num % 4;                                                                        \
        uint64_t i;                                                                                        \
                                                                                                             \
        for (i = 0; i < unroll_count; i++) {                                                                 \
            ssz_error_t err;                                                                                 \
            err = deser_func(p, (ElementSize), dest);                                                        \
            if (err != SSZ_SUCCESS) { free(list->data); return SSZ_ERROR_DESERIALIZATION; }                    \
            p += (ElementSize); dest++;                                                                      \
                                                                                                             \
            err = deser_func(p, (ElementSize), dest);                                                        \
            if (err != SSZ_SUCCESS) { free(list->data); return SSZ_ERROR_DESERIALIZATION; }                    \
            p += (ElementSize); dest++;                                                                      \
                                                                                                             \
            err = deser_func(p, (ElementSize), dest);                                                        \
            if (err != SSZ_SUCCESS) { free(list->data); return SSZ_ERROR_DESERIALIZATION; }                    \
            p += (ElementSize); dest++;                                                                      \
                                                                                                             \
            err = deser_func(p, (ElementSize), dest);                                                        \
            if (err != SSZ_SUCCESS) { free(list->data); return SSZ_ERROR_DESERIALIZATION; }                    \
            p += (ElementSize); dest++;                                                                      \
        }                                                                                                    \
                                                                                                             \
        for (i = 0; i < remainder; i++) {                                                                    \
            ssz_error_t err = deser_func(p, (ElementSize), dest);                                            \
            if (err != SSZ_SUCCESS) { free(list->data); return SSZ_ERROR_DESERIALIZATION; }                    \
            p += (ElementSize); dest++;                                                                      \
        }                                                                                                    \
                                                                                                             \
        return SSZ_SUCCESS;                                                                                  \
    }

#define DESERIALIZE_BASIC_FIELD(obj, offset, field, deser_func)                                   \
    do                                                                                            \
    {                                                                                             \
        ssz_error_t err_local = deser_func(data + (offset), sizeof((obj)->field), &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                             \
        {                                                                                         \
            return SSZ_ERROR_DESERIALIZATION;                                                     \
        }                                                                                         \
        (offset) += sizeof((obj)->field);                                                         \
    } while (0)

#define DESERIALIZE_VECTOR_FIELD(obj, offset, field, deser_func)                                              \
    do                                                                                                        \
    {                                                                                                         \
        ssz_error_t err_local = deser_func(data + (offset), sizeof((obj)->field),                             \
                                           (sizeof((obj)->field) / sizeof(((obj)->field)[0])), (obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                         \
        {                                                                                                     \
            return SSZ_ERROR_DESERIALIZATION;                                                                 \
        }                                                                                                     \
        (offset) += sizeof((obj)->field);                                                                     \
    } while (0)

#define DESERIALIZE_VECTOR_ARRAY_FIELD(obj, offset, field, element_size, count, deser_func)                                             \
    do                                                                                                                                  \
    {                                                                                                                                   \
        ssz_error_t err_local = deser_func(data + (offset), (element_size) * (count), (element_size) * (count), &((obj)->field[0][0])); \
        if (err_local != SSZ_SUCCESS)                                                                                                   \
        {                                                                                                                               \
            return SSZ_ERROR_DESERIALIZATION;                                                                                           \
        }                                                                                                                               \
        (offset) += (element_size) * (count);                                                                                           \
    } while (0)

#define DESERIALIZE_BITVECTOR_FIELD(obj, offset, field, bits)                                                \
    do                                                                                                       \
    {                                                                                                        \
        size_t byte_size = ((bits) + 7) / 8;                                                                 \
        ssz_error_t err_local = ssz_deserialize_bitvector(data + (offset), byte_size, (bits), (obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                        \
        {                                                                                                    \
            return SSZ_ERROR_DESERIALIZATION;                                                                \
        }                                                                                                    \
        (offset) += byte_size;                                                                               \
    } while (0)

#define DESERIALIZE_BITLIST_FIELD(obj, offset_start, field_size, field, max_bits)                                                           \
    do                                                                                                                                      \
    {                                                                                                                                       \
        size_t actual_count = 0;                                                                                                            \
        (obj)->field.data = malloc((field_size));                                                                                           \
        if (!(obj)->field.data)                                                                                                             \
        {                                                                                                                                   \
            return SSZ_ERROR_DESERIALIZATION;                                                                                               \
        }                                                                                                                                   \
        ssz_error_t err_local = ssz_deserialize_bitlist(data + (offset_start), (field_size), (max_bits), (obj)->field.data, &actual_count); \
        if (err_local != SSZ_SUCCESS)                                                                                                       \
        {                                                                                                                                   \
            free((obj)->field.data);                                                                                                        \
            return SSZ_ERROR_DESERIALIZATION;                                                                                               \
        }                                                                                                                                   \
        (obj)->field.length = actual_count;                                                                                                 \
        (offset_start) += (field_size);                                                                                                     \
    } while (0)

#define DESERIALIZE_LIST_FIELD(obj, offset_start, list_size, field, max_length, deser_func)                                     \
    do                                                                                                                          \
    {                                                                                                                           \
        size_t actual_count = 0;                                                                                                \
        (obj)->field.data = malloc((list_size));                                                                                \
        if (!(obj)->field.data)                                                                                                 \
        {                                                                                                                       \
            return SSZ_ERROR_DESERIALIZATION;                                                                                   \
        }                                                                                                                       \
        ssz_error_t err_local = deser_func(data + (offset_start), (list_size), (max_length), (obj)->field.data, &actual_count); \
        if (err_local != SSZ_SUCCESS)                                                                                           \
        {                                                                                                                       \
            free((obj)->field.data);                                                                                            \
            return SSZ_ERROR_DESERIALIZATION;                                                                                   \
        }                                                                                                                       \
        (obj)->field.length = actual_count;                                                                                     \
    } while (0)

#define DESERIALIZE_OFFSET_FIELD(var, offset, field_name)                                          \
    do                                                                                             \
    {                                                                                              \
        ssz_error_t err_local = ssz_deserialize_uint32(data + (offset), sizeof(uint32_t), &(var)); \
        if (err_local != SSZ_SUCCESS)                                                              \
        {                                                                                          \
            return SSZ_ERROR_DESERIALIZATION;                                                      \
        }                                                                                          \
        (offset) += sizeof(uint32_t);                                                              \
    } while (0)

#define DESERIALIZE_CONTAINER_FIELD(obj, offset, field, container_deser_func, field_total_length)         \
    do                                                                                                    \
    {                                                                                                     \
        ssz_error_t err_local = container_deser_func(data + (offset), field_total_length, &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                     \
        {                                                                                                 \
            return SSZ_ERROR_DESERIALIZATION;                                                             \
        }                                                                                                 \
        (offset) += field_total_length;                                                                   \
    } while (0)

#define DESERIALIZE_LIST_CONTAINER_FIELD(obj, offset_start, size, field, deser_func)      \
    do                                                                                    \
    {                                                                                     \
        ssz_error_t err_local = deser_func(data + (offset_start), (size), &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                     \
        {                                                                                 \
            return SSZ_ERROR_DESERIALIZATION;                                             \
        }                                                                                 \
    } while (0)

#define DESERIALIZE_LIST_VARIABLE_CONTAINER_FIELD(obj, field_offset, field_size, field, max_length, deser_func) \
    do                                                                                                          \
    {                                                                                                           \
        const unsigned char *const _base_ptr = data + (field_offset);                                           \
        const size_t _field_size = (field_size);                                                                \
        const uint32_t *const _offsets = (const uint32_t *)_base_ptr;                                           \
        const uint32_t _num_elements = _offsets[0] / SSZ_BYTES_PER_LENGTH_OFFSET;                               \
        if (_num_elements > (max_length))                                                                       \
        {                                                                                                       \
            return SSZ_ERROR_DESERIALIZATION;                                                                   \
        }                                                                                                       \
        (obj)->field.length = _num_elements;                                                                    \
        (obj)->field.data = malloc(_num_elements * sizeof(*(obj)->field.data));                                 \
        if (!(obj)->field.data)                                                                                 \
        {                                                                                                       \
            return SSZ_ERROR_DESERIALIZATION;                                                                   \
        }                                                                                                       \
        const uint32_t _last_index = _num_elements - 1;                                                         \
        for (uint32_t _i = 0; _i < _num_elements; _i++)                                                         \
        {                                                                                                       \
            const uint32_t _elem_rel_offset = _offsets[_i];                                                     \
            const uint32_t _next_rel_offset = (_i < _last_index) ? _offsets[_i + 1] : _field_size;              \
            const uint32_t _elem_total_size = _next_rel_offset - _elem_rel_offset;                              \
            ssz_error_t _err = deser_func(_base_ptr + _elem_rel_offset, _elem_total_size,                       \
                                          &((obj)->field.data[_i]));                                            \
            if (_err != SSZ_SUCCESS)                                                                            \
            {                                                                                                   \
                free((obj)->field.data);                                                                        \
                return SSZ_ERROR_DESERIALIZATION;                                                               \
            }                                                                                                   \
        }                                                                                                       \
    } while (0)

#endif /* SSZ_MACROS_H */