#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "ssz_deserialize.h"
#include "ssz_serialize.h"
#include "ssz_constants.h"

static bool compare_bool_arrays(const bool *arr1, const bool *arr2, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (arr1[i] != arr2[i])
            return false;
    }
    return true;
}

static ssz_error_t union_subtype_cb(const uint8_t *b, size_t bs, void **out_data)
{
    if (bs < 1)
        return SSZ_ERROR_DESERIALIZATION;
    if (b[0] != 0xAA)
        return SSZ_ERROR_DESERIALIZATION;
    *out_data = NULL;
    return SSZ_SUCCESS;
}

static void test_deserialize_uintN(void)
{
    printf("\n--- Testing ssz_deserialize_uintN ---\n");

    printf("Testing valid 8-bit deserialization...\n");
    {
        uint8_t buf[1] = {0xAB};
        uint8_t value = 0;
        ssz_error_t err = ssz_deserialize_uint8(buf, 1, &value);
        if (err == SSZ_SUCCESS && value == 0xAB)
        {
            printf("  OK: 8-bit value 0xAB deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 8-bit value deserialization failed.\n");
        }
    }

    printf("Testing 8-bit with null pointer...\n");
    {
        uint8_t buf[1] = {0xFF};
        ssz_error_t err = ssz_deserialize_uint8(buf, 1, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 8-bit null out_value rejected.\n");
        }
        else
        {
            printf("  FAIL: 8-bit null out_value was not rejected.\n");
        }
    }

    printf("Testing 8-bit with zero-size buffer...\n");
    {
        uint8_t buf[1] = {0xFF};
        uint8_t value = 0;
        ssz_error_t err = ssz_deserialize_uint8(buf, 0, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 8-bit zero buffer size rejected.\n");
        }
        else
        {
            printf("  FAIL: 8-bit zero buffer size was not rejected.\n");
        }
    }

    printf("Testing valid 16-bit deserialization...\n");
    {
        uint8_t buf[2] = {0xDD, 0xCC};
        uint16_t value = 0;
        ssz_error_t err = ssz_deserialize_uint16(buf, 2, &value);
        if (err == SSZ_SUCCESS && value == 0xCCDD)
        {
            printf("  OK: 16-bit value 0xCCDD deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 16-bit value deserialization failed.\n");
        }
    }

    printf("Testing 16-bit with insufficient buffer...\n");
    {
        uint8_t buf[1] = {0x12};
        uint16_t value = 0;
        ssz_error_t err = ssz_deserialize_uint16(buf, 1, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 16-bit insufficient buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 16-bit insufficient buffer was not rejected.\n");
        }
    }

    printf("Testing valid 32-bit deserialization...\n");
    {
        uint8_t buf[4] = {0xDD, 0xCC, 0xBB, 0xAA};
        uint32_t value = 0;
        ssz_error_t err = ssz_deserialize_uint32(buf, 4, &value);
        if (err == SSZ_SUCCESS && value == 0xAABBCCDDUL)
        {
            printf("  OK: 32-bit value 0xAABBCCDD deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 32-bit value deserialization failed.\n");
        }
    }

    printf("Testing 32-bit with null pointer...\n");
    {
        uint8_t buf[4] = {0xBA, 0xAD, 0xF0, 0x0D};
        ssz_error_t err = ssz_deserialize_uint32(buf, 4, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 32-bit null out_value rejected.\n");
        }
        else
        {
            printf("  FAIL: 32-bit null out_value was not rejected.\n");
        }
    }

    printf("Testing valid 64-bit deserialization...\n");
    {
        uint8_t buf[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uint64(buf, 8, &value);
        if (err == SSZ_SUCCESS && value == 0x1122334455667788ULL)
        {
            printf("  OK: 64-bit value 0x1122334455667788 deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 64-bit value deserialization failed.\n");
        }
    }

    printf("Testing 64-bit with insufficient buffer...\n");
    {
        uint8_t buf[7] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uint64(buf, 7, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 64-bit insufficient buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 64-bit insufficient buffer was not rejected.\n");
        }
    }

    printf("Testing valid 128-bit deserialization...\n");
    {
        uint8_t buf[16] = {
            0xEF, 0xCD, 0xAB, 0x89,
            0x67, 0x45, 0x23, 0x01,
            0xEF, 0xCD, 0xAB, 0x89,
            0x67, 0x45, 0x23, 0x01
        };
        uint8_t value[16];
        memset(value, 0, sizeof(value));
        ssz_error_t err = ssz_deserialize_uint128(buf, 16, value);
        if (err == SSZ_SUCCESS && memcmp(value, buf, 16) == 0)
        {
            printf("  OK: 128-bit value deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 128-bit value deserialization failed.\n");
        }
    }

    printf("Testing 128-bit with insufficient buffer...\n");
    {
        uint8_t buf[8] = {0x00};
        uint8_t value[16];
        ssz_error_t err = ssz_deserialize_uint128(buf, 8, value);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 128-bit insufficient buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 128-bit insufficient buffer was not rejected.\n");
        }
    }

    printf("Testing valid 256-bit deserialization...\n");
    {
        uint8_t buf[32] = {
            0x88, 0x77, 0x66, 0x55,
            0x44, 0x33, 0x22, 0x11,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x10, 0x20,
            0x30, 0x40, 0x50, 0x60,
            0x70, 0x80, 0x90, 0xA0,
            0xB0, 0xC0, 0xD0, 0xE0,
            0xF0, 0x01, 0x02, 0x03
        };
        uint8_t value[32];
        memset(value, 0, sizeof(value));
        ssz_error_t err = ssz_deserialize_uint256(buf, 32, value);
        if (err == SSZ_SUCCESS && memcmp(value, buf, 32) == 0)
        {
            printf("  OK: 256-bit value deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 256-bit value deserialization failed.\n");
        }
    }

    printf("Testing 256-bit with null pointer...\n");
    {
        uint8_t buf[32] = {
            0x00, 0x01, 0x02, 0x03,
            0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F,
            0x10, 0x11, 0x12, 0x13,
            0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B,
            0x1C, 0x1D, 0x1E, 0x1F
        };
        ssz_error_t err = ssz_deserialize_uint256(buf, 32, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: 256-bit null out_value rejected.\n");
        }
        else
        {
            printf("  FAIL: 256-bit null out_value was not rejected.\n");
        }
    }
}

