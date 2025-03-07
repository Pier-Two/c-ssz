#ifndef SSZ_GENERATOR_H
#define SSZ_GENERATOR_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "ssz_types.h"

/*
 * Note: To improve portability for extremely large serializations, we use a 64-bit
 * offset type internally. This avoids potential overflow on 32-bit platforms when
 * serializing data larger than 4GB. At the end of serialization the offset is checked
 * to fit into a size_t (the maximum supported serialized size). 
 */
typedef uint64_t ssz_offset_t;

#define DEFINE_SERIALIZE_CONTAINER(ContainerType, CONTAINER_FIELDS)           \
    ssz_error_t serialize_##ContainerType(const ContainerType *obj,           \
                                          uint8_t *out_buf, size_t *out_size) \
    {                                                                         \
        ssz_offset_t offset = 0;                                                \
        CONTAINER_FIELDS                                                      \
        if (offset > SIZE_MAX) {                                                \
            return SSZ_ERROR_SERIALIZATION;                                   \
        }                                                                     \
        *out_size = (size_t)offset;                                             \
        return SSZ_SUCCESS;                                                   \
    }

#define DEFINE_SERIALIZE_LIST(ListType, ElementType, ElementSize, ser_func)                    \
    ssz_error_t serialize_##ListType(const ListType *list, uint8_t *out_buf, size_t *out_size) \
    {                                                                                          \
        ssz_offset_t offset = 0;                                                                 \
                                                                                               \
        if (list->length == 0 || list->data == NULL)                                           \
        {                                                                                      \
            *out_size = 0;                                                                     \
            return SSZ_SUCCESS;                                                                \
        }                                                                                      \
                                                                                               \
        uint64_t num = list->length;                                                           \
        uint64_t unroll_count = num / SSZ_BYTES_PER_LENGTH_OFFSET;                             \
        uint64_t remainder = num % SSZ_BYTES_PER_LENGTH_OFFSET;                                \
                                                                                               \
        const ElementType *src = list->data;                                                   \
        for (uint64_t i = 0; i < unroll_count; i++)                                            \
        {                                                                                      \
            ssz_error_t err;                                                                   \
            size_t tmp_size = 0;                                                               \
                                                                                               \
            err = ser_func(&src[0], out_buf + (size_t)offset, &tmp_size);                        \
            if (err != SSZ_SUCCESS)                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                \
            offset += tmp_size;                                                                \
                                                                                               \
            tmp_size = 0;                                                                      \
            err = ser_func(&src[1], out_buf + (size_t)offset, &tmp_size);                        \
            if (err != SSZ_SUCCESS)                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                \
            offset += tmp_size;                                                                \
                                                                                               \
            tmp_size = 0;                                                                      \
            err = ser_func(&src[2], out_buf + (size_t)offset, &tmp_size);                        \
            if (err != SSZ_SUCCESS)                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                \
            offset += tmp_size;                                                                \
                                                                                               \
            tmp_size = 0;                                                                      \
            err = ser_func(&src[3], out_buf + (size_t)offset, &tmp_size);                        \
            if (err != SSZ_SUCCESS)                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                \
            offset += tmp_size;                                                                \
            src += SSZ_BYTES_PER_LENGTH_OFFSET;                                                \
        }                                                                                      \
                                                                                               \
        for (uint64_t i = 0; i < remainder; i++)                                               \
        {                                                                                      \
            ssz_error_t err;                                                                   \
            size_t tmp_size = 0;                                                               \
            err = ser_func(&src[i], out_buf + (size_t)offset, &tmp_size);                        \
            if (err != SSZ_SUCCESS)                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                \
            offset += tmp_size;                                                                \
        }                                                                                      \
                                                                                               \
        if (offset > SIZE_MAX) {                                                               \
            return SSZ_ERROR_SERIALIZATION;                                                    \
        }                                                                                      \
        *out_size = (size_t)offset;                                                            \
        return SSZ_SUCCESS;                                                                    \
    }

