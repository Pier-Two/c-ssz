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
    printf("Running tests for ssz_deserialize_uintN...\n");

    {
        printf("  Checking valid 32-bit deserialization...\n");
        uint8_t buf[4] = {0xDD, 0xCC, 0xBB, 0xAA};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uintN(buf, 4, 32, &value);
        if (err == SSZ_SUCCESS && value == 0xAABBCCDDULL)
            printf("    Success: 0xAABBCCDD deserialized correctly.\n");
        else
            printf("    Failure: 32-bit direct test.\n");
    }

    {
        printf("  Checking valid 64-bit deserialization...\n");
        uint8_t buf[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uintN(buf, 8, 64, &value);
        if (err == SSZ_SUCCESS && value == 0x1122334455667788ULL)
            printf("    Success: 0x1122334455667788 deserialized correctly.\n");
        else
            printf("    Failure: 64-bit direct test.\n");
    }

    {
        printf("  Checking valid 8-bit deserialization...\n");
        uint8_t buf[1] = {0x7F};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uintN(buf, 1, 8, &value);
        if (err == SSZ_SUCCESS && value == 0x7F)
            printf("    Success: 0x7F deserialized correctly as 8-bit.\n");
        else
            printf("    Failure: 8-bit test.\n");
    }

    {
        printf("  Checking invalid bit_size...\n");
        uint8_t buf[4] = {0x01, 0x02, 0x03, 0x04};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uintN(buf, 4, 12, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught invalid bit_size 12.\n");
        else
            printf("    Failure: did not catch invalid bit_size.\n");
    }

    {
        printf("  Checking buffer too small for bit_size...\n");
        uint8_t buf[1] = {0x01};
        uint64_t value = 0;
        ssz_error_t err = ssz_deserialize_uintN(buf, 1, 32, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught small buffer size for 32 bits.\n");
        else
            printf("    Failure: did not catch buffer underflow.\n");
    }

    {
        printf("  Checking null buffer handling...\n");
        uint64_t value = 123;
        ssz_error_t err = ssz_deserialize_uintN(NULL, 1, 8, &value);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null buffer.\n");
        else
            printf("    Failure: did not catch null buffer.\n");
    }

    {
        printf("  Checking null out_value handling...\n");
        uint8_t buf[1] = {0xFF};
        ssz_error_t err = ssz_deserialize_uintN(buf, 1, 8, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_value.\n");
        else
            printf("    Failure: did not catch null out_value.\n");
    }
}

static void test_deserialize_boolean(void)
{
    printf("\nRunning tests for ssz_deserialize_boolean...\n");

    {
        printf("  Checking valid false (0x00)...\n");
        uint8_t buf[1] = {0x00};
        bool val = true;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_SUCCESS && val == false)
            printf("    Success: false handled correctly.\n");
        else
            printf("    Failure: false handling.\n");
    }

    {
        printf("  Checking valid true (0x01)...\n");
        uint8_t buf[1] = {0x01};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_SUCCESS && val == true)
            printf("    Success: true handled correctly.\n");
        else
            printf("    Failure: true handling.\n");
    }

    {
        printf("  Checking invalid boolean value (0x02)...\n");
        uint8_t buf[1] = {0x02};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught invalid boolean.\n");
        else
            printf("    Failure: did not catch invalid boolean.\n");
    }

    {
        printf("  Checking empty buffer...\n");
        uint8_t buf[1] = {0x01};
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(buf, 0, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught empty buffer.\n");
        else
            printf("    Failure: did not catch empty buffer.\n");
    }

    {
        printf("  Checking null buffer handling...\n");
        bool val = false;
        ssz_error_t err = ssz_deserialize_boolean(NULL, 1, &val);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null buffer.\n");
        else
            printf("    Failure: did not catch null buffer.\n");
    }

    {
        printf("  Checking null out_value handling...\n");
        uint8_t buf[1] = {0x00};
        ssz_error_t err = ssz_deserialize_boolean(buf, 1, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_value.\n");
        else
            printf("    Failure: did not catch null out_value.\n");
    }
}

static void test_deserialize_bitvector(void)
{
    printf("\nRunning tests for ssz_deserialize_bitvector...\n");

    {
        printf("  Checking bitvector with 10 bits and partial second byte...\n");
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
            printf("    Failure: could not serialize bitvector with 10 bits.\n");
        }
        else
        {
            bool recovered[10] = {false};
            ssz_error_t derr = ssz_deserialize_bitvector(buffer, out_size, 10, recovered);
            if (derr == SSZ_SUCCESS && compare_bool_arrays(original, recovered, 10))
                printf("    Success: 10-bit bitvector round-trip.\n");
            else
                printf("    Failure: 10-bit bitvector mismatch.\n");
        }
    }

    {
        printf("  Checking mismatch buffer size for 12-bit bitvector...\n");
        bool dummy[12] = {false};
        dummy[11] = true;
        uint8_t buffer[2] = {0};
        size_t out_size = sizeof(buffer);
        ssz_error_t serr = ssz_serialize_bitvector(dummy, 12, buffer, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("    Failure: expected success in serialization.\n");
        }
        else
        {
            // Now deserialize with a deliberately incorrect buffer_size
            // to see if it catches the mismatch.
            bool recovered[12] = {false};
            ssz_error_t derr = ssz_deserialize_bitvector(buffer, 1, 12, recovered);
            if (derr == SSZ_ERROR_DESERIALIZATION)
                printf("    Success: detected buffer size mismatch.\n");
            else
                printf("    Failure: did not detect mismatch.\n");
        }
    }

    {
        printf("  Checking null buffer handling...\n");
        bool recovered[8] = {false};
        ssz_error_t derr = ssz_deserialize_bitvector(NULL, 1, 8, recovered);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null buffer.\n");
        else
            printf("    Failure: did not catch null buffer.\n");
    }

    {
        printf("  Checking null out_bits handling...\n");
        uint8_t buf[1] = {0xAA};
        ssz_error_t derr = ssz_deserialize_bitvector(buf, 1, 8, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_bits.\n");
        else
            printf("    Failure: did not catch null out_bits.\n");
    }
}

static void test_deserialize_bitlist(void)
{
    printf("\nRunning tests for ssz_deserialize_bitlist...\n");

    {
        printf("  Checking a 10-bit bitlist with last boundary bit...\n");
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
            printf("    Failure: could not serialize 10-bit bitlist.\n");
        }
        else
        {
            bool recovered[10] = {false};
            size_t actual_bits = 0;
            ssz_error_t derr = ssz_deserialize_bitlist(buffer, out_size, 10, recovered, &actual_bits);
            if (derr == SSZ_SUCCESS && actual_bits == 10 && compare_bool_arrays(original, recovered, 10))
                printf("    Success: 10-bit bitlist round-trip.\n");
            else
                printf("    Failure: 10-bit bitlist mismatch.\n");
        }
    }

    {
        printf("  Checking boundary bit out of max_bits range...\n");
        uint8_t buffer[2] = {0xFF, 0xFF};
        bool recovered[8] = {false};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 2, 7, recovered, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught out-of-range boundary bit.\n");
        else
            printf("    Failure: did not catch out-of-range.\n");
    }

    {
        printf("  Checking empty buffer handling...\n");
        bool recovered[10] = {false};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(NULL, 0, 10, recovered, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught empty or null buffer.\n");
        else
            printf("    Failure: did not catch empty buffer.\n");
    }

    {
        printf("  Checking null out_bits handling...\n");
        uint8_t buffer[1] = {0x01};
        size_t actual_bits = 0;
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, NULL, &actual_bits);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_bits.\n");
        else
            printf("    Failure: did not catch null out_bits.\n");
    }

    {
        printf("  Checking null out_actual_bits handling...\n");
        uint8_t buffer[1] = {0x01};
        bool out_bits[8] = {false};
        ssz_error_t derr = ssz_deserialize_bitlist(buffer, 1, 8, out_bits, NULL);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_actual_bits.\n");
        else
            printf("    Failure: did not catch null out_actual_bits.\n");
    }
}

static void test_deserialize_union(void)
{
    printf("\nRunning tests for ssz_deserialize_union...\n");

    {
        printf("  Checking selector=0 (None) deserialization...\n");
        uint8_t buffer[1] = {0x00};
        ssz_union_t un;
        un.deserialize_fn = NULL;
        ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
        if (err == SSZ_SUCCESS && un.selector == 0 && un.data == NULL)
            printf("    Success: None union.\n");
        else
            printf("    Failure: union None test.\n");
    }

    {
        printf("  Checking selector > 127 (invalid)...\n");
        uint8_t buffer[1] = {0xFF};
        ssz_union_t un;
        un.deserialize_fn = NULL;
        ssz_error_t err = ssz_deserialize_union(buffer, 1, &un);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught invalid selector > 127.\n");
        else
            printf("    Failure: did not catch invalid selector.\n");
    }

    {
        printf("  Checking sub-type deserialization callback...\n");
        ssz_union_t un;
        un.deserialize_fn = union_subtype_cb;
        uint8_t buffer[2] = {0x01, 0xAA};
        ssz_error_t err = ssz_deserialize_union(buffer, 2, &un);
        if (err == SSZ_SUCCESS && un.selector == 1)
            printf("    Success: union with sub-type callback.\n");
        else
            printf("    Failure: union sub-type callback test.\n");
    }

    {
        printf("  Checking null out_union handling...\n");
        uint8_t buffer[2] = {0x01, 0xBB};
        ssz_error_t err = ssz_deserialize_union(buffer, 2, NULL);
        if (err == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught null out_union.\n");
        else
            printf("    Failure: did not catch null out_union.\n");
    }
}

static void test_deserialize_vector(void)
{
    printf("\nRunning tests for ssz_deserialize_vector...\n");

    {
        printf("  Checking fixed-size vector with three 4-byte elements...\n");
        uint8_t elements[12] = {
            0xDD, 0xCC, 0xBB, 0xAA,
            0x44, 0x33, 0x22, 0x11,
            0xEE, 0xCD, 0xAB, 0x99};
        size_t field_sizes[3] = {4, 4, 4};

        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector(elements, 3, field_sizes, false, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("    Failure: serialization of fixed-size vector.\n");
        }
        else
        {
            uint8_t recovered[12] = {0};
            ssz_error_t derr = ssz_deserialize_vector(serialized, out_size, 3, field_sizes, false, recovered);
            if (derr == SSZ_SUCCESS && memcmp(elements, recovered, 12) == 0)
                printf("    Success: fixed-size vector round-trip.\n");
            else
                printf("    Failure: fixed-size vector mismatch.\n");
        }
    }

    {
        printf("  Checking variable-size vector with different element sizes...\n");
        uint8_t elements[9] = {
            0xAA, 0xBB,            // 2 bytes
            0x11, 0x22, 0x33,      // 3 bytes
            0x99, 0x88, 0x77, 0x66 // 4 bytes
        };
        size_t field_sizes[3] = {2, 3, 4};

        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);
        ssz_error_t serr = ssz_serialize_vector(elements, 3, field_sizes, true, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("    Failure: serialization of variable-size vector.\n");
        }
        else
        {
            uint8_t recovered[9] = {0};
            ssz_error_t derr = ssz_deserialize_vector(serialized, out_size, 3, field_sizes, true, recovered);
            if (derr == SSZ_SUCCESS && memcmp(elements, recovered, 9) == 0)
                printf("    Success: variable-size vector round-trip.\n");
            else
                printf("    Failure: variable-size vector mismatch.\n");
        }
    }

    {
        printf("  Checking vector with zero elements (illegal in SSZ, but verifying error handling)...\n");
        size_t field_sizes[1] = {4};
        uint8_t out_data[4];
        ssz_error_t derr = ssz_deserialize_vector(NULL, 0, 0, field_sizes, false, out_data);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: caught zero-length vector.\n");
        else
            printf("    Failure: did not catch zero-length vector.\n");
    }
}

static void test_deserialize_list(void)
{
    printf("\nRunning tests for ssz_deserialize_list...\n");

    {
        printf("  Checking fixed-size list up to 3 elements, actually 2 used...\n");
        uint8_t elements[8] = {
            0xDE, 0xAD, 0xBE, 0xEF,
            0x11, 0x22, 0x33, 0x44};
        size_t field_sizes[3] = {4, 4, 4};
        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);

        ssz_error_t serr = ssz_serialize_list(elements, 2, field_sizes, false, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("    Failure: could not serialize partial fixed-size list.\n");
        }
        else
        {
            uint8_t recovered[12] = {0};
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list(serialized, out_size, 3, field_sizes, false, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(recovered, elements, 8) == 0)
                printf("    Success: fixed-size list partial parse.\n");
            else
                printf("    Failure: fixed-size list mismatch.\n");
        }
    }

    {
        printf("  Checking variable-size list up to 3 elements, actually 2 used...\n");
        uint8_t elements[5] = {
            0xAA, 0xBB,
            0x11, 0x22, 0x33};
        size_t field_sizes[3] = {2, 3, 4};
        uint8_t serialized[64] = {0};
        size_t out_size = sizeof(serialized);

        ssz_error_t serr = ssz_serialize_list(elements, 2, field_sizes, true, serialized, &out_size);
        if (serr != SSZ_SUCCESS)
        {
            printf("    Failure: could not serialize partial variable-size list.\n");
        }
        else
        {
            uint8_t recovered[9] = {0};
            size_t actual_count = 0;
            ssz_error_t derr = ssz_deserialize_list(serialized, out_size, 3, field_sizes, true, recovered, &actual_count);
            if (derr == SSZ_SUCCESS && actual_count == 2 && memcmp(recovered, elements, 5) == 0)
                printf("    Success: variable-size list partial parse.\n");
            else
                printf("    Failure: variable-size list mismatch.\n");
        }
    }

    {
        printf("  Checking list with invalid offsets to ensure error detection...\n");

        uint8_t bad_serial[8] = {0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
        size_t field_sizes[2] = {1, 1};
        uint8_t recovered[2] = {0};
        size_t actual_count = 0;
        ssz_error_t derr = ssz_deserialize_list(bad_serial, 8, 2, field_sizes, true, recovered, &actual_count);
        if (derr == SSZ_ERROR_DESERIALIZATION)
            printf("    Success: detected out-of-order offsets.\n");
        else
            printf("    Failure: did not detect offset error.\n");
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

    printf("\nFinished running ssz_deserialization tests.\n");
    return 0;
}