static void test_deserialize_boolean(void)
{
    printf("\n--- Testing ssz_deserialize_boolean ---\n");

    printf("Testing false (0x00)...\n");
    {
        uint8_t buf[1] = {0x00};
        bool val = true;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_SUCCESS && val == false)
        {
            printf("  OK: False boolean deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: False boolean deserialization failed.\n");
        }
    }

    printf("Testing true (0x01)...\n");
    {
        uint8_t buf[1] = {0x01};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_SUCCESS && val == true)
        {
            printf("  OK: True boolean deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: True boolean deserialization failed.\n");
        }
    }

    printf("Testing invalid boolean value (0x02)...\n");
    {
        uint8_t buf[1] = {0x02};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected invalid boolean value.\n");
        }
        else
        {
            printf("  FAIL: Did not reject invalid boolean value.\n");
        }
    }

    printf("Testing empty buffer...\n");
    {
        uint8_t buf[1] = {0x01};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 0, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected empty buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject empty buffer.\n");
        }
    }

    printf("Testing null buffer handling...\n");
    {
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(NULL, 1, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null buffer.\n");
        }
    }

    printf("Testing null out_value handling...\n");
    {
        uint8_t buf[1] = {0x00};
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_value.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_value.\n");
        }
    }
}

static void test_deserialize_bitvector(void)
{
    printf("\n--- Testing ssz_deserialize_bitvector ---\n");

    printf("Testing a 10-bit bitvector...\n");
    {
        bool original[10] = {0};
        original[1] = true;
        original[3] = true;
        original[5] = true;
        original[6] = true;
        original[7] = true;
        original[9] = true;
        uint8_t buffer[2] = {0};
        size_t out_size = sizeof(buffer);
        ssz_error_t serr = ssz_serialize_bitvector(original, 10, buffer, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: 10-bit bitvector serialization failed.\n");
        }
        else
        {
            bool recovered[10] = {false};
            ssz_error_t derr = ssz_deserialize_bitvector(buffer, out_size, 10, recovered);
            if (derr == SSZ_SUCCESS && compare_bool_arrays(original, recovered, 10))
            {
                printf("  OK: 10-bit bitvector round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: 10-bit bitvector round-trip failed.\n");
            }
        }
    }

    printf("Testing mismatch buffer size for 12-bit bitvector...\n");
    {
        bool dummy[12] = {false};
        dummy[11] = true;
        uint8_t buffer[2] = {0};
        size_t out_size = sizeof(buffer);
        ssz_error_t serr = ssz_serialize_bitvector(dummy, 12, buffer, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Expected successful serialization of 12-bit bitvector.\n");
        }
        else
        {
            bool recovered[12] = {false};
            ssz_error_t derr = ssz_deserialize_bitvector(buffer, 1, 12, recovered);
            if (derr == SSZ_ERROR_DESERIALIZATION)
            {
                printf("  OK: Detected buffer size mismatch for 12-bit bitvector.\n");
            }
            else
            {
                printf("  FAIL: Did not detect mismatch for 12-bit bitvector.\n");
            }
        }
    }

    printf("Testing null buffer handling...\n");
    {
        bool recovered[8] = {false};
        ssz_error_t derr = ssz_deserialize_bitvector(NULL, 1, 8, recovered);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null buffer.\n");
        }
    }

    printf("Testing null out_bits handling...\n");
    {
        uint8_t buf[1] = {0xAA};
        ssz_error_t derr = ssz_deserialize_bitvector(buf, 1, 8, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_bits.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_bits.\n");
        }
    }
}

