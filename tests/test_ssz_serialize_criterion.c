#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "ssz_serialize.h"
#include "ssz_types.h"

Test(ssz_serialize_uintN, valid_32bit)
{
    uint64_t val32 = 0xAABBCCDDULL;
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_uintN(&val32, 32, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS, "Serialization must succeed for 32-bit values");
    cr_assert_eq(out_size, 4, "out_size should be 4 for 32-bit");
    cr_assert(buffer[0] == 0xDD && buffer[1] == 0xCC && buffer[2] == 0xBB && buffer[3] == 0xAA,
              "Unexpected byte order for 32-bit serialization");
}

Test(ssz_serialize_uintN, valid_64bit)
{
    uint64_t val64 = 0x1122334455667788ULL;
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_uintN(&val64, 64, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS, "Serialization must succeed for 64-bit values");
    cr_assert_eq(out_size, 8, "out_size should be 8 for 64-bit");
    cr_assert(buffer[0] == 0x88 && buffer[1] == 0x77 && buffer[2] == 0x66 && buffer[3] == 0x55 &&
              buffer[4] == 0x44 && buffer[5] == 0x33 && buffer[6] == 0x22 && buffer[7] == 0x11,
              "Unexpected byte order for 64-bit serialization");
}

Test(ssz_serialize_uintN, valid_128bit)
{
    uint8_t val128[16] = {
        0xEF, 0xCD, 0xAB, 0x89,
        0x67, 0x45, 0x23, 0x01,
        0xEF, 0xCD, 0xAB, 0x89,
        0x67, 0x45, 0x23, 0x01
    };
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_uintN(val128, 128, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 16);
    cr_assert_eq(memcmp(buffer, val128, 16), 0, "128-bit data mismatch after serialization");
}

Test(ssz_serialize_uintN, valid_256bit)
{
    uint8_t val256[32] = {
        0x88, 0x77, 0x66, 0x55,
        0x44, 0x33, 0x22, 0x11,
        0xAA, 0xBB, 0xCC, 0xDD,
        0xEE, 0xFF, 0x10, 0x20,
        0x30, 0x40, 0x50, 0x60,
        0x70, 0x80, 0x90, 0xA0,
        0xB0, 0xC0, 0xD0, 0xE0,
        0xF0, 0x01, 0x02, 0x03
    };
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_uintN(val256, 256, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 32);
    cr_assert_eq(memcmp(buffer, val256, 32), 0, "256-bit data mismatch after serialization");
}

