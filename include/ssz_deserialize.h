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

ssz_error_t ssz_deserialize_uint8(const uint8_t in[1], uint8_t *out_value);
ssz_error_t ssz_deserialize_uint16(const uint8_t in[2], uint16_t *out_value);
ssz_error_t ssz_deserialize_uint32(const uint8_t in[4], uint32_t *out_value);
ssz_error_t ssz_deserialize_uint64(const uint8_t in[8], uint64_t *out_value);
ssz_error_t ssz_deserialize_uint128(const uint8_t in[16], uint8_t out_value[16]);
ssz_error_t ssz_deserialize_uint256(const uint8_t in[32], uint8_t out_value[32]);
ssz_error_t ssz_deserialize_boolean(const uint8_t in[1], uint8_t *out_value);

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

ssz_error_t ssz_deserialize_vector_fixed(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t element_size,
    uint8_t *out_elements,
    size_t out_elements_len);

ssz_error_t ssz_deserialize_vector_variable(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_count,
    size_t min_element_size,
    ssz_member_codec_t *codec);

ssz_error_t ssz_deserialize_list_fixed(
    const uint8_t *in,
    size_t in_len,
    uint64_t element_limit,
    size_t element_size,
    uint8_t *out_elements,
    size_t out_elements_len,
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
    const size_t *field_fixed_sizes,
    uint32_t field_count,
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

static inline ssz_error_t ssz_deserialize_progressive_container(
    const uint8_t *in,
    size_t in_len,
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    ssz_member_codec_t *codec)
{
    return ssz_deserialize_container(in, in_len, field_fixed_sizes, field_count, codec);
}

static inline ssz_error_t ssz_deserialize_progressive_list_fixed(
    const uint8_t *in,
    size_t in_len,
    size_t element_size,
    uint8_t *out_elements,
    size_t out_elements_len,
    uint64_t *out_element_count)
{
    return ssz_deserialize_list_fixed(
        in,
        in_len,
        SSZ_NO_LIMIT,
        element_size,
        out_elements,
        out_elements_len,
        out_element_count);
}

static inline ssz_error_t ssz_deserialize_progressive_list_variable(
    const uint8_t *in,
    size_t in_len,
    size_t min_element_size,
    ssz_member_codec_t *codec,
    uint64_t *out_element_count)
{
    return ssz_deserialize_list_variable(
        in,
        in_len,
        SSZ_NO_LIMIT,
        min_element_size,
        codec,
        out_element_count);
}

static inline ssz_error_t ssz_deserialize_progressive_bitlist(
    const uint8_t *in,
    size_t in_len,
    uint8_t *out_bits_le,
    size_t out_bits_le_len,
    uint64_t *out_bit_len)
{
    return ssz_deserialize_bitlist(
        in,
        in_len,
        SSZ_NO_LIMIT,
        out_bits_le,
        out_bits_le_len,
        out_bit_len);
}

#ifdef __cplusplus
}
#endif

#endif