static void test_deserialize_bitlist(void)
{
    printf("\n--- Testing ssz_deserialize_bitlist ---\n");

    printf("Testing a 10-bit bitlist...\n");
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
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: 10-bit bitlist serialization failed.\n");
        }
        else
        {
            bool recovered[10] = {false};
            size_t actual_bits = 0;
            ssz_error_t derr = ssz_deserialize_bitlist(buffer, out_size, 10, recovered, &actual_bits);
            if (derr == SSZ_SUCCESS && actual_bits == 10 && compare_bool_arrays(original, recovered, 10))
            {
                printf("  OK: 10-bit bitlist round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: 10-bit bitlist round-trip failed.\n");
            }
        }
    }

    printf("Testing boundary bit out of max_bits range...\n");
    {
        uint8_t buffer[2] = {0xFF, 0xFF};
        bool recovered[8] = {false};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 2, 7, recovered, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected out-of-range boundary bit.\n");
        }
        else
        {
            printf("  FAIL: Did not reject out-of-range boundary bit.\n");
        }
    }

    printf("Testing empty buffer handling...\n");
    {
        bool recovered[10] = {false};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(NULL, 0, 10, recovered, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected empty or null buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject empty or null buffer.\n");
        }
    }

    printf("Testing null out_bits handling...\n");
    {
        uint8_t buffer[1] = {0x01};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, NULL, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_bits.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_bits.\n");
        }
    }

    printf("Testing null out_actual_bits handling...\n");
    {
        uint8_t buffer[1] = {0x01};
        bool out_bits[8] = {false};
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, out_bits, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_actual_bits.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_actual_bits.\n");
        }
    }
}

