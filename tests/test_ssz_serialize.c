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

    printf("Testing valid 8-bit serialization...\n");
    {
        uint8_t val8 = 0xAB;
        out_size = sizeof(buffer);
        err = ssz_serialize_uint8(&val8, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 1 && buffer[0] == 0xAB)
        {
            printf("  OK: 8-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 8-bit value serialization failed.\n");
        }
    }

    printf("Testing 8-bit with null pointer...\n");
    {
        out_size = 1;
        err = ssz_serialize_uint8(NULL, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 8-bit null pointer rejected.\n");
        }
        else
        {
            printf("  FAIL: 8-bit null pointer was not rejected.\n");
        }
    }

    printf("Testing 8-bit with out_size=0...\n");
    {
        uint8_t val8 = 0xFF;
        out_size = 0;
        err = ssz_serialize_uint8(&val8, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 8-bit zero out_size rejected.\n");
        }
        else
        {
            printf("  FAIL: 8-bit zero out_size was not rejected.\n");
        }
    }

    printf("Testing valid 16-bit serialization...\n");
    {
        uint16_t val16 = 0xCCDD;
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint16(&val16, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 2 && buffer[0] == 0xDD && buffer[1] == 0xCC)
        {
            printf("  OK: 16-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 16-bit value serialization failed.\n");
        }
    }

    printf("Testing 16-bit with insufficient buffer...\n");
    {
        uint16_t val16 = 0x1234;
        out_size = 1;
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint16(&val16, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 16-bit insufficient buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 16-bit insufficient buffer was not rejected.\n");
        }
    }

    printf("Testing valid 32-bit serialization...\n");
    {
        uint64_t val32 = 0xAABBCCDDULL;
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint32(&val32, buffer, &out_size);
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

    printf("Testing 32-bit with out_buf=NULL...\n");
    {
        uint32_t val32 = 0xDEADBEEF;
        out_size = 4;
        err = ssz_serialize_uint32(&val32, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 32-bit null buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 32-bit null buffer was not rejected.\n");
        }
    }

    printf("Testing valid 64-bit serialization...\n");
    {
        uint64_t val64 = 0x1122334455667788ULL;
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint64(&val64, buffer, &out_size);
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

    printf("Testing 64-bit with out_size=7...\n");
    {
        uint64_t val64 = 0xFFFFFFFFFFFFFFFFULL;
        out_size = 7;
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint64(&val64, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 64-bit insufficient out_size rejected.\n");
        }
        else
        {
            printf("  FAIL: 64-bit out_size=7 was not rejected.\n");
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
        err = ssz_serialize_uint128(val128, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 16 && memcmp(buffer, val128, 16) == 0)
        {
            printf("  OK: 128-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 128-bit value serialization failed.\n");
        }
    }

    printf("Testing 128-bit with out_size=8...\n");
    {
        uint8_t val128[16] = {
            0x00, 0x01, 0x02, 0x03,
            0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F
        };
        out_size = 8;
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_uint128(val128, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 128-bit out_size=8 rejected.\n");
        }
        else
        {
            printf("  FAIL: 128-bit out_size=8 was not rejected.\n");
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
        err = ssz_serialize_uint256(val256, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 32 && memcmp(buffer, val256, 32) == 0)
        {
            printf("  OK: 256-bit value serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: 256-bit value serialization failed.\n");
        }
    }

    printf("Testing 256-bit with out_buf=NULL...\n");
    {
        uint8_t val256[32] = {
            0x00, 0x01, 0x02, 0x03,
            0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F,
            0x10, 0x11, 0x12, 0x13,
            0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B,
            0x1C, 0x1D, 0x1E, 0x1F
        };
        out_size = 32;
        err = ssz_serialize_uint256(val256, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: 256-bit null buffer rejected.\n");
        }
        else
        {
            printf("  FAIL: 256-bit null buffer was not rejected.\n");
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
        memset(bits, true, sizeof(bits));
        uint8_t small_buf[1];
        size_t small_sz = sizeof(small_buf);
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
        bool empty_bits[1];
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

static void test_serialize_vector_uint8(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint8 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint8 => should fail...\n");
    {
        uint8_t empty[1] = {0xAA};
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint8(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint8 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint8 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint8 => should pass...\n");
    {
        uint8_t data[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint8(data, 5, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 5 && memcmp(buffer, data, 5) == 0)
        {
            printf("  OK: Vector of 5 uint8 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 5 uint8 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_uint16(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint16 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint16 => should fail...\n");
    {
        uint16_t empty[1] = {0xABCD};
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint16(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint16 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint16 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint16 => should pass...\n");
    {
        uint16_t data[2] = {0x1234, 0xABCD};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint16(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 4 &&
            buffer[0] == 0x34 && buffer[1] == 0x12 &&
            buffer[2] == 0xCD && buffer[3] == 0xAB)
        {
            printf("  OK: Vector of 2 uint16 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 2 uint16 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_uint32(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint32 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint32 => should fail...\n");
    {
        uint32_t empty[1] = {0xABCD1234};
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint32(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint32 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint32 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint32 => should pass...\n");
    {
        uint32_t data[2] = {0x11223344, 0xAABBCCDD};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint32(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 8 &&
            buffer[0] == 0x44 && buffer[1] == 0x33 &&
            buffer[2] == 0x22 && buffer[3] == 0x11 &&
            buffer[4] == 0xDD && buffer[5] == 0xCC &&
            buffer[6] == 0xBB && buffer[7] == 0xAA)
        {
            printf("  OK: Vector of 2 uint32 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 2 uint32 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_uint64(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint64 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint64 => should fail...\n");
    {
        uint64_t empty[1] = {0xDEADDEADDEADDEADULL};
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint64(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint64 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint64 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint64 => should pass...\n");
    {
        uint64_t data[2] = {0x1122334455667788ULL, 0xAABBCCDDEEFF0011ULL};
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint64(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 16 &&
            buffer[0] == 0x88 && buffer[1] == 0x77 && buffer[2] == 0x66 &&
            buffer[3] == 0x55 && buffer[4] == 0x44 && buffer[5] == 0x33 &&
            buffer[6] == 0x22 && buffer[7] == 0x11 &&
            buffer[8] == 0x11 && buffer[9] == 0x00 &&
            buffer[10] == 0xFF && buffer[11] == 0xEE &&
            buffer[12] == 0xDD && buffer[13] == 0xCC &&
            buffer[14] == 0xBB && buffer[15] == 0xAA)
        {
            printf("  OK: Vector of 2 uint64 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 2 uint64 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_uint128(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint128 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint128 => should fail...\n");
    {
        uint8_t empty[16];
        memset(empty, 0x12, sizeof(empty));
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint128(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint128 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint128 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint128 => should pass...\n");
    {
        uint8_t data[32] = {
            0x01, 0x02, 0x03, 0x04,
            0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x0B, 0x0C,
            0x0D, 0x0E, 0x0F, 0x10,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x11, 0x22,
            0x33, 0x44, 0x55, 0x66,
            0x77, 0x88, 0x99, 0x00
        };
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint128(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 32 && memcmp(buffer, data, 32) == 0)
        {
            printf("  OK: Vector of 2 uint128 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 2 uint128 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_uint256(void)
{
    printf("\n--- Testing ssz_serialize_vector_uint256 ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_uint256 => should fail...\n");
    {
        uint8_t empty[32];
        memset(empty, 0x55, sizeof(empty));
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_uint256(empty, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of uint256 was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of uint256 was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_uint256 => should pass...\n");
    {
        uint8_t data[64] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x99, 0xAA, 0xBB, 0xCC,
            0xDD, 0xEE, 0xFF, 0x00,
            0x12, 0x13, 0x14, 0x15,
            0x16, 0x17, 0x18, 0x19,
            0x1A, 0x1B, 0x1C, 0x1D,
            0x1E, 0x1F, 0x20, 0x21,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x00, 0x11,
            0x22, 0x33, 0x44, 0x55,
            0x66, 0x77, 0x88, 0x99,
            0x9A, 0x9B, 0x9C, 0x9D,
            0x9E, 0x9F, 0xA0, 0xA1,
            0xA2, 0xA3, 0xA4, 0xA5,
            0xA6, 0xA7, 0xA8, 0xA9
        };
        out_size = sizeof(buffer);
        memset(buffer, 0, sizeof(buffer));
        err = ssz_serialize_vector_uint256(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 64 && memcmp(buffer, data, 64) == 0)
        {
            printf("  OK: Vector of 2 uint256 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 2 uint256 elements serialization failed.\n");
        }
    }
}

static void test_serialize_vector_bool(void)
{
    printf("\n--- Testing ssz_serialize_vector_bool ---\n");
    uint8_t buffer[64];
    memset(buffer, 0, sizeof(buffer));
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element with ssz_serialize_vector_bool => should fail...\n");
    {
        bool empty_bool[1] = {true};
        out_size = sizeof(buffer);
        err = ssz_serialize_vector_bool(empty_bool, 0, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Zero-element vector of bool was rejected.\n");
        }
        else
        {
            printf("  FAIL: Zero-element vector of bool was not rejected.\n");
        }
    }

    printf("Testing a small ssz_serialize_vector_bool => should pass...\n");
    {
        bool data[5] = {false, true, false, true, true};
        out_size = sizeof(buffer);
        memset(buffer, 0x99, sizeof(buffer));
        err = ssz_serialize_vector_bool(data, 5, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 5 &&
            buffer[0] == 0x00 && buffer[1] == 0x01 &&
            buffer[2] == 0x00 && buffer[3] == 0x01 &&
            buffer[4] == 0x01)
        {
            printf("  OK: Vector of 5 bool elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: Vector of 5 bool elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_uint8(void)
{
    printf("\n--- Testing ssz_serialize_list_uint8 ---\n");
    uint8_t buffer[64];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint8 => should produce out_size=0...\n");
    {
        uint8_t elements[1] = {0x12};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint8(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint8 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint8 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 3 elements => should pass...\n");
    {
        uint8_t data[3] = {0x01, 0x02, 0x03};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint8(data, 3, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 3 &&
            buffer[0] == 0x01 && buffer[1] == 0x02 && buffer[2] == 0x03)
        {
            printf("  OK: List of 3 uint8 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 3 uint8 elements serialization failed.\n");
        }
    }

    printf("Testing insufficient out_buf size => should fail...\n");
    {
        uint8_t data[2] = {0x55, 0x66};
        out_size = 1;
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint8(data, 2, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected insufficient buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject insufficient buffer.\n");
        }
    }

    printf("Testing null out_buf => should fail...\n");
    {
        uint8_t data[2] = {0x55, 0x66};
        out_size = 2;
        err = ssz_serialize_list_uint8(data, 2, NULL, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null output buffer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null output buffer.\n");
        }
    }

    printf("Testing null elements pointer => should fail...\n");
    {
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint8(NULL, 2, buffer, &out_size);
        if (err == SSZ_ERROR_SERIALIZATION)
        {
            printf("  OK: Rejected null elements pointer.\n");
        }
        else
        {
            printf("  FAIL: Did not reject null elements pointer.\n");
        }
    }
}

static void test_serialize_list_uint16(void)
{
    printf("\n--- Testing ssz_serialize_list_uint16 ---\n");
    uint8_t buffer[64];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint16 => should produce out_size=0...\n");
    {
        uint16_t elements[1] = {0xABCD};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint16(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint16 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint16 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 2 elements => should pass...\n");
    {
        uint16_t data[2] = {0x1234, 0xABCD};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint16(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 4 &&
            buffer[0] == 0x34 && buffer[1] == 0x12 &&
            buffer[2] == 0xCD && buffer[3] == 0xAB)
        {
            printf("  OK: List of 2 uint16 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 2 uint16 elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_uint32(void)
{
    printf("\n--- Testing ssz_serialize_list_uint32 ---\n");
    uint8_t buffer[64];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint32 => should produce out_size=0...\n");
    {
        uint32_t elements[1] = {0xDEADBEEF};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint32(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint32 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint32 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 2 elements => should pass...\n");
    {
        uint32_t data[2] = {0x11223344, 0xAABBCCDD};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint32(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 8 &&
            buffer[0] == 0x44 && buffer[1] == 0x33 &&
            buffer[2] == 0x22 && buffer[3] == 0x11 &&
            buffer[4] == 0xDD && buffer[5] == 0xCC &&
            buffer[6] == 0xBB && buffer[7] == 0xAA)
        {
            printf("  OK: List of 2 uint32 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 2 uint32 elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_uint64(void)
{
    printf("\n--- Testing ssz_serialize_list_uint64 ---\n");
    uint8_t buffer[64];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint64 => should produce out_size=0...\n");
    {
        uint64_t elements[1] = {0x0011223344556677ULL};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint64(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint64 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint64 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 2 elements => should pass...\n");
    {
        uint64_t data[2] = {0x1122334455667788ULL, 0xAABBCCDDEEFF0011ULL};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint64(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 16 &&
            buffer[0] == 0x88 && buffer[1] == 0x77 && buffer[2] == 0x66 &&
            buffer[3] == 0x55 && buffer[4] == 0x44 && buffer[5] == 0x33 &&
            buffer[6] == 0x22 && buffer[7] == 0x11 &&
            buffer[8] == 0x11 && buffer[9] == 0x00 &&
            buffer[10] == 0xFF && buffer[11] == 0xEE &&
            buffer[12] == 0xDD && buffer[13] == 0xCC &&
            buffer[14] == 0xBB && buffer[15] == 0xAA)
        {
            printf("  OK: List of 2 uint64 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 2 uint64 elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_uint128(void)
{
    printf("\n--- Testing ssz_serialize_list_uint128 ---\n");
    uint8_t buffer[128];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint128 => should produce out_size=0...\n");
    {
        uint8_t elements[16];
        memset(elements, 0x12, sizeof(elements));
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint128(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint128 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint128 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 2 elements => should pass...\n");
    {
        uint8_t data[32] = {
            0x01, 0x02, 0x03, 0x04,
            0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x0B, 0x0C,
            0x0D, 0x0E, 0x0F, 0x10,
            0xFF, 0xEE, 0xDD, 0xCC,
            0xBB, 0xAA, 0x99, 0x88,
            0x77, 0x66, 0x55, 0x44,
            0x33, 0x22, 0x11, 0x00
        };
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint128(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 32 && memcmp(buffer, data, 32) == 0)
        {
            printf("  OK: List of 2 uint128 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 2 uint128 elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_uint256(void)
{
    printf("\n--- Testing ssz_serialize_list_uint256 ---\n");
    uint8_t buffer[128];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of uint256 => should produce out_size=0...\n");
    {
        uint8_t elements[32];
        memset(elements, 0x55, sizeof(elements));
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint256(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of uint256 produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of uint256 did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 2 elements => should pass...\n");
    {
        uint8_t data[64] = {
            0x11, 0x22, 0x33, 0x44,
            0x55, 0x66, 0x77, 0x88,
            0x11, 0x11, 0x11, 0x11,
            0x22, 0x22, 0x22, 0x22,
            0xAA, 0xBB, 0xCC, 0xDD,
            0xEE, 0xFF, 0x11, 0x22,
            0x33, 0x44, 0x55, 0x66,
            0x77, 0x88, 0x99, 0x00,
            0x10, 0x10, 0x20, 0x20,
            0x30, 0x30, 0x40, 0x40,
            0x50, 0x50, 0x60, 0x60,
            0x70, 0x70, 0x80, 0x80,
            0x90, 0x90, 0xA0, 0xA0,
            0xB0, 0xB0, 0xC0, 0xC0,
            0xD0, 0xD0, 0xE0, 0xE0,
            0xF0, 0xF0, 0x99, 0x99
        };
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_uint256(data, 2, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 64 && memcmp(buffer, data, 64) == 0)
        {
            printf("  OK: List of 2 uint256 elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 2 uint256 elements serialization failed.\n");
        }
    }
}

static void test_serialize_list_bool(void)
{
    printf("\n--- Testing ssz_serialize_list_bool ---\n");
    uint8_t buffer[32];
    ssz_error_t err;
    size_t out_size;

    printf("Testing zero-element list of bool => should produce out_size=0...\n");
    {
        bool elements[1] = {true};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_bool(elements, 0, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 0)
        {
            printf("  OK: Zero-element list of bool produced out_size=0.\n");
        }
        else
        {
            printf("  FAIL: Zero-element list of bool did not behave as expected.\n");
        }
    }

    printf("Testing a small list of 4 booleans => should pass...\n");
    {
        bool data[4] = {true, false, true, true};
        out_size = sizeof(buffer);
        memset(buffer, 0xAA, sizeof(buffer));
        err = ssz_serialize_list_bool(data, 4, buffer, &out_size);
        if (err == SSZ_SUCCESS && out_size == 4 &&
            buffer[0] == 0x01 && buffer[1] == 0x00 &&
            buffer[2] == 0x01 && buffer[3] == 0x01)
        {
            printf("  OK: List of 4 bool elements serialized correctly.\n");
        }
        else
        {
            printf("  FAIL: List of 4 bool elements serialization failed.\n");
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
    test_serialize_vector_uint8();
    test_serialize_vector_uint16();
    test_serialize_vector_uint32();
    test_serialize_vector_uint64();
    test_serialize_vector_uint128();
    test_serialize_vector_uint256();
    test_serialize_vector_bool();
    test_serialize_list_uint8();
    test_serialize_list_uint16();
    test_serialize_list_uint32();
    test_serialize_list_uint64();
    test_serialize_list_uint128();
    test_serialize_list_uint256();
    test_serialize_list_bool();

    return 0;
}