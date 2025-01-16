#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "ssz_serialize.h"
#include "ssz_types.h"

static void test_serialize_uintN(void)
{
    printf("\n--- Testing ssz_serialize_uintN ---\n");

    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing valid 32-bit serialization...\n");
    {
        uint64_t val32 = 0xAABBCCDDULL;
        out_size = sizeof(buffer);
        err = ssz_serialize_uintN(&val32, 32, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 4 && 
            buffer[0] == 0xDD && buffer[1] == 0xCC && buffer[2] == 0xBB && buffer[3] == 0xAA)
        {
            printf("  OK: 32-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 32-bit value serialization failed.\n");
        }
    }

    printf("Testing valid 64-bit serialization...\n");
    {
        uint64_t val64 = 0x1122334455667788ULL;
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uintN(&val64, 64, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 8 &&
            buffer[0] == 0x88 && buffer[1] == 0x77 && buffer[2] == 0x66 && buffer[3] == 0x55 &&
            buffer[4] == 0x44 && buffer[5] == 0x33 && buffer[6] == 0x22 && buffer[7] == 0x11)
        {
            printf("  OK: 64-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 64-bit value serialization failed.\n");
        }
    }

    printf("Testing valid 128-bit serialization...\n");
    {
        uint8_t val128[16] = {
            0xEF, 0xCD, 0xAB, 0x89,
            0x67, 0x45, 0x23, 0x01,
            0xEF, 0xCD, 0xAB, 0x89,
            0x67, 0x45, 0x23, 0x01
        };
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uintN(val128, 128, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 16 && memcmp(buffer, val128, 16) == 0)
        {
            printf("  OK: 128-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 128-bit value serialization failed.\n");
        }
    }

    printf("Testing valid 256-bit serialization...\n");
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
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uintN(val256, 256, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 32 && memcmp(buffer, val256, 32) == 0)
        {
            printf("  OK: 256-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 256-bit value serialization failed.\n");
        }
    }

    printf("Testing invalid bit_size...\n");
    {
        uint64_t dummy = 0x12345678;
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uintN(&dummy, 999, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected invalid bit_size.\n");
        }
        else
        {
            printf("  FAIL: Did not reject invalid bit_size.\n");
        }
    }

    printf("Testing null pointers...\n");
    {
        out_size = sizeof(buffer);
        err = ssz_serialize_uintN(NULL, 32, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null value pointer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null value pointer.\n");
        }
        err = ssz_serialize_uintN((void*)&out_size, 32, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null output buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null output buffer.\n");
        }
    }

    printf("Testing insufficient buffer space...\n");
    {
        uint64_t val64 = 0xFFFFFFFFFFFFFFFFULL;
        uint8_t small_buffer[2];
        size_t small_out_size = sizeof(small_buffer);
        err = ssz_serialize_uintN(&val64, 64, small_buffer, &small_out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected when out_size is too small.\n");
        }
        else
        {
            printf("  FAIL: Did not reject insufficient out_size.\n");
        }
    }
}

