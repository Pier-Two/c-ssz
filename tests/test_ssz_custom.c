#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

// Include your SSZ serialization header:
#include "../include/ssz_serialization.h"

static bool parse_hex(const char *hex, uint8_t *out, size_t out_len) {
    // Very basic function that parses exactly 2* out_len hex chars into out buffer.
    // Returns false if there's any parsing error or if hex is too short.
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

void run_test_1(void)
{
    const char *pubkey_hex = "b41b57c27f30930062ae471b1e05cfc33750a8b61364e631cefcb81acbbfa8207d6711f2c727dd0f83dee166d307b929";
    const char *withdrawal_credentials_hex = "0100000000000000000000003575b29a214974db700a4c0ef7058d0972fa5a4f";
    uint64_t amount_val = 32000000000ULL;  // decimal
    const char *signature_hex = "8993f9535afba024613b99176098fc34b663d74dd951e8be96bcfa28d524a442d3a37964b926a75656a922976e4d0472143534bcffa9b2809b2f344ce5ac18794af1695b01c685ac8acb6904673647157d69a79864f2ddf863d2562430ff083e";

    // Buffers for each field. 
    uint8_t pubkey[48];
    uint8_t withdrawal_credentials[32];
    uint8_t signature[96];

    // Parse hex into raw bytes:
    if (!parse_hex(pubkey_hex, pubkey, sizeof(pubkey))) {
        printf("Failed to parse pubkey hex.\n");
    }
    if (!parse_hex(withdrawal_credentials_hex, withdrawal_credentials, sizeof(withdrawal_credentials))) {
        printf("Failed to parse withdrawal_credentials hex.\n");
    }
    if (!parse_hex(signature_hex, signature, sizeof(signature))) {
        printf("Failed to parse signature hex.\n");
    }

    // Convert amount to little-endian 8-byte SSZ form. 
    uint8_t amount_bytes[8];
    uint64_t amount_le = amount_val;  // If your system is big-endian, you should swap here.
    memcpy(amount_bytes, &amount_le, 8);

    // Now store everything in a single container_data buffer.
    // The container layout here is pubkey (48 bytes) + withdrawal_credentials (32 bytes)
    // + amount (8 bytes) + signature (96 bytes) = 48+32+8+96 = 184 total.
    uint8_t container_data[48 + 32 + 8 + 96];
    memcpy(container_data, pubkey, 48);
    memcpy(container_data + 48, withdrawal_credentials, 32);
    memcpy(container_data + 48 + 32, amount_bytes, 8);
    memcpy(container_data + 48 + 32 + 8, signature, 96);

    // For a container with 4 fields, all are fixed-size, so we say none are variable.
    bool field_is_variable_size[4] = {false, false, false, false};
    // And the sizes are exactly 48, 32, 8, and 96 in that order.
    size_t field_sizes[4] = {48, 32, 8, 96};

    // We allocate a buffer for the final serialized container. We'll guess 1024 is enough.
    uint8_t container_serialized[1024];
    size_t out_size = sizeof(container_serialized);

    // Call your container serialization function.
    ssz_error_t err = ssz_serialize_container(
        container_data,
        4,  // number of fields
        field_is_variable_size,
        field_sizes,
        container_serialized,
        &out_size
    );

    if (err != SSZ_SUCCESS) {
        printf("ssz_serialize_container failed with error code: %d\n", (int)err);
    }

    // If all fields are fixed-size, the result is basically the same bytes in order, 
    // since there's no variable offset region. We'll just print them in hex.
    printf("Serialized container (size=%zu): 0x", out_size);
    for (size_t i = 0; i < out_size; i++) {
        printf("%02x", container_serialized[i]);
    }
    printf("\n");
}

void run_test_2()
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
    // IMPORTANT: Provide a non-null element_sizes array. Each element is size 1 for a uint8.
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

    // Combine these three SSZ-encoded fields into one buffer. The order is:
    //   - uint64_serialized (8 bytes)
    //   - list_serialized (out_size_list bytes)
    //   - bool_serialized (1 byte)
    size_t container_data_len = out_size_uint64 + out_size_list + out_size_bool;
    uint8_t *container_data = malloc(container_data_len);
    if (!container_data) {
        printf("malloc failed\n");
    }
    size_t offset = 0;
    memcpy(container_data + offset, uint64_serialized, out_size_uint64);
    offset += out_size_uint64;
    memcpy(container_data + offset, list_serialized, out_size_list);
    offset += out_size_list;
    memcpy(container_data + offset, bool_serialized, out_size_bool);
    offset += out_size_bool;

    // Now tell ssz_serialize_container which fields are variable-size and how large they are.
    bool field_is_variable_size[3] = {false, true, false};
    size_t field_sizes[3] = {out_size_uint64, out_size_list, out_size_bool};

    uint8_t container_serialized[1024];
    size_t out_size_container = sizeof(container_serialized);
    err = ssz_serialize_container(
        container_data,
        3, // 3 fields
        field_is_variable_size,
        field_sizes,
        container_serialized,
        &out_size_container
    );
    free(container_data);

    if (err != SSZ_SUCCESS) {
        printf("ssz_serialize_container failed: %d\n", (int)err);
    }

    printf("Container SSZ encoding (size=%zu): 0x", out_size_container);
    for (size_t i = 0; i < out_size_container; i++) {
        printf("%02x", container_serialized[i]);
    }
    printf("\n");
}

int main(void)
{
    run_test_1();
    run_test_2();

    return 0;
}