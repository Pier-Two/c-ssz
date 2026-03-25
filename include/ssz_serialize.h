#ifndef SSZ_SERIALIZE_H
#define SSZ_SERIALIZE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ssz_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

ssz_error_t ssz_serialize_uint8(uint8_t value, uint8_t out[1]);
ssz_error_t ssz_serialize_uint16(uint16_t value, uint8_t out[2]);
ssz_error_t ssz_serialize_uint32(uint32_t value, uint8_t out[4]);
ssz_error_t ssz_serialize_uint64(uint64_t value, uint8_t out[8]);
ssz_error_t ssz_serialize_uint128(const uint8_t value[16], uint8_t out[16]);
ssz_error_t ssz_serialize_uint256(const uint8_t value[32], uint8_t out[32]);
ssz_error_t ssz_serialize_boolean(uint8_t value, uint8_t out[1]);

ssz_error_t ssz_serialize_bitvector(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_count,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_bitlist(
    const uint8_t *bits_le,
    size_t bits_le_len,
    uint64_t bit_len,
    uint64_t bit_limit,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_vector_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    size_t element_size,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_vector_variable(
    uint64_t element_count,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_list_fixed(
    const uint8_t *elements,
    uint64_t element_count,
    uint64_t element_limit,
    size_t element_size,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_list_variable(
    uint64_t element_count,
    uint64_t element_limit,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_container(
    const size_t *field_fixed_sizes,
    uint32_t field_count,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_union(
    uint8_t selector,
    uint32_t option_count,
    bool has_none,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

ssz_error_t ssz_serialize_compatible_union(
    uint8_t selector,
    const uint8_t *allowed_selectors,
    uint32_t allowed_selector_count,
    const ssz_member_codec_t *codec,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