static void test_serialize_boolean(void)
{
    printf("\n--- Testing ssz_serialize_boolean ---\n");

    uint8_t buffer[2];
    memset(buffer, 0xAB, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing false serialization...\n");
    {
        out_size = sizeof(buffer);
        err = ssz_serialize_boolean(false, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0x00)
        {
            printf("  OK: False boolean serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: False boolean serialization failed.\n");
        }
    }

    printf("Testing true serialization...\n");
    {
        memset(buffer, 0xAB, sizeof(buffer));
        out_size = sizeof(buffer);
        err = ssz_serialize_boolean(true, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0x01)
        {
            printf("  OK: True boolean serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: True boolean serialization failed.\n");
        }
    }

    printf("Testing null pointers and insufficient size...\n");
    {
        bool val = true;
        out_size = 0;
        err = ssz_serialize_boolean(val, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected zero out_size.\n");
        }
        else
        {
            printf("  FAIL: Did not reject zero out_size.\n");
        }
        err = ssz_serialize_boolean(val, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null output buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null output buffer.\n");
        }
    }
}

static void test_serialize_bitvector(void)
{
    printf("\n--- Testing ssz_serialize_bitvector ---\n");

    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing a 10-bit bitvector with bits [1,3,5,6,7,9] = 1...\n");
    {
        bool bits[10];
        memset(bits, false, sizeof(bits));
        bits[1] = true;
        bits[3] = true;
        bits[5] = true;
        bits[6] = true;
        bits[7] = true;
        bits[9] = true;
        out_size = sizeof(buffer);
        err = ssz_serialize_bitvector(bits, 10, buffer, &out_size);

        if (err == SSZ_SUCCESS && out_size == 2 &&
            buffer[0] == 0xEA && buffer[1] == 0x02)
        {
            printf("  OK: 10-bit bitvector serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 10-bit bitvector serialization failed.\n");
        }
    }

    printf("Testing zero-bit bitvector (invalid by spec)...\n");
    {
        bool empty_bits[1]; 
        out_size = sizeof(buffer);
        // Attempt a zero-length bitvector
        err = ssz_serialize_bitvector(empty_bits, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-length bitvector rejected as expected.\n");
        }
        else
        {
            printf("  FAIL: Zero-length bitvector not rejected.\n");
        }
    }

    printf("Testing null pointers...\n");
    {
        out_size = sizeof(buffer);
        err = ssz_serialize_bitvector(NULL, 8, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null bits pointer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null bits pointer.\n");
        }
        err = ssz_serialize_bitvector((bool*)buffer, 8, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null output buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null output buffer.\n");
        }
    }

    printf("Testing insufficient out_size...\n");
    {
        bool bits[16];
        memset(bits, true, sizeof(bits));  // arbitrary data
        uint8_t small_buf[1];
        size_t small_sz = sizeof(small_buf); // 1 byte
        ssz_error_t err = ssz_serialize_bitvector(bits, 16, small_buf, &small_sz);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected insufficient out_size.\n");
        }
        else
        {
            printf("  FAIL: Did not reject insufficient out_size.\n");
        }
    }
}