#define SERIALIZE_BASIC_FIELD(obj, offset, field, field_size, ser_func)                 \
    do                                                                                  \
    {                                                                                   \
        size_t tmp_size = (field_size);                                                 \
        ssz_error_t err_local = ser_func(&(obj)->field, out_buf + (size_t)(offset), &tmp_size); \
        if (err_local != SSZ_SUCCESS)                                                   \
        {                                                                               \
            return SSZ_ERROR_SERIALIZATION;                                             \
        }                                                                               \
        (offset) += tmp_size;                                                           \
    } while (0)

#define SERIALIZE_VECTOR_FIELD(obj, offset, field, field_size, ser_func)         \
    do                                                                           \
    {                                                                            \
        size_t element_count = sizeof((obj)->field) / sizeof(((obj)->field)[0]); \
        size_t tmp_size = (field_size);                                          \
        ssz_error_t err_local = ser_func((obj)->field, element_count,            \
                                         out_buf + (size_t)(offset), &tmp_size);   \
        if (err_local != SSZ_SUCCESS)                                            \
        {                                                                        \
            return SSZ_ERROR_SERIALIZATION;                                      \
        }                                                                        \
        (offset) += tmp_size;                                                    \
    } while (0)

#define SERIALIZE_VECTOR_ARRAY_FIELD(obj, offset, field, element_size, count, ser_func) \
    do                                                                                  \
    {                                                                                   \
        for (size_t _i = 0; _i < (count); _i++)                                         \
        {                                                                               \
            size_t tmp_size = (element_size);                                           \
            size_t element_count = (element_size) / sizeof(((obj)->field)[_i][0]);      \
            ssz_error_t err_local = ser_func((obj)->field[_i], element_count,           \
                                             out_buf + (size_t)(offset), &tmp_size);        \
            if (err_local != SSZ_SUCCESS)                                               \
            {                                                                           \
                return SSZ_ERROR_SERIALIZATION;                                         \
            }                                                                           \
            (offset) += tmp_size;                                                       \
        }                                                                               \
    } while (0)

#define SERIALIZE_BITVECTOR_FIELD(obj, offset, field, bits)                             \
    do                                                                                  \
    {                                                                                   \
        size_t byte_size = ((bits) + 7) / SSZ_BITS_PER_BYTE;                            \
        size_t tmp_size = byte_size;                                                    \
        ssz_error_t err_local = ssz_serialize_bitvector((obj)->field, (bits),           \
                                                        out_buf + (size_t)(offset), &tmp_size); \
        if (err_local != SSZ_SUCCESS)                                                   \
        {                                                                               \
            return SSZ_ERROR_SERIALIZATION;                                             \
        }                                                                               \
        (offset) += tmp_size;                                                           \
    } while (0)

#define SERIALIZE_BITLIST_FIELD(obj, offset, field, max_bits)                   \
    do                                                                          \
    {                                                                           \
        size_t expected_size = (((obj)->field.length) / SSZ_BITS_PER_BYTE) + 1;   \
        size_t tmp_size = expected_size;                                        \
        ssz_error_t err_local = ssz_serialize_bitlist((obj)->field.data,         \
                                                      (obj)->field.length,       \
                                                      out_buf + (size_t)(offset),\
                                                      &tmp_size);                \
        if (err_local != SSZ_SUCCESS)                                           \
        {                                                                       \
            return SSZ_ERROR_SERIALIZATION;                                     \
        }                                                                       \
        (offset) += tmp_size;                                                   \
    } while (0)

#define SERIALIZE_OFFSET_FIELD(var, base, offset, field_size)                              \
    do                                                                                     \
    {                                                                                      \
        var = (uint32_t)(base);                                                            \
        size_t tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                         \
        ssz_error_t err_local = ssz_serialize_uint32(&var, out_buf + (size_t)(offset), &tmp_size); \
        if (err_local != SSZ_SUCCESS)                                                      \
        {                                                                                  \
            return SSZ_ERROR_SERIALIZATION;                                                \
        }                                                                                  \
        (offset) += tmp_size;                                                              \
        (base) += (field_size);                                                            \
    } while (0)

