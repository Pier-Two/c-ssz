#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "ssz_deserialize.h"
#include "ssz_serialize.h"
#include "ssz_constants.h"
#include "ssz_types.h"

static bool compare_bool_arrays(const bool *arr1, const bool *arr2, size_t len)
{
    for (size_t i = 0; i < len; i++)
        if (arr1[i] != arr2[i]) return false;
    return true;
}

static ssz_error_t union_subtype_cb(const uint8_t *b, size_t bs, void **out_data)
{
    if (bs < 1) return SSZ_ERROR_DESERIALIZATION;
    if (b[0] != 0xAA) return SSZ_ERROR_DESERIALIZATION;
    *out_data = NULL;
    return SSZ_SUCCESS;
}

Test(ssz_deserialize_uintN, valid_32bit)
{
    uint8_t buf[4] = {0xDD, 0xCC, 0xBB, 0xAA};
    uint64_t value = 0;
    ssz_error_t err = ssz_deserialize_uintN(buf, 4, 32, &value);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(value, 0xAABBCCDDULL, "32-bit value not deserialized correctly");
}

Test(ssz_deserialize_uintN, valid_64bit)
{
    uint8_t buf[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    uint64_t value = 0;
    ssz_error_t err = ssz_deserialize_uintN(buf, 8, 64, &value);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(value, 0x1122334455667788ULL, "64-bit value not deserialized correctly");
}

Test(ssz_deserialize_uintN, valid_8bit)
{
    uint8_t buf[1] = {0x7F};
    uint64_t value = 0;
    ssz_error_t err = ssz_deserialize_uintN(buf, 1, 8, &value);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(value, 0x7F, "8-bit value mismatch");
}

Test(ssz_deserialize_uintN, invalid_bit_size)
{
    uint8_t buf[4] = {0x01, 0x02, 0x03, 0x04};
    uint64_t value = 0;
    ssz_error_t err = ssz_deserialize_uintN(buf, 4, 12, &value);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject invalid bit_size of 12");
}

Test(ssz_deserialize_uintN, buffer_too_small)
{
    uint8_t buf[1] = {0x01};
    uint64_t value = 0;
    ssz_error_t err = ssz_deserialize_uintN(buf, 1, 32, &value);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject small buffer for 32 bits");
}

Test(ssz_deserialize_uintN, null_buffer)
{
    uint64_t value = 123;
    ssz_error_t err = ssz_deserialize_uintN(NULL, 1, 8, &value);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject null buffer");
}

Test(ssz_deserialize_uintN, null_out_value)
{
    uint8_t buf[1] = {0xFF};
    ssz_error_t err = ssz_deserialize_uintN(buf, 1, 8, NULL);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject null out_value");
}

Test(ssz_deserialize_boolean, valid_false)
{
    uint8_t buf[1] = {0x00};
    bool val = true;
    ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(val, false, "False boolean not deserialized correctly");
}

Test(ssz_deserialize_boolean, valid_true)
{
    uint8_t buf[1] = {0x01};
    bool val = false;
    ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(val, true, "True boolean not deserialized correctly");
}

Test(ssz_deserialize_boolean, invalid_boolean_value)
{
    uint8_t buf[1] = {0x02};
    bool val = false;
    ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject boolean value 0x02");
}

Test(ssz_deserialize_boolean, empty_buffer)
{
    uint8_t buf[1] = {0x01};
    bool val = false;
    ssz_error_t err = ssz_deserialize_boolean(buf, 0, &val);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject empty buffer");
}

Test(ssz_deserialize_boolean, null_buffer)
{
    bool val = false;
    ssz_error_t err = ssz_deserialize_boolean(NULL, 1, &val);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject null buffer");
}

Test(ssz_deserialize_boolean, null_out_value)
{
    uint8_t buf[1] = {0x00};
    ssz_error_t err = ssz_deserialize_boolean(buf, 1, NULL);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject null out_value");
}

Test(ssz_deserialize_bitvector, basic_10bit_round_trip)
{
    bool original[10] = {false};
    original[1] = true;
    original[3] = true;
    original[5] = true;
    original[6] = true;
    original[7] = true;
    original[9] = true;

    uint8_t buffer[2] = {0};
    size_t out_size = sizeof(buffer);
    ssz_error_t serr = ssz_serialize_bitvector(original, 10, buffer, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS, "Serialize of 10-bit bitvector must succeed");
    bool recovered[10] = {false};
    ssz_error_t derr = ssz_deserialize_bitvector(buffer, out_size, 10, recovered);
    cr_assert_eq(derr, SSZ_SUCCESS, "Deserialize of 10-bit bitvector must succeed");
    cr_assert(compare_bool_arrays(original, recovered, 10), "Mismatch in 10-bit bitvector round-trip");
}

Test(ssz_deserialize_bitvector, mismatch_buffer_size)
{
    bool dummy[12] = {false};
    dummy[11] = true;
    uint8_t buffer[2] = {0};
    size_t out_size = sizeof(buffer);
    ssz_error_t serr = ssz_serialize_bitvector(dummy, 12, buffer, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS, "Serialization for 12-bit bitvector must succeed");
    bool recovered[12] = {false};
    ssz_error_t derr = ssz_deserialize_bitvector(buffer, 1, 12, recovered);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Should fail with incorrect buffer size");
}

Test(ssz_deserialize_bitvector, null_buffer)
{
    bool recovered[8] = {false};
    ssz_error_t derr = ssz_deserialize_bitvector(NULL, 1, 8, recovered);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must reject null buffer for bitvector");
}

Test(ssz_deserialize_bitvector, null_out_bits)
{
    uint8_t buf[1] = {0xAA};
    ssz_error_t derr = ssz_deserialize_bitvector(buf, 1, 8, NULL);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must reject null out_bits pointer");
}

Test(ssz_deserialize_bitlist, basic_10bit_round_trip)
{
    bool original[10] = {false};
    original[1] = true;
    original[3] = true;
    original[5] = true;
    original[6] = true;
    original[7] = true;
    original[9] = true;

    uint8_t buffer[2] = {0};
    size_t out_size = sizeof(buffer);
    ssz_error_t serr = ssz_serialize_bitlist(original, 10, buffer, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS, "Serialize of 10-bit bitlist must succeed");
    bool recovered[10] = {false};
    size_t actual_bits = 0;
    ssz_error_t derr = ssz_deserialize_bitlist(buffer, out_size, 10, recovered, &actual_bits);
    cr_assert_eq(derr, SSZ_SUCCESS, "Deserialize of 10-bit bitlist must succeed");
    cr_assert_eq(actual_bits, 10, "Bit count after deserialization must be 10");
    cr_assert(compare_bool_arrays(original, recovered, 10), "Mismatch in 10-bit bitlist round-trip");
}

Test(ssz_deserialize_bitlist, boundary_bit_out_of_range)
{
    uint8_t buffer[2] = {0xFF, 0xFF};
    bool recovered[8] = {false};
    size_t actual_bits = 0;
    ssz_error_t derr = ssz_deserialize_bitlist(buffer, 2, 7, recovered, &actual_bits);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must detect out-of-range boundary bit");
}

Test(ssz_deserialize_bitlist, empty_buffer)
{
    bool recovered[10] = {false};
    size_t actual_bits = 0;
    ssz_error_t derr = ssz_deserialize_bitlist(NULL, 0, 10, recovered, &actual_bits);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must reject empty or null buffer");
}

Test(ssz_deserialize_bitlist, null_out_bits)
{
    uint8_t buffer[1] = {0x01};
    size_t actual_bits = 0;
    ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, NULL, &actual_bits);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must reject null out_bits");
}