static void test_deserialize_union(void)
{
    printf("\n--- Testing ssz_deserialize_union ---\n");

    printf("Testing selector=0 (None) deserialization...\n");
    {
        uint8_t buffer[1] = {0x00};
        ssz_union_t un;
        un.deserialize_fn = NULL;
        ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
        if (err == SSZ_SUCCESS && un.selector == 0 && un.data == NULL)
        {
            printf("  OK: Union with selector=0 deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Union selector=0 deserialization failed.\n");
        }
    }

    printf("Testing selector > 127 (invalid)...\n");
    {
        uint8_t buffer[1] = {0xFF};
        ssz_union_t un;
        un.deserialize_fn = NULL;
        ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected invalid selector > 127.\n");
        }
        else
        {
            printf("  FAIL: Did not reject invalid selector.\n");
        }
    }

    printf("Testing sub-type deserialization callback...\n");
    {
        ssz_union_t un;
        un.deserialize_fn = union_subtype_cb;
        uint8_t buffer[2] = {0x01, 0xAA};
        ssz_error_t err = ssz_deserialize_union(buffer, 2, &un);
        if (err == SSZ_SUCCESS && un.selector == 1)
        {
            printf("  OK: Union with valid sub-type callback deserialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Union sub-type callback deserialization failed.\n");
        }
    }

    printf("Testing null out_union handling...\n");
    {
        uint8_t buffer[2] = {0x01, 0xBB};
        ssz_error_t err = ssz_deserialize_union(buffer, 2, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_union.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_union.\n");
        }
    }
}

static void test_deserialize_vector(void)
{
    printf("\n--- Testing ssz_deserialize_vector ---\n");

    printf("Testing fixed-size vector with three 4-byte elements...\n");
    {
        uint8_t elements[12] = {
            0xDD, 0xCC, 0xBB, 0xAA,
            0x44, 0x33, 0x22, 0x11,
            0xEE, 0xCD, 0xAB, 0x99};
        size_t field_sizes[3] = {4, 4, 4};
        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector(elements, 3, field_sizes, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector serialization failed.\n");
        }
        else
        {
            uint8_t recovered[12] = {0};
            ssz_error_t derr = ssz_deserialize_vector(serialized, out_size, 3, field_sizes, recovered);
            if (derr == SSZ_SUCCESS && memcmp(elements, recovered, 12) == 0)
            {
                printf("  OK: Fixed-size vector round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Fixed-size vector round-trip failed.\n");
            }
        }
    }

    printf("Testing zero-element vector...\n");
    {
        size_t field_sizes[1] = {4};
        uint8_t out_data[4];
        ssz_error_t derr = ssz_deserialize_vector(NULL, 0, 0, field_sizes, out_data);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected zero-length vector.\n");
        }
        else
        {
            printf("  FAIL: Did not reject zero-length vector.\n");
        }
    }
}

static void test_deserialize_list(void)
{
    printf("\n--- Testing ssz_deserialize_list ---\n");

    printf("Testing variable-size list up to 3 elements, with 2 used...\n");
    {
        uint8_t elements[5] = {
            0xAA, 0xBB,
            0x11, 0x22, 0x33};
        size_t field_sizes[3] = {2, 3, 4};
        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list(elements, 2, field_sizes, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Could not serialize partial variable-size list.\n");
        }
        else
        {
            uint8_t recovered[9] = {0};
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list(serialized, out_size, 3, field_sizes, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(recovered, elements, 5) == 0)
            {
                printf("  OK: Variable-size list partial parse succeeded.\n");
            }
            else
            {
                printf("  FAIL: Variable-size list partial parse failed.\n");
            }
        }
    }

    printf("Testing invalid offsets in list...\n");
    {
        uint8_t bad_serial[8] = {0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
        size_t field_sizes[2] = {1, 1};
        uint8_t recovered[2] = {0};
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list(bad_serial, 8, 2, field_sizes, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Detected out-of-order offsets.\n");
        }
        else
        {
            printf("  FAIL: Did not detect offset error.\n");
        }
    }
}

int main(void)
{
    test_deserialize_uintN();
    test_deserialize_boolean();
    test_deserialize_bitvector();
    test_deserialize_bitlist();
    test_deserialize_union();
    test_deserialize_vector();
    test_deserialize_list();
    return 0;
}