Test(ssz_serialize_uintN, invalid_bit_size)
{
    uint64_t dummy = 0x12345678ULL;
    uint8_t buffer[64];
    size_t out_size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err = ssz_serialize_uintN(&dummy, 999, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject invalid bit_size");
}

Test(ssz_serialize_uintN, null_pointers)
{
    uint8_t buffer[64];
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_uintN(NULL, 32, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null value pointer");
    err = ssz_serialize_uintN(&out_size, 32, NULL, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null output buffer");
}

Test(ssz_serialize_uintN, insufficient_buffer)
{
    uint64_t val64 = 0xFFFFFFFFFFFFFFFFULL;
    uint8_t small_buffer[2];
    size_t small_out_size = sizeof(small_buffer);
    ssz_error_t err = ssz_serialize_uintN(&val64, 64, small_buffer, &small_out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject when out_size is too small");
}

Test(ssz_serialize_boolean, false_value)
{
    uint8_t buffer[2];
    memset(buffer, 0xAB, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_boolean(false, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 1);
    cr_assert_eq(buffer[0], 0x00, "False boolean should serialize to 0x00");
}

Test(ssz_serialize_boolean, true_value)
{
    uint8_t buffer[2];
    memset(buffer, 0xAB, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_boolean(true, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 1);
    cr_assert_eq(buffer[0], 0x01, "True boolean should serialize to 0x01");
}

Test(ssz_serialize_boolean, null_pointers_and_insufficient)
{
    uint8_t buffer[2];
    size_t out_size = 0;
    bool val = true;
    ssz_error_t err = ssz_serialize_boolean(val, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject zero out_size");
    err = ssz_serialize_boolean(val, NULL, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null output buffer");
}

Test(ssz_serialize_bitvector, basic_10bit)
{
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    bool bits[10];
    memset(bits, false, sizeof(bits));
    bits[1] = true;
    bits[3] = true;
    bits[5] = true;
    bits[6] = true;
    bits[7] = true;
    bits[9] = true;
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_bitvector(bits, 10, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 2);
    cr_assert_eq(buffer[0], (uint8_t)0xEA, "Wrong first byte for 10-bit bitvector");
    cr_assert_eq(buffer[1], (uint8_t)0x02, "Wrong second byte for 10-bit bitvector");
}

Test(ssz_serialize_bitvector, zero_length)
{
    bool empty_bits[1];
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_bitvector(empty_bits, 0, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Zero-length bitvector should be rejected");
}

Test(ssz_serialize_bitvector, null_pointers)
{
    uint8_t buffer[16];
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_bitvector(NULL, 8, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null bits pointer");
    err = ssz_serialize_bitvector((bool*)buffer, 8, NULL, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null output buffer");
}

Test(ssz_serialize_bitvector, insufficient_out_size)
{
    bool bits[16];
    memset(bits, true, sizeof(bits));
    uint8_t small_buf[1];
    size_t small_sz = sizeof(small_buf);
    ssz_error_t err = ssz_serialize_bitvector(bits, 16, small_buf, &small_sz);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject insufficient out_size");
}

Test(ssz_serialize_bitlist, basic_10bit)
{
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    bool bits[10];
    memset(bits, false, sizeof(bits));
    bits[1] = true;
    bits[3] = true;
    bits[5] = true;
    bits[6] = true;
    bits[7] = true;
    bits[9] = true;
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_bitlist(bits, 10, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 2);
    cr_assert_eq(buffer[0], (uint8_t)0xEA);
    cr_assert_eq(buffer[1], (uint8_t)0x06, "Boundary bit not set properly for 10-bit bitlist");
}

Test(ssz_serialize_bitlist, empty_bitlist)
{
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    bool empty_bits[1];
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_bitlist(empty_bits, 0, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 1);
    cr_assert_eq(buffer[0], 0x01, "Empty bitlist should have just one boundary bit set");
}

Test(ssz_serialize_bitlist, null_pointers_and_insufficient)
{
    bool dummy[5];
    memset(dummy, false, sizeof(dummy));
    uint8_t buffer[16];
    size_t out_size = 0;
    ssz_error_t err = ssz_serialize_bitlist(dummy, 5, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject zero out_size");
    err = ssz_serialize_bitlist(NULL, 5, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject null bits pointer");
}

static ssz_error_t dummy_subserialize(const void* data, uint8_t* out_buf, size_t* out_size)
{
    if (!data || !out_buf || !out_size) return SSZ_ERROR_SERIALIZATION;
    const char* str = (const char*) data; 
    size_t len = strlen(str);
    if (*out_size < len) return SSZ_ERROR_SERIALIZATION;
    memcpy(out_buf, str, len);
    *out_size = len;
    return SSZ_SUCCESS;
}

Test(ssz_serialize_union, selector_zero_none_variant)
{
    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_union_t un;
    un.selector = 0;
    un.data = NULL;
    un.serialize_fn = NULL;
    ssz_error_t err = ssz_serialize_union(&un, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 1);
    cr_assert_eq(buffer[0], 0x00, "Union with selector=0 must be 0x00");
}

Test(ssz_serialize_union, selector_zero_but_non_null_data)
{
    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_union_t un;
    un.selector = 0;
    un.data = (void*)"Hello";
    un.serialize_fn = dummy_subserialize;
    ssz_error_t err = ssz_serialize_union(&un, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Union with selector=0 but non-null data must fail");
}

Test(ssz_serialize_union, non_zero_selector_no_sub_data)
{
    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_union_t un;
    un.selector = 5;
    un.data = NULL;
    un.serialize_fn = NULL;
    ssz_error_t err = ssz_serialize_union(&un, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 1);
    cr_assert_eq(buffer[0], 0x05, "Union with selector=5 but no data must be one byte [0x05]");
}

Test(ssz_serialize_union, sub_data_and_selector_10)
{
    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_union_t un;
    un.selector = 10;
    un.data = (void*)"Subdata";
    un.serialize_fn = dummy_subserialize;
    ssz_error_t err = ssz_serialize_union(&un, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    size_t expected_len = 1 + strlen("Subdata");
    cr_assert_eq(out_size, expected_len);
    cr_assert_eq(buffer[0], 0x0A);
    cr_assert_eq(memcmp(&buffer[1], "Subdata", strlen("Subdata")), 0,
                 "Union sub-data mismatch for 'Subdata'");
}

Test(ssz_serialize_union, invalid_selector_over_127)
{
    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_union_t un;
    un.selector = 200;
    un.data = NULL;
    un.serialize_fn = NULL;
    ssz_error_t err = ssz_serialize_union(&un, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Should reject union with selector > 127");
}

Test(ssz_serialize_vector, zero_element_vector)
{
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    uint8_t dummy_data[1];
    size_t dummy_sizes[1];
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_vector(dummy_data, 0, dummy_sizes, false, buffer, &out_size);
    cr_assert_eq(err, SSZ_ERROR_SERIALIZATION, "Vector with zero elements should fail");
}

Test(ssz_serialize_vector, fixed_3x4)
{
    uint8_t elements[12] = {
        0xDD, 0xCC, 0xBB, 0xAA, 
        0x44, 0x33, 0x22, 0x11, 
        0xEE, 0xCD, 0xAB, 0x99
    };
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    size_t element_sizes[3] = {4, 4, 4};
    ssz_error_t err = ssz_serialize_vector(elements, 3, element_sizes, false, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 12);
    cr_assert_eq(memcmp(buffer, elements, 12), 0, "Fixed-size vector mismatch in serialization");
}

Test(ssz_serialize_vector, varsize_3_elements)
{
    uint8_t elements[9] = {
        0xAA, 0xBB,
        0x01, 0x02, 0x03, 0x04,
        0x99, 0x88, 0x77
    };
    size_t element_sizes[3] = {2, 4, 3};
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_vector(elements, 3, element_sizes, true, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS, "Var-size vector call must succeed");
    cr_assert_eq(out_size, 21, "Unexpected serialized size for var-size vector with 3 elements");
    cr_assert(buffer[0] == 0x0C && buffer[4] == 0x0E && buffer[8] == 0x12,
              "Offsets in first 12 bytes are incorrect for variable-size vector");
    cr_assert(buffer[12] == 0xAA && buffer[13] == 0xBB,
              "Var-size vector's first payload incorrectly serialized");
    cr_assert(buffer[14] == 0x01 && buffer[17] == 0x04,
              "Var-size vector's second payload incorrectly serialized");
    cr_assert(buffer[18] == 0x99 && buffer[20] == 0x77,
              "Var-size vector's third payload incorrectly serialized");
}

Test(ssz_serialize_list, zero_element_list)
{
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    uint8_t dummy_data[1];
    size_t dummy_sizes[1];
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_list(dummy_data, 0, dummy_sizes, false, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS, "A zero-element list should be valid and produce 0 bytes");
    cr_assert_eq(out_size, 0);
}

Test(ssz_serialize_list, fixed_size_list_3_elements)
{
    uint8_t elements[12] = {
        0xDD, 0xCC, 0xBB, 0xAA, 
        0x44, 0x33, 0x22, 0x11, 
        0xEE, 0xCD, 0xAB, 0x99
    };
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t element_sizes[3] = {4, 4, 4};
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_list(elements, 3, element_sizes, false, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(out_size, 12);
    cr_assert_eq(memcmp(buffer, elements, 12), 0, "Fixed-size list mismatch");
}

Test(ssz_serialize_list, varsize_list_3_elements)
{
    uint8_t elements[9] = {
        0xAA, 0xBB,
        0x01, 0x02, 0x03, 0x04,
        0x99, 0x88, 0x77
    };
    size_t element_sizes[3] = {2, 4, 3};
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t out_size = sizeof(buffer);
    ssz_error_t err = ssz_serialize_list(elements, 3, element_sizes, true, buffer, &out_size);
    cr_assert_eq(err, SSZ_SUCCESS, "Var-size list call must succeed");
    cr_assert_eq(out_size, 21, "Unexpected serialized size for var-size list with 3 elements");
    cr_assert(buffer[0] == 0x0C && buffer[4] == 0x0E && buffer[8] == 0x12,
              "Offsets in first 12 bytes are incorrect for variable-size list");
    cr_assert(buffer[12] == 0xAA && buffer[13] == 0xBB,
              "Var-size list's first payload incorrectly serialized");
    cr_assert(buffer[14] == 0x01 && buffer[17] == 0x04,
              "Var-size list's second payload incorrectly serialized");
    cr_assert(buffer[18] == 0x99 && buffer[20] == 0x77,
              "Var-size list's third payload incorrectly serialized");
}