Test(ssz_deserialize_bitlist, null_out_actual_bits)
{
    uint8_t buffer[1] = {0x01};
    bool out_bits[8] = {false};
    ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, out_bits, NULL);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must reject null out_actual_bits");
}

Test(ssz_deserialize_union, selector_zero_none)
{
    uint8_t buffer[1] = {0x00};
    ssz_union_t un;
    un.deserialize_fn = NULL;
    ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
    cr_assert_eq(err, SSZ_SUCCESS);
    cr_assert_eq(un.selector, 0);
    cr_assert_null(un.data, "Union data must be NULL when selector=0");
}

Test(ssz_deserialize_union, invalid_selector_over_127)
{
    uint8_t buffer[1] = {0xFF};
    ssz_union_t un;
    un.deserialize_fn = NULL;
    ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject selector > 127");
}

Test(ssz_deserialize_union, subtype_callback)
{
    uint8_t buffer[2] = {0x01, 0xAA};
    ssz_union_t un;
    un.deserialize_fn = union_subtype_cb;
    ssz_error_t err = ssz_deserialize_union(buffer, 2, &un);
    cr_assert_eq(err, SSZ_SUCCESS, "Sub-type callback must succeed if buffer[1] = 0xAA");
    cr_assert_eq(un.selector, 1);
}

Test(ssz_deserialize_union, null_out_union)
{
    uint8_t buffer[2] = {0x01, 0xBB};
    ssz_error_t err = ssz_deserialize_union(buffer, 2, NULL);
    cr_assert_eq(err, SSZ_ERROR_DESERIALIZATION, "Must reject null out_union pointer");
}