#define SERIALIZE_CONTAINER_FIELD(obj, offset, field, container_ser_func, field_size) \
    do                                                                                \
    {                                                                                 \
        size_t tmp_size = (field_size);                                               \
        ssz_error_t err_local = container_ser_func(&(obj)->field, out_buf + (size_t)(offset), \
                                                   &tmp_size);                        \
        if (err_local != SSZ_SUCCESS)                                                 \
        {                                                                             \
            return SSZ_ERROR_SERIALIZATION;                                           \
        }                                                                             \
        (offset) += tmp_size;                                                         \
    } while (0)

#define SERIALIZE_LIST_FIELD(obj, offset, field, element_size)    \
    do                                                            \
    {                                                             \
        size_t _tmp_size = (obj)->field.length * (element_size);  \
        memcpy(out_buf + (size_t)(offset), (obj)->field.data, _tmp_size); \
        (offset) += _tmp_size;                                    \
    } while (0)

#define SERIALIZE_LIST_CONTAINER_FIELD(obj, offset, size, field, container_ser_func)  \
    do                                                                                \
    {                                                                                 \
        size_t tmp_size = (size);                                                     \
        ssz_error_t err_local = container_ser_func(&(obj)->field, out_buf + (size_t)(offset), \
                                                   &tmp_size);                        \
        if (err_local != SSZ_SUCCESS)                                                 \
        {                                                                             \
            return SSZ_ERROR_SERIALIZATION;                                           \
        }                                                                             \
        (offset) += tmp_size;                                                         \
    } while (0)

#define SERIALIZE_VARIABLE_FIELD_OFFSET(obj, fixed_var, fixed_offset, base, allocated_size, field, container_ser_func)                   \
    do                                                                                                                                   \
    {                                                                                                                                    \
        uint32_t _num_elements = (uint32_t)((obj)->field.length);                                                                        \
        fixed_var = (uint32_t)(base);                                                                                                    \
        {                                                                                                                                \
            size_t _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                                                  \
            ssz_error_t _err_local = ssz_serialize_uint32(&fixed_var, out_buf + (size_t)(fixed_offset), &_tmp_size);                             \
            if (_err_local != SSZ_SUCCESS)                                                                                               \
            {                                                                                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                                                          \
            }                                                                                                                            \
            (fixed_offset) += SSZ_BYTE_SIZE_OF_UINT32;                                                                                   \
        }                                                                                                                                \
        if (_num_elements == 0)                                                                                                          \
            break;                                                                                                                       \
        if (_num_elements > (allocated_size))                                                                                            \
        {                                                                                                                                \
            return SSZ_ERROR_SERIALIZATION;                                                                                              \
        }                                                                                                                                \
        size_t _offset_table_start = base;                                                                                               \
        uint32_t _table_size = _num_elements * SSZ_BYTE_SIZE_OF_UINT32;                                                                  \
        size_t _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                                                      \
        ssz_error_t _err = ssz_serialize_uint32(&_table_size, out_buf + (size_t)(_offset_table_start), &_tmp_size);                                \
        if (_err != SSZ_SUCCESS)                                                                                                         \
        {                                                                                                                                \
            return SSZ_ERROR_SERIALIZATION;                                                                                              \
        }                                                                                                                                \
        size_t _current_pos = _offset_table_start + SSZ_BYTE_SIZE_OF_UINT32;                                                             \
        for (uint32_t _i = 1; _i < _num_elements; _i++)                                                                                  \
        {                                                                                                                                \
            uint32_t zero_val = 0;                                                                                                       \
            _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                                                         \
            _err = ssz_serialize_uint32(&zero_val, out_buf + (size_t)(_current_pos), &_tmp_size);                                                  \
            if (_err != SSZ_SUCCESS)                                                                                                     \
            {                                                                                                                            \
                return SSZ_ERROR_SERIALIZATION;                                                                                          \
            }                                                                                                                            \
            _current_pos += SSZ_BYTE_SIZE_OF_UINT32;                                                                                     \
        }                                                                                                                                \
        size_t _variable_start = _offset_table_start + _num_elements * SSZ_BYTE_SIZE_OF_UINT32;                                          \
        size_t _current_sub_offset = _variable_start;                                                                                    \
        uint8_t *_temp_buf = malloc(allocated_size);                                                                                     \
        if (!_temp_buf)                                                                                                                  \
        {                                                                                                                                \
            return SSZ_ERROR_SERIALIZATION;                                                                                              \
        }                                                                                                                                \
        for (uint32_t _i = 0; _i < _num_elements; _i++)                                                                                  \
        {                                                                                                                                \
            uint32_t _this_elem_offset = (uint32_t)(_current_sub_offset - _offset_table_start);                                          \
            _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                                                         \
            _err = ssz_serialize_uint32(&_this_elem_offset, out_buf + (size_t)(_offset_table_start + (_i * SSZ_BYTE_SIZE_OF_UINT32)), &_tmp_size); \
            if (_err != SSZ_SUCCESS)                                                                                                     \
            {                                                                                                                            \
                free(_temp_buf);                                                                                                         \
                return SSZ_ERROR_SERIALIZATION;                                                                                          \
            }                                                                                                                            \
            size_t _elem_ser_size = 0;                                                                                                   \
            _err = container_ser_func(&(obj)->field.data[_i], _temp_buf, &_elem_ser_size);                                               \
            if (_err != SSZ_SUCCESS)                                                                                                     \
            {                                                                                                                            \
                free(_temp_buf);                                                                                                         \
                return SSZ_ERROR_SERIALIZATION;                                                                                          \
            }                                                                                                                            \
            _current_sub_offset += _elem_ser_size;                                                                                       \
        }                                                                                                                                \
        free(_temp_buf);                                                                                                                 \
        if (_current_sub_offset > _offset_table_start + (allocated_size))                                                                \
        {                                                                                                                                \
            return SSZ_ERROR_SERIALIZATION;                                                                                              \
        }                                                                                                                                \
        base = _current_sub_offset;                                                                                                      \
    } while (0)