static void test_serialize_bitlist(void)
{
    printf("\n--- Testing ssz_serialize_bitlist ---\n");

    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing a 10-bit bitlist with bits [1,3,5,6,7,9] = 1...\n");
    {
        bool bits[10];
        memset(bits, false, sizeof(bits));
        bits[1] = true;
        bits[3] = true;
        bits[5] = true;
        bits[6] = true;
        bits[7] = true;
        bits[9] = true;
        out_size = sizeof(buffer);
        err = ssz_serialize_bitlist(bits, 10, buffer, &out_size);

        if (err == SSZ_SUCCESS && out_size == 2 && 
            buffer[0] == 0xEA && buffer[1] == 0x06)
        {
            printf("  OK: 10-bit bitlist serialized correctly (boundary bit included).\n");
        }
        else
        {
            printf("  FAIL: 10-bit bitlist serialization failed.\n");
        }
    }

    printf("Testing empty bitlist (should just set boundary bit at index 0)...\n");
    {
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        bool empty_bits[1]; // not used, but we pass pointer
        err = ssz_serialize_bitlist(empty_bits, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0x01)
        {
            printf("  OK: Zero-length bitlist has one byte [0x01] for boundary bit.\n");
        }
        else
        {
            printf("  FAIL: Zero-length bitlist handling is incorrect.\n");
        }
    }

    printf("Testing null pointers and insufficient buffer...\n");
    {
        bool dummy[5];
        memset(dummy, false, sizeof(dummy));
        out_size = 0;
        err = ssz_serialize_bitlist(dummy, 5, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected zero out_size.\n");
        }
        else
        {
            printf("  FAIL: Did not reject zero out_size.\n");
        }
        err = ssz_serialize_bitlist(NULL, 5, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null bits pointer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null bits pointer.\n");
        }
    }
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

static void test_serialize_union(void)
{
    printf("\n--- Testing ssz_serialize_union ---\n");

    uint8_t buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing union with selector=0 => None variant...\n");
    {
        ssz_union_t un;
        un.selector = 0;
        un.data = NULL;
        un.serialize_fn = NULL;
        out_size = sizeof(buffer);
        err = ssz_serialize_union(&un, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0x00)
        {
            printf("  OK: Union with selector=0 and no data is correct.\n");
        }
        else
        {
            printf("  FAIL: Union with selector=0 did not serialize properly.\n");
        }
    }

    printf("Testing union with selector=0 but non-null data => should fail...\n");
    {
        ssz_union_t un;
        un.selector = 0;
        un.data = (void*)"Hello";
        un.serialize_fn = dummy_subserialize;
        out_size = sizeof(buffer);
        err = ssz_serialize_union(&un, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected union with selector=0 but non-null data.\n");
        }
        else
        {
            printf("  FAIL: Did not reject union with invalid combination.\n");
        }
    }

    printf("Testing union with non-zero selector and no sub-data...\n");
    {
        ssz_union_t un;
        un.selector = 5;
        un.data = NULL;
        un.serialize_fn = NULL;
        out_size = sizeof(buffer);
        err = ssz_serialize_union(&un, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0x05)
        {
            printf("  OK: Union with non-zero selector but no sub-data is correct.\n");
        }
        else
        {
            printf("  FAIL: Union with non-zero selector had unexpected serialization.\n");
        }
    }

    printf("Testing union with sub-data (non-zero selector) + valid sub-serialization...\n");
    {
        ssz_union_t un;
        un.selector = 10;
        un.data = (void*)"Subdata";
        un.serialize_fn = dummy_subserialize;
        memset(buffer, 0xAA, sizeof(buffer));
        out_size = sizeof(buffer);
        err = ssz_serialize_union(&un, buffer, &out_size);
        size_t expected_len = 1 + strlen("Subdata");
        if (err == SSZ_SUCCESS && out_size == expected_len &&
            buffer[0] == 0x0A && memcmp(&buffer[1], "Subdata", strlen("Subdata")) == 0)
        {
            printf("  OK: Union with sub-data and selector=10 is correct.\n");
        }
        else
        {
            printf("  FAIL: Union with sub-data did not serialize as expected.\n");
        }
    }

    printf("Testing union with invalid selector > 127...\n");
    {
        ssz_union_t un;
        un.selector = 200; 
        un.data = NULL;
        un.serialize_fn = NULL;
        out_size = sizeof(buffer);
        err = ssz_serialize_union(&un, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected union with out-of-range selector.\n");
        }
        else
        {
            printf("  FAIL: Did not reject union with out-of-range selector.\n");
        }
    }
}

static void test_serialize_vector(void)
{
    printf("\n--- Testing ssz_serialize_vector ---\n");

    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element vector => should fail...\n");
    {
        uint8_t dummy_data[1];
        size_t dummy_element_sizes[1];
        out_size = sizeof(buffer);
        err = ssz_serialize_vector(dummy_data, 0, dummy_element_sizes, false, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected zero-element vector.\n");
        }
        else
        {
            printf("  FAIL: Did not reject zero-element vector.\n");
        }
    }

    printf("Testing fixed-size vector with 3 elements each 4 bytes...\n");
    {
        uint8_t elements[12] = {
            0xDD, 0xCC, 0xBB, 0xAA, 
            0x44, 0x33, 0x22, 0x11, 
            0xEE, 0xCD, 0xAB, 0x99
        };
        size_t element_sizes[3] = {4, 4, 4};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector(elements, 3, element_sizes, false, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 12 && memcmp(buffer, elements, 12) == 0)
        {
            printf("  OK: Fixed-size vector of 3 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Fixed-size vector serialization failed.\n");
        }
    }

    printf("Testing variable-size vector with 3 elements of sizes 2,4,3...\n");
    {
        uint8_t elements[9] = {
            0xAA, 0xBB,
            0x01, 0x02, 0x03, 0x04,
            0x99, 0x88, 0x77
        };
        size_t element_sizes[3] = {2, 4, 3};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector(elements, 3, element_sizes, true, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 21)
        {
            if (buffer[0] == 0x0C && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x00 &&
                buffer[4] == 0x0E && buffer[5] == 0x00 && buffer[6] == 0x00 && buffer[7] == 0x00 &&
                buffer[8] == 0x12 && buffer[9] == 0x00 && buffer[10] == 0x00 && buffer[11] == 0x00 &&
                buffer[12] == 0xAA && buffer[13] == 0xBB &&
                buffer[14] == 0x01 && buffer[15] == 0x02 && buffer[16] == 0x03 && buffer[17] == 0x04 &&
                buffer[18] == 0x99 && buffer[19] == 0x88 && buffer[20] == 0x77)
            {
                printf("  OK: Variable-size vector of 3 elements serialized correctly.\n");
            }
            else
            {
                printf("  FAIL: Variable-size vector serialized data mismatch.\n");
            }
        }
        else
        {
            printf("  FAIL: Variable-size vector call did not succeed as expected.\n");
        }
    }
}

static void test_serialize_list(void)
{
    printf("\n--- Testing ssz_serialize_list ---\n");

    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list => valid, should produce 0 bytes...\n");
    {
        uint8_t dummy_data[1];
        size_t dummy_element_sizes[1];
        out_size = sizeof(buffer);
        err = ssz_serialize_list(dummy_data, 0, dummy_element_sizes, false, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list accepted, out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list did not behave as expected.\n");
        }
    }

    printf("Testing fixed-size list of 3 elements each 4 bytes...\n");
    {
        uint8_t elements[12] = {
            0xDD, 0xCC, 0xBB, 0xAA, 
            0x44, 0x33, 0x22, 0x11, 
            0xEE, 0xCD, 0xAB, 0x99
        };
        size_t element_sizes[3] = {4, 4, 4};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_list(elements, 3, element_sizes, false, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 12 && memcmp(buffer, elements, 12) == 0)
        {
            printf("  OK: Fixed-size list with 3 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Fixed-size list serialization mismatch.\n");
        }
    }

    printf("Testing variable-size list of 3 elements with sizes 2,4,3...\n");
    {
        uint8_t elements[9] = {
            0xAA, 0xBB,
            0x01, 0x02, 0x03, 0x04,
            0x99, 0x88, 0x77
        };
        size_t element_sizes[3] = {2, 4, 3};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_list(elements, 3, element_sizes, true, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 21)
        {
            if (buffer[0] == 0x0C && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x00 &&
                buffer[4] == 0x0E && buffer[5] == 0x00 && buffer[6] == 0x00 && buffer[7] == 0x00 &&
                buffer[8] == 0x12 && buffer[9] == 0x00 && buffer[10] == 0x00 && buffer[11] == 0x00 &&
                buffer[12] == 0xAA && buffer[13] == 0xBB &&
                buffer[14] == 0x01 && buffer[15] == 0x02 && buffer[16] == 0x03 && buffer[17] == 0x04 &&
                buffer[18] == 0x99 && buffer[19] == 0x88 && buffer[20] == 0x77)
            {
                printf("  OK: Variable-size list of 3 elements serialized correctly.\n");
            }
            else
            {
                printf("  FAIL: Variable-size list data mismatch.\n");
            }
        }
        else
        {
            printf("  FAIL: Variable-size list call did not succeed as expected.\n");
        }
    }
}

int main(void)
{
    test_serialize_uintN();
    test_serialize_boolean();
    test_serialize_bitvector();
    test_serialize_bitlist();
    test_serialize_union();
    test_serialize_vector();
    test_serialize_list();
    return 0;
}
