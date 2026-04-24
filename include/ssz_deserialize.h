#ifndef SSZ_DESERIALIZE_H
#define SSZ_DESERIALIZE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    ssz_error_t ssz_deserialize_uint8(const uint8_t *in, size_t in_len, uint8_t *out_value);
    ssz_error_t ssz_deserialize_uint16(const uint8_t *in, size_t in_len, uint16_t *out_value);
    ssz_error_t ssz_deserialize_uint32(const uint8_t *in, size_t in_len, uint32_t *out_value);
    ssz_error_t ssz_deserialize_uint64(const uint8_t *in, size_t in_len, uint64_t *out_value);
    ssz_error_t ssz_deserialize_uint128(const uint8_t *in, size_t in_len, uint8_t out_value[16]);
    ssz_error_t ssz_deserialize_uint256(const uint8_t *in, size_t in_len, uint8_t out_value[32]);
    ssz_error_t ssz_deserialize_boolean(const uint8_t *in, size_t in_len, uint8_t *out_value);

    ssz_error_t ssz_deserialize_bitvector(
        const uint8_t *in,
        size_t in_len,
        uint64_t bit_count,
        uint8_t *out_bits_le,
        size_t out_bits_le_len);

    ssz_error_t ssz_deserialize_bitlist(
        const uint8_t *in,
        size_t in_len,
        uint64_t bit_limit,
        uint8_t *out_bits_le,
        size_t out_bits_le_len,
        uint64_t *out_bit_len);

    /* Raw fixed-width vector decoding. For Vector[boolean, N], use
       ssz_deserialize_vector_boolean so every element is checked for the
       canonical 0x00/0x01 encoding. */
    ssz_error_t ssz_deserialize_vector_fixed(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_count,
        size_t element_size,
        uint8_t *out_elements,
        size_t out_elements_len);

    /* Decode Vector[boolean, N] and reject any non-canonical boolean byte. */
    ssz_error_t ssz_deserialize_vector_boolean(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_count,
        uint8_t *out_values,
        size_t out_values_len);

    ssz_error_t ssz_deserialize_vector_variable(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_count,
        size_t min_element_size,
        ssz_member_codec_t *codec);

    /* Raw fixed-width list decoding. For List[boolean, N], use
       ssz_deserialize_list_boolean so every element is checked for the
       canonical 0x00/0x01 encoding. */
    ssz_error_t ssz_deserialize_list_fixed(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_limit,
        size_t element_size,
        uint8_t *out_elements,
        size_t out_elements_len,
        uint64_t *out_element_count);

    /* Decode List[boolean, N] and reject any non-canonical boolean byte. */
    ssz_error_t ssz_deserialize_list_boolean(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_limit,
        uint8_t *out_values,
        size_t out_values_len,
        uint64_t *out_element_count);

    ssz_error_t ssz_deserialize_list_variable(
        const uint8_t *in,
        size_t in_len,
        uint64_t element_limit,
        size_t min_element_size,
        ssz_member_codec_t *codec,
        uint64_t *out_element_count);

    ssz_error_t ssz_deserialize_container(
        const uint8_t *in,
        size_t in_len,
        const ssz_container_schema_t *schema,
        ssz_member_codec_t *codec);

    ssz_error_t ssz_deserialize_union(
        const uint8_t *in,
        size_t in_len,
        uint32_t option_count,
        bool has_none,
        ssz_member_codec_t *codec,
        uint8_t *out_selector);

    ssz_error_t ssz_deserialize_compatible_union(
        const uint8_t *in,
        size_t in_len,
        const uint8_t *allowed_selectors,
        uint32_t allowed_selector_count,
        ssz_member_codec_t *codec,
        uint8_t *out_selector);

#ifdef __cplusplus
}
#endif

#endif