Test(ssz_deserialize_vector, fixed_3x4_round_trip)
{
    uint8_t elements[12] = {
        0xDD, 0xCC, 0xBB, 0xAA,
        0x44, 0x33, 0x22, 0x11,
        0xEE, 0xCD, 0xAB, 0x99
    };
    size_t field_sizes[3] = {4, 4, 4};
    uint8_t serialized[64];
    memset(serialized, 0, sizeof(serialized));
    size_t out_size = sizeof(serialized);

    ssz_error_t serr = ssz_serialize_vector(elements, 3, field_sizes, false, serialized, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS);
    uint8_t recovered[12];
    memset(recovered, 0, sizeof(recovered));
    ssz_error_t derr = ssz_deserialize_vector(serialized, out_size, 3, field_sizes, false, recovered);
    cr_assert_eq(derr, SSZ_SUCCESS);
    cr_assert_eq(memcmp(elements, recovered, 12), 0, "Fixed-size vector mismatch after round-trip");
}

Test(ssz_deserialize_vector, varsize_3_elements_round_trip)
{
    uint8_t elements[9] = {
        0xAA, 0xBB,
        0x11, 0x22, 0x33,
        0x99, 0x88, 0x77, 0x66
    };
    size_t field_sizes[3] = {2, 3, 4};
    uint8_t serialized[64];
    memset(serialized, 0, sizeof(serialized));
    size_t out_size = sizeof(serialized);

    ssz_error_t serr = ssz_serialize_vector(elements, 3, field_sizes, true, serialized, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS);
    uint8_t recovered[9];
    memset(recovered, 0, sizeof(recovered));
    ssz_error_t derr = ssz_deserialize_vector(serialized, out_size, 3, field_sizes, true, recovered);
    cr_assert_eq(derr, SSZ_SUCCESS);
    cr_assert_eq(memcmp(elements, recovered, 9), 0, "Var-size vector mismatch after round-trip");
}

Test(ssz_deserialize_vector, zero_length_vector)
{
    size_t field_sizes[1] = {4};
    uint8_t out_data[4];
    ssz_error_t derr = ssz_deserialize_vector(NULL, 0, 0, field_sizes, false, out_data);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Should reject zero-length vector (illegal in SSZ)");
}

Test(ssz_deserialize_list, fixed_size_list_partial)
{
    uint8_t elements[8] = {
        0xDE, 0xAD, 0xBE, 0xEF,
        0x11, 0x22, 0x33, 0x44
    };
    size_t field_sizes[3] = {4, 4, 4};
    uint8_t serialized[64];
    memset(serialized, 0, sizeof(serialized));
    size_t out_size = sizeof(serialized);

    ssz_error_t serr = ssz_serialize_list(elements, 2, field_sizes, false, serialized, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS, "Partial fixed-size list must serialize successfully");
    uint8_t recovered[12];
    memset(recovered, 0, sizeof(recovered));
    size_t actual_count = 0;
    ssz_error_t derr = ssz_deserialize_list(serialized, out_size, 3, field_sizes, false, recovered, &actual_count);
    cr_assert_eq(derr, SSZ_SUCCESS, "Deserialization of partial fixed-size list must succeed");
    cr_assert_eq(actual_count, 2, "List must contain 2 elements after partial parse");
    cr_assert_eq(memcmp(recovered, elements, 8), 0, "Mismatch in recovered data for partial fixed-size list");
}

Test(ssz_deserialize_list, varsize_list_partial)
{
    uint8_t elements[5] = {
        0xAA, 0xBB,
        0x11, 0x22, 0x33
    };
    size_t field_sizes[3] = {2, 3, 4};
    uint8_t serialized[64];
    memset(serialized, 0, sizeof(serialized));
    size_t out_size = sizeof(serialized);

    ssz_error_t serr = ssz_serialize_list(elements, 2, field_sizes, true, serialized, &out_size);
    cr_assert_eq(serr, SSZ_SUCCESS);
    uint8_t recovered[9];
    memset(recovered, 0, sizeof(recovered));
    size_t actual_count = 0;
    ssz_error_t derr = ssz_deserialize_list(serialized, out_size, 3, field_sizes, true, recovered, &actual_count);
    cr_assert_eq(derr, SSZ_SUCCESS);
    cr_assert_eq(actual_count, 2);
    cr_assert_eq(memcmp(recovered, elements, 5), 0, "Var-size list mismatch after partial parse");
}

Test(ssz_deserialize_list, invalid_offsets)
{
    uint8_t bad_serial[8] = {0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    size_t field_sizes[2] = {1, 1};
    uint8_t recovered[2] = {0};
    size_t actual_count = 0;
    ssz_error_t derr = ssz_deserialize_list(bad_serial, 8, 2, field_sizes, true, recovered, &actual_count);
    cr_assert_eq(derr, SSZ_ERROR_DESERIALIZATION, "Must detect out-of-order or invalid offsets");
}
