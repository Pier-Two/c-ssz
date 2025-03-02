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
            0x67, 0x45, 0x23, 0x01};
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
            0xF0, 0x01, 0x02, 0x03};
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
            0x1C, 0x1D, 0x1E, 0x1F};
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

static void test_deserialize_vector_uint8(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint8 ---\n");
    printf("Testing 4-element uint8 vector round-trip...\n");
    {
        uint8_t data[4] = {0x11, 0x22, 0x33, 0x44};
        uint8_t serialized[16];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint8(data, 4, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint8 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[4];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint8(serialized, out_size, 4, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 4) == 0)
            {
                printf("  OK: Vector uint8 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint8 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_uint16(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint16 ---\n");
    printf("Testing 3-element uint16 vector round-trip...\n");
    {
        uint16_t data[3] = {0x1122, 0x3344, 0x5566};
        uint8_t serialized[32];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint16(data, 3, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint16 serialization failed.\n");
        }
        else
        {
            uint16_t recovered[3];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint16(serialized, out_size, 3, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 6) == 0)
            {
                printf("  OK: Vector uint16 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint16 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_uint32(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint32 ---\n");
    printf("Testing 3-element uint32 vector round-trip...\n");
    {
        uint32_t data[3] = {0xAABBCCDD, 0x11223344, 0x99ABCDEE};
        uint8_t serialized[64];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint32(data, 3, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint32 serialization failed.\n");
        }
        else
        {
            uint32_t recovered[3];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint32(serialized, out_size, 3, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 12) == 0)
            {
                printf("  OK: Vector uint32 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint32 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_uint64(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint64 ---\n");
    printf("Testing 2-element uint64 vector round-trip...\n");
    {
        uint64_t data[2] = {0x1122334455667788ULL, 0xAABBCCDDEEFF0011ULL};
        uint8_t serialized[64];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint64(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint64 serialization failed.\n");
        }
        else
        {
            uint64_t recovered[2];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint64(serialized, out_size, 2, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 16) == 0)
            {
                printf("  OK: Vector uint64 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint64 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_uint128(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint128 ---\n");
    printf("Testing 2-element uint128 vector round-trip...\n");
    {
        uint8_t data[32] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC,
            0xDD, 0xEE, 0xFF, 0x00,
            0x12, 0x34, 0x56, 0x78,
            0x9A, 0xBC, 0xDE, 0xF0,
            0x01, 0x02, 0x03, 0x04,
            0x05, 0x06, 0x07, 0x08};
        uint8_t serialized[64];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint128(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint128 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[32];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint128(serialized, out_size, 2, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 32) == 0)
            {
                printf("  OK: Vector uint128 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint128 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_uint256(void)
{
    printf("\n--- Testing ssz_deserialize_vector_uint256 ---\n");
    printf("Testing 2-element uint256 vector round-trip...\n");
    {
        uint8_t data[64] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC,
            0xDD, 0xEE, 0xFF, 0x00,
            0x12, 0x34, 0x56, 0x78,
            0x9A, 0xBC, 0xDE, 0xF0,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x10, 0x20,
            0x30, 0x40, 0x50, 0x60,
            0x70, 0x80, 0x90, 0xA0,
            0xB0, 0xC0, 0xD0, 0xE0,
            0xF0, 0x01, 0x02, 0x03,
            0xDE, 0xAD, 0xBE, 0xEF,
            0xFE, 0xED, 0xFA, 0xCE,
            0x12, 0xFE, 0x34, 0x56,
            0x78, 0x9A, 0xBC, 0xDF};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_uint256(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector uint256 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[64];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_uint256(serialized, out_size, 2, recovered);
            if (derr == SSZ_SUCCESS && memcmp(data, recovered, 64) == 0)
            {
                printf("  OK: Vector uint256 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector uint256 round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_vector_bool(void)
{
    printf("\n--- Testing ssz_deserialize_vector_bool ---\n");
    printf("Testing 5-element bool vector round-trip...\n");
    {
        bool data[5] = {true, false, true, true, false};
        uint8_t serialized[16];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector_bool(data, 5, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: Vector bool serialization failed.\n");
        }
        else
        {
            bool recovered[5];
            memset(recovered, 0, sizeof(recovered));
            ssz_error_t derr = ssz_deserialize_vector_bool(serialized, out_size, 5, recovered);
            if (derr == SSZ_SUCCESS && compare_bool_arrays(data, recovered, 5))
            {
                printf("  OK: Vector bool round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: Vector bool round-trip failed.\n");
            }
        }
    }
}

static void test_deserialize_list_uint8(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint8 ---\n");
    printf("Testing 5-element list with max_length=10 round-trip...\n");
    {
        uint8_t data[5] = {1, 2, 3, 4, 5};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint8(data, 5, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint8 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[10];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint8(serialized, out_size, 10, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 5 && memcmp(data, recovered, 5) == 0)
            {
                printf("  OK: list uint8 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint8 round-trip failed.\n");
            }
        }
    }
    printf("Testing list with more elements than max_length...\n");
    {
        uint8_t buffer[10];
        for (int i = 0; i < 10; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 10;
        uint8_t recovered[5];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint8(buffer, out_size, 5, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
    printf("Testing null buffer...\n");
    {
        uint8_t recovered[5];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint8(NULL, 10, 5, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null buffer.\n");
        }
    }
    printf("Testing null out_elements...\n");
    {
        uint8_t buffer[5] = {1, 2, 3, 4, 5};
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint8(buffer, 5, 10, NULL, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_elements.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_elements.\n");
        }
    }
    printf("Testing null out_actual_count...\n");
    {
        uint8_t buffer[5] = {1, 2, 3, 4, 5};
        uint8_t recovered[10];
        ssz_error_t derr = ssz_deserialize_list_uint8(buffer, 5, 10, recovered, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_actual_count.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_actual_count.\n");
        }
    }
}

static void test_deserialize_list_uint16(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint16 ---\n");
    printf("Testing 3-element list with max_length=5 round-trip...\n");
    {
        uint16_t data[3] = {0x1122, 0x3344, 0x5566};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint16(data, 3, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint16 serialization failed.\n");
        }
        else
        {
            uint16_t recovered[5];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint16(serialized, out_size, 5, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 3 && memcmp(data, recovered, 6) == 0)
            {
                printf("  OK: list uint16 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint16 round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[8];
        for (int i = 0; i < 8; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 8;
        uint16_t recovered[2];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint16(buffer, out_size, 2, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
}

static void test_deserialize_list_uint32(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint32 ---\n");
    printf("Testing 3-element list with max_length=5 round-trip...\n");
    {
        uint32_t data[3] = {0xAABBCCDD, 0x11223344, 0x99ABCDEE};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint32(data, 3, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint32 serialization failed.\n");
        }
        else
        {
            uint32_t recovered[5];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint32(serialized, out_size, 5, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 3 && memcmp(data, recovered, 12) == 0)
            {
                printf("  OK: list uint32 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint32 round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[16];
        for (int i = 0; i < 16; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 16;
        uint32_t recovered[3];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint32(buffer, out_size, 3, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
}

static void test_deserialize_list_uint64(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint64 ---\n");
    printf("Testing 2-element list with max_length=4 round-trip...\n");
    {
        uint64_t data[2] = {0x1122334455667788ULL, 0xAABBCCDDEEFF0011ULL};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint64(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint64 serialization failed.\n");
        }
        else
        {
            uint64_t recovered[4];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint64(serialized, out_size, 4, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(data, recovered, 16) == 0)
            {
                printf("  OK: list uint64 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint64 round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[24];
        for (int i = 0; i < 24; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 24;
        uint64_t recovered[2];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint64(buffer, out_size, 2, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
}

static void test_deserialize_list_uint128(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint128 ---\n");
    printf("Testing 2-element list with max_length=4 round-trip...\n");
    {
        uint8_t data[32] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC,
            0xDD, 0xEE, 0xFF, 0x00,
            0x12, 0x34, 0x56, 0x78,
            0x9A, 0xBC, 0xDE, 0xF0,
            0x01, 0x02, 0x03, 0x04,
            0x05, 0x06, 0x07, 0x08};
        uint8_t serialized[128];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint128(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint128 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[64];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint128(serialized, out_size, 4, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(data, recovered, 32) == 0 && memcmp(data + 16, recovered + 16, 16) == 0)
            {
                printf("  OK: list uint128 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint128 round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[48];
        for (int i = 0; i < 48; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 48;
        uint8_t recovered[32];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint128(buffer, out_size, 2, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
}

static void test_deserialize_list_uint256(void)
{
    printf("\n--- Testing ssz_deserialize_list_uint256 ---\n");
    printf("Testing 2-element list with max_length=4 round-trip...\n");
    {
        uint8_t data[64] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC,
            0xDD, 0xEE, 0xFF, 0x00,
            0x12, 0x34, 0x56, 0x78,
            0x9A, 0xBC, 0xDE, 0xF0,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x10, 0x20,
            0x30, 0x40, 0x50, 0x60,
            0x70, 0x80, 0x90, 0xA0,
            0xB0, 0xC0, 0xD0, 0xE0,
            0xF0, 0x01, 0x02, 0x03,
            0xDE, 0xAD, 0xBE, 0xEF,
            0xFE, 0xED, 0xFA, 0xCE,
            0x12, 0xFE, 0x34, 0x56,
            0x78, 0x9A, 0xBC, 0xDF};
        uint8_t serialized[256];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_uint256(data, 2, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list uint256 serialization failed.\n");
        }
        else
        {
            uint8_t recovered[128];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_uint256(serialized, out_size, 4, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(data, recovered, 64) == 0)
            {
                printf("  OK: list uint256 round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list uint256 round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[192];
        for (int i = 0; i < 192; i++)
            buffer[i] = (uint8_t)i;
        size_t out_size = 192;
        uint8_t recovered[64];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_uint256(buffer, out_size, 2, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
}

static void test_deserialize_list_bool(void)
{
    printf("\n--- Testing ssz_deserialize_list_bool ---\n");
    printf("Testing 6-element list with max_length=10 round-trip...\n");
    {
        bool data[6] = {true, false, true, true, false, true};
        uint8_t serialized[16];
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_list_bool(data, 6, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("  FAIL: list bool serialization failed.\n");
        }
        else
        {
            bool recovered[10];
            memset(recovered, 0, sizeof(recovered));
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list_bool(serialized, out_size, 10, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 6 && compare_bool_arrays(data, recovered, 6))
            {
                printf("  OK: list bool round-trip succeeded.\n");
            }
            else
            {
                printf("  FAIL: list bool round-trip failed.\n");
            }
        }
    }
    printf("Testing list exceeding max_length...\n");
    {
        uint8_t buffer[2] = {0xFF, 0xFF};
        bool recovered[1];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_bool(buffer, 2, 1, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected list that exceeds max_length.\n");
        }
        else
        {
            printf("  FAIL: Did not reject list that exceeds max_length.\n");
        }
    }
    printf("Testing null buffer...\n");
    {
        bool recovered[5];
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_bool(NULL, 5, 5, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null buffer.\n");
        }
    }
    printf("Testing null out_elements...\n");
    {
        uint8_t buffer[1] = {0x01};
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list_bool(buffer, 1, 5, NULL, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_elements.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_elements.\n");
        }
    }
    printf("Testing null out_actual_count...\n");
    {
        uint8_t buffer[1] = {0x01};
        bool recovered[5];
        ssz_error_t derr = ssz_deserialize_list_bool(buffer, 1, 5, recovered, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
        {
            printf("  OK: Rejected null out_actual_count.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null out_actual_count.\n");
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
    test_deserialize_vector_uint8();
    test_deserialize_vector_uint16();
    test_deserialize_vector_uint32();
    test_deserialize_vector_uint64();
    test_deserialize_vector_uint128();
    test_deserialize_vector_uint256();
    test_deserialize_vector_bool();
    test_deserialize_list_uint8();
    test_deserialize_list_uint16();
    test_deserialize_list_uint32();
    test_deserialize_list_uint64();
    test_deserialize_list_uint128();
    test_deserialize_list_uint256();
    test_deserialize_list_bool();
    
    return 0;
}