#define SERIALIZE_LIST_VARIABLE_CONTAINER_FIELD(obj, var_offset, field, container_ser_func)                         \
    do                                                                                                              \
    {                                                                                                               \
        uint32_t _num_elements = (obj)->field.length;                                                               \
        size_t _offset_table_start = (var_offset);                                                                  \
        for (uint32_t _i = 0; _i < _num_elements; _i++)                                                             \
        {                                                                                                           \
            uint32_t zero_val = 0;                                                                                  \
            size_t _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                             \
            ssz_error_t _err = ssz_serialize_uint32(&zero_val,                                                      \
                                                    out_buf + (size_t)(_offset_table_start + (_i * SSZ_BYTE_SIZE_OF_UINT32)), \
                                                    &_tmp_size);                                                    \
            if (_err != SSZ_SUCCESS)                                                                                \
            {                                                                                                       \
                return SSZ_ERROR_SERIALIZATION;                                                                     \
            }                                                                                                       \
        }                                                                                                           \
        size_t _variable_start = _offset_table_start + _num_elements * SSZ_BYTE_SIZE_OF_UINT32;                     \
        size_t _cur_offset = _variable_start;                                                                       \
        for (uint32_t _i = 0; _i < _num_elements; _i++)                                                             \
        {                                                                                                           \
            size_t _elem_ser_size = 0;                                                                              \
            ssz_error_t _err = container_ser_func(&(obj)->field.data[_i],                                           \
                                                  out_buf + (size_t)(_cur_offset),                                            \
                                                  &_elem_ser_size);                                                 \
            if (_err != SSZ_SUCCESS)                                                                                \
            {                                                                                                       \
                return SSZ_ERROR_SERIALIZATION;                                                                     \
            }                                                                                                       \
            uint32_t _elem_offset = (uint32_t)(_cur_offset - _offset_table_start);                                  \
            size_t _tmp_size = SSZ_BYTE_SIZE_OF_UINT32;                                                             \
            _err = ssz_serialize_uint32(&_elem_offset,                                                              \
                                        out_buf + (size_t)(_offset_table_start + (_i * SSZ_BYTE_SIZE_OF_UINT32)),             \
                                        &_tmp_size);                                                                \
            if (_err != SSZ_SUCCESS)                                                                                \
            {                                                                                                       \
                return SSZ_ERROR_SERIALIZATION;                                                                     \
            }                                                                                                       \
            _cur_offset += _elem_ser_size;                                                                          \
        }                                                                                                           \
        (var_offset) = _cur_offset;                                                                                 \
    } while (0)

#define DEFINE_DESERIALIZE_CONTAINER(ContainerType, CONTAINER_FIELDS)                    \
    ssz_error_t deserialize_##ContainerType(const unsigned char *data, size_t data_size, \
                                            ContainerType *obj)                          \
    {                                                                                    \
        (void)data_size;                                                                 \
        ssz_offset_t offset = 0;                                                         \
        CONTAINER_FIELDS                                                                 \
        return SSZ_SUCCESS;                                                              \
    }

#define DEFINE_DESERIALIZE_LIST(ListType, ElementType, ElementSize, deserialize_func)               \
    ssz_error_t deserialize_##ListType(const unsigned char *data, size_t data_size, ListType *list) \
    {                                                                                               \
        if (data_size % (ElementSize) != 0)                                                         \
        {                                                                                           \
            return SSZ_ERROR_DESERIALIZATION;                                                       \
        }                                                                                           \
        uint64_t num = data_size / (ElementSize);                                                   \
        list->length = num;                                                                         \
        if (num == 0)                                                                               \
        {                                                                                           \
            list->data = NULL;                                                                      \
            return SSZ_SUCCESS;                                                                     \
        }                                                                                           \
        list->data = malloc(num * sizeof(ElementType));                                             \
        if (!list->data)                                                                            \
        {                                                                                           \
            return SSZ_ERROR_DESERIALIZATION;                                                       \
        }                                                                                           \
                                                                                                    \
        const unsigned char *p = data;                                                              \
        ElementType *dest = list->data;                                                             \
        uint64_t unroll_count = num / SSZ_BYTES_PER_LENGTH_OFFSET;                                  \
        uint64_t remainder = num % SSZ_BYTES_PER_LENGTH_OFFSET;                                     \
        uint64_t i;                                                                                 \
                                                                                                    \
        for (i = 0; i < unroll_count; i++)                                                          \
        {                                                                                           \
            ssz_error_t err;                                                                        \
            err = deserialize_func(p, (ElementSize), dest);                                         \
            if (err != SSZ_SUCCESS)                                                                 \
            {                                                                                       \
                free(list->data);                                                                   \
                return SSZ_ERROR_DESERIALIZATION;                                                   \
            }                                                                                       \
            p += (ElementSize);                                                                     \
            dest++;                                                                                 \
                                                                                                    \
            err = deserialize_func(p, (ElementSize), dest);                                         \
            if (err != SSZ_SUCCESS)                                                                 \
            {                                                                                       \
                free(list->data);                                                                   \
                return SSZ_ERROR_DESERIALIZATION;                                                   \
            }                                                                                       \
            p += (ElementSize);                                                                     \
            dest++;                                                                                 \
                                                                                                    \
            err = deserialize_func(p, (ElementSize), dest);                                         \
            if (err != SSZ_SUCCESS)                                                                 \
            {                                                                                       \
                free(list->data);                                                                   \
                return SSZ_ERROR_DESERIALIZATION;                                                   \
            }                                                                                       \
            p += (ElementSize);                                                                     \
            dest++;                                                                                 \
                                                                                                    \
            err = deserialize_func(p, (ElementSize), dest);                                         \
            if (err != SSZ_SUCCESS)                                                                 \
            {                                                                                       \
                free(list->data);                                                                   \
                return SSZ_ERROR_DESERIALIZATION;                                                   \
            }                                                                                       \
            p += (ElementSize);                                                                     \
            dest++;                                                                                 \
        }                                                                                           \
                                                                                                    \
        for (i = 0; i < remainder; i++)                                                             \
        {                                                                                           \
            ssz_error_t err = deserialize_func(p, (ElementSize), dest);                             \
            if (err != SSZ_SUCCESS)                                                                 \
            {                                                                                       \
                free(list->data);                                                                   \
                return SSZ_ERROR_DESERIALIZATION;                                                   \
            }                                                                                       \
            p += (ElementSize);                                                                     \
            dest++;                                                                                 \
        }                                                                                           \
                                                                                                    \
        return SSZ_SUCCESS;                                                                         \
    }

