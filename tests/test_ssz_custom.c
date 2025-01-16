#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "ssz_serialization.h"

static bool parse_hex(const char *hex, uint8_t *out, size_t out_len) {
    size_t hex_len = strlen(hex);
    if (hex_len < (out_len * 2)) {
        return false;
    }
    for (size_t i = 0; i < out_len; i++) {
        char hi = hex[2*i];
        char lo = hex[2*i + 1];
        uint8_t val_hi = 0;
        uint8_t val_lo = 0;
        if (hi >= '0' && hi <= '9') {
            val_hi = hi - '0';
        } else if (hi >= 'a' && hi <= 'f') {
            val_hi = hi - 'a' + 10;
        } else if (hi >= 'A' && hi <= 'F') {
            val_hi = hi - 'A' + 10;
        } else {
            return false;
        }
        if (lo >= '0' && lo <= '9') {
            val_lo = lo - '0';
        } else if (lo >= 'a' && lo <= 'f') {
            val_lo = lo - 'a' + 10;
        } else if (lo >= 'A' && lo <= 'F') {
            val_lo = lo - 'A' + 10;
        } else {
            return false;
        }
        out[i] = (val_hi << 4) | val_lo;
    }
    return true;
}

void run_test_1()
{
    uint64_t some_uint64_value = 123456789ULL;
    uint8_t uint64_serialized[8];
    size_t out_size_uint64 = sizeof(uint64_serialized);
    ssz_error_t err = ssz_serialize_uintN(
        &some_uint64_value,
        64,
        uint64_serialized,
        &out_size_uint64
    );
    if (err != SSZ_SUCCESS) {
        printf("ssz_serialize_uintN error: %d\n", (int)err);
    }

    uint8_t raw_list_data[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
    size_t element_count = 5;
    size_t byte_sizes[5];
    for (size_t i = 0; i < 5; i++) {
        byte_sizes[i] = 1;
    }
    uint8_t list_serialized[128];
    size_t out_size_list = sizeof(list_serialized);
    err = ssz_serialize_list(
        raw_list_data,
        element_count,
        byte_sizes,     // we must not pass NULL here
        false,          // each element is not itself variable-sized, just one byte
        list_serialized,
        &out_size_list
    );
    if (err != SSZ_SUCCESS) {
        printf("ssz_serialize_list error: %d\n", (int)err);
    }

    bool some_bool_value = true;
    uint8_t bool_serialized[1];
    size_t out_size_bool = sizeof(bool_serialized);
    err = ssz_serialize_boolean(
        some_bool_value,
        bool_serialized,
        &out_size_bool
    );
    if (err != SSZ_SUCCESS) {
        printf("ssz_serialize_boolean error: %d\n", (int)err);
    }
}

int main(void)
{
    //run_test_1();
    return 0;
}