#define DESERIALIZE_BASIC_FIELD(obj, offset, field, deserialize_func)                                   \
    do                                                                                                  \
    {                                                                                                   \
        ssz_error_t err_local = deserialize_func(data + (size_t)(offset), sizeof((obj)->field), &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                   \
        {                                                                                               \
            return SSZ_ERROR_DESERIALIZATION;                                                           \
        }                                                                                               \
        (offset) += sizeof((obj)->field);                                                               \
    } while (0)

#define DESERIALIZE_VECTOR_FIELD(obj, offset, field, deserialize_func)                                              \
    do                                                                                                              \
    {                                                                                                               \
        ssz_error_t err_local = deserialize_func(data + (size_t)(offset), sizeof((obj)->field),                             \
                                                 (sizeof((obj)->field) / sizeof(((obj)->field)[0])), (obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                               \
        {                                                                                                           \
            return SSZ_ERROR_DESERIALIZATION;                                                                       \
        }                                                                                                           \
        (offset) += sizeof((obj)->field);                                                                           \
    } while (0)

#define DESERIALIZE_VECTOR_ARRAY_FIELD(obj, offset, field, element_size, count, deserialize_func)                                             \
    do                                                                                                                                        \
    {                                                                                                                                         \
        ssz_error_t err_local = deserialize_func(data + (size_t)(offset), (element_size) * (count), (element_size) * (count), &((obj)->field[0][0])); \
        if (err_local != SSZ_SUCCESS)                                                                                                         \
        {                                                                                                                                     \
            return SSZ_ERROR_DESERIALIZATION;                                                                                                 \
        }                                                                                                                                     \
        (offset) += (element_size) * (count);                                                                                                 \
    } while (0)

#define DESERIALIZE_BITVECTOR_FIELD(obj, offset, field, bits)                                                \
    do                                                                                                       \
    {                                                                                                        \
        size_t byte_size = ((bits) + 7) / SSZ_BITS_PER_BYTE;                                                 \
        ssz_error_t err_local = ssz_deserialize_bitvector(data + (size_t)(offset), byte_size, (bits), (obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                                        \
        {                                                                                                    \
            return SSZ_ERROR_DESERIALIZATION;                                                                \
        }                                                                                                    \
        (offset) += byte_size;                                                                               \
    } while (0)

#define DESERIALIZE_BITLIST_FIELD(obj, offset_start, field_size, field, max_bits) \
    do                                                                            \
    {                                                                             \
        size_t actual_count = 0;                                                  \
        (obj)->field.data = malloc((max_bits) * sizeof(bool));                    \
        if (!(obj)->field.data)                                                   \
        {                                                                         \
            return SSZ_ERROR_DESERIALIZATION;                                     \
        }                                                                         \
        ssz_error_t err_local = ssz_deserialize_bitlist(data + (size_t)(offset_start),    \
                                                        (field_size),             \
                                                        (max_bits),               \
                                                        (obj)->field.data,        \
                                                        &actual_count);           \
        if (err_local != SSZ_SUCCESS)                                             \
        {                                                                         \
            free((obj)->field.data);                                              \
            return SSZ_ERROR_DESERIALIZATION;                                     \
        }                                                                         \
        (obj)->field.length = actual_count;                                       \
        (offset_start) += (field_size);                                           \
    } while (0)

#define DESERIALIZE_LIST_FIELD(obj, offset_start, list_size, field, max_length, deserialize_func)                                     \
    do                                                                                                                                \
    {                                                                                                                                 \
        size_t actual_count = 0;                                                                                                      \
        (obj)->field.data = malloc((list_size));                                                                                      \
        if (!(obj)->field.data)                                                                                                       \
        {                                                                                                                             \
            return SSZ_ERROR_DESERIALIZATION;                                                                                         \
        }                                                                                                                             \
        ssz_error_t err_local = deserialize_func(data + (size_t)(offset_start), (list_size), (max_length), (obj)->field.data, &actual_count); \
        if (err_local != SSZ_SUCCESS)                                                                                                 \
        {                                                                                                                             \
            free((obj)->field.data);                                                                                                  \
            return SSZ_ERROR_DESERIALIZATION;                                                                                         \
        }                                                                                                                             \
        (obj)->field.length = actual_count;                                                                                           \
    } while (0)

#define DESERIALIZE_OFFSET_FIELD(var, offset)                                                      \
    do                                                                                             \
    {                                                                                              \
        ssz_error_t err_local = ssz_deserialize_uint32(data + (size_t)(offset), sizeof(uint32_t), &(var)); \
        if (err_local != SSZ_SUCCESS)                                                              \
        {                                                                                          \
            return SSZ_ERROR_DESERIALIZATION;                                                      \
        }                                                                                          \
        (offset) += sizeof(uint32_t);                                                              \
    } while (0)

#define DESERIALIZE_CONTAINER_FIELD(obj, offset, field, container_deser_func, field_size)         \
    do                                                                                            \
    {                                                                                             \
        ssz_error_t err_local = container_deser_func(data + (size_t)(offset), field_size, &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                             \
        {                                                                                         \
            return SSZ_ERROR_DESERIALIZATION;                                                     \
        }                                                                                         \
        (offset) += field_size;                                                                   \
    } while (0)

#define DESERIALIZE_LIST_CONTAINER_FIELD(obj, offset_start, size, field, deserialize_func)      \
    do                                                                                          \
    {                                                                                           \
        ssz_error_t err_local = deserialize_func(data + (size_t)(offset_start), (size), &(obj)->field); \
        if (err_local != SSZ_SUCCESS)                                                           \
        {                                                                                       \
            return SSZ_ERROR_DESERIALIZATION;                                                   \
        }                                                                                       \
    } while (0)

#define DESERIALIZE_LIST_VARIABLE_CONTAINER_FIELD(obj, field_offset, field_size, field, max_length, deserialize_func) \
    do                                                                                                                \
    {                                                                                                                 \
        const unsigned char *const _base_ptr = data + (size_t)(field_offset);                                                 \
        const size_t _field_size = (field_size);                                                                      \
        const uint32_t *const _offsets = (const uint32_t *)_base_ptr;                                                 \
        const uint32_t _num_elements = _offsets[0] / SSZ_BYTES_PER_LENGTH_OFFSET;                                     \
        if (_num_elements > (max_length))                                                                             \
        {                                                                                                             \
            return SSZ_ERROR_DESERIALIZATION;                                                                         \
        }                                                                                                             \
        (obj)->field.length = _num_elements;                                                                          \
        (obj)->field.data = malloc(_num_elements * sizeof(*(obj)->field.data));                                       \
        if (!(obj)->field.data)                                                                                       \
        {                                                                                                             \
            return SSZ_ERROR_DESERIALIZATION;                                                                         \
        }                                                                                                             \
        const uint32_t _last_index = _num_elements - 1;                                                               \
        for (uint32_t _i = 0; _i < _num_elements; _i++)                                                               \
        {                                                                                                             \
            const uint32_t _elem_rel_offset = _offsets[_i];                                                           \
            const uint32_t _next_rel_offset = (_i < _last_index) ? _offsets[_i + 1] : _field_size;                    \
            const uint32_t _elem_total_size = _next_rel_offset - _elem_rel_offset;                                    \
            ssz_error_t _err = deserialize_func(_base_ptr + _elem_rel_offset, _elem_total_size,                       \
                                                &((obj)->field.data[_i]));                                            \
            if (_err != SSZ_SUCCESS)                                                                                  \
            {                                                                                                         \
                free((obj)->field.data);                                                                              \
                return SSZ_ERROR_DESERIALIZATION;                                                                     \
            }                                                                                                         \
        }                                                                                                             \
    } while (0)

#endif /* SSZ_GENERATOR_H */