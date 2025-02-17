#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "snappy_decode.h"
#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "ssz_merkle.h"
#include "yaml_parser.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/uints"
#endif

#define MAX_FAILURES 1024

typedef struct {
    char folder_name[256];
    char folder_path[1024];
    char message[1024];
} FailureDetail;

FailureDetail failures[MAX_FAILURES];
int failure_count = 0;
int total_valid_tests = 0;
int total_invalid_tests = 0;
int valid_passed = 0;
int valid_failed = 0;
int invalid_passed = 0;
int invalid_failed = 0;

void record_failure(const char *folder_name, const char *folder_path, const char *message)
{
    if (failure_count < MAX_FAILURES)
    {
        snprintf(failures[failure_count].folder_name, sizeof(failures[failure_count].folder_name), "%s", folder_name);
        snprintf(failures[failure_count].folder_path, sizeof(failures[failure_count].folder_path), "%s", folder_path);
        snprintf(failures[failure_count].message, sizeof(failures[failure_count].message), "%s", message);
        failure_count++;
    }
}

unsigned char *snappy_decode(const unsigned char *compressed_data, size_t compressed_size, size_t *decoded_size)
{
    size_t uncompressed_length;
    snappy_status status = snappy_uncompressed_length((const char *)compressed_data, compressed_size, &uncompressed_length);
    if (status != SNAPPY_OK)
    {
        fprintf(stderr, "Error: snappy_uncompressed_length failed with status %d\n", status);
        return NULL;
    }
    unsigned char *decoded = malloc(uncompressed_length);
    if (!decoded)
    {
        perror("malloc");
        return NULL;
    }
    status = snappy_uncompress((const char *)compressed_data, compressed_size, (char *)decoded, &uncompressed_length);
    if (status != SNAPPY_OK)
    {
        fprintf(stderr, "Error: snappy_uncompress failed with status %d\n", status);
        free(decoded);
        return NULL;
    }
    *decoded_size = uncompressed_length;
    return decoded;
}

unsigned char *read_file(const char *filepath, size_t *size_out)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return NULL;
    }
    long filesize = ftell(fp);
    if (filesize < 0)
    {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    unsigned char *buffer = malloc(filesize);
    if (!buffer)
    {
        fclose(fp);
        return NULL;
    }
    size_t read_bytes = fread(buffer, 1, filesize, fp);
    if (read_bytes != (size_t)filesize)
    {
        free(buffer);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *size_out = (size_t)filesize;
    return buffer;
}

void print_hex(const unsigned char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        printf("%02x", data[i]);
    printf("\n");
}

void process_serialized_file(const char *folder_name, const char *folder_path, const char *serialized_file_path)
{
    total_valid_tests++;
    size_t comp_size = 0;
    unsigned char *comp_data = read_file(serialized_file_path, &comp_size);
    if (!comp_data)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Failed to read serialized file");
        return;
    }
    size_t dec_size = 0;
    unsigned char *expected_data = snappy_decode(comp_data, comp_size, &dec_size);
    free(comp_data);
    if (!expected_data)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Failed to decode Snappy data");
        return;
    }
    unsigned int bit_size;
    char variant[64];
    if (sscanf(folder_name, "uint_%u_%63s", &bit_size, variant) != 2)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Folder name does not match expected pattern");
        free(expected_data);
        return;
    }
    void *in_mem = NULL;
    ssz_error_t des_err;
    char err_msg[256];
    if (bit_size == 8)
    {
        in_mem = malloc(sizeof(uint8_t));
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint8");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint8(expected_data, dec_size, (uint8_t *)in_mem);
    }
    else if (bit_size == 16)
    {
        in_mem = malloc(sizeof(uint16_t));
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint16");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint16(expected_data, dec_size, (uint16_t *)in_mem);
    }
    else if (bit_size == 32)
    {
        in_mem = malloc(sizeof(uint32_t));
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint32");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint32(expected_data, dec_size, (uint32_t *)in_mem);
    }
    else if (bit_size == 64)
    {
        in_mem = malloc(sizeof(uint64_t));
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint64");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_vector_uint64(expected_data, dec_size, 1, (uint64_t *)in_mem);
    }
    else if (bit_size == 128)
    {
        in_mem = malloc(16);
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint128");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint128(expected_data, dec_size, in_mem);
    }
    else if (bit_size == 256)
    {
        in_mem = malloc(32);
        if (!in_mem)
        {
            valid_failed++;
            record_failure(folder_name, folder_path, "Memory allocation failed for uint256");
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint256(expected_data, dec_size, in_mem);
    }
    else
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Unsupported bit size");
        free(expected_data);
        return;
    }
    if (des_err != SSZ_SUCCESS)
    {
        snprintf(err_msg, sizeof(err_msg), "Deserialization error: %d", des_err);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
        free(in_mem);
        free(expected_data);
        return;
    }
    size_t expected_out_size = 0;
    if (bit_size == 8)
        expected_out_size = sizeof(uint8_t);
    else if (bit_size == 16)
        expected_out_size = sizeof(uint16_t);
    else if (bit_size == 32)
        expected_out_size = sizeof(uint32_t);
    else if (bit_size == 64)
        expected_out_size = sizeof(uint64_t);
    else if (bit_size == 128)
        expected_out_size = 16;
    else if (bit_size == 256)
        expected_out_size = 32;
    uint8_t *out_buf = malloc(expected_out_size);
    if (!out_buf)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Memory allocation failed for output buffer");
        free(in_mem);
        free(expected_data);
        return;
    }
    size_t out_size = expected_out_size;
    ssz_error_t ser_err;
    if (bit_size == 8)
        ser_err = ssz_serialize_uint8(in_mem, out_buf, &out_size);
    else if (bit_size == 16)
        ser_err = ssz_serialize_uint16(in_mem, out_buf, &out_size);
    else if (bit_size == 32)
        ser_err = ssz_serialize_uint32(in_mem, out_buf, &out_size);
    else if (bit_size == 64)
        ser_err = ssz_serialize_vector_uint64((uint64_t *)in_mem, 1, out_buf, &out_size);
    else if (bit_size == 128)
        ser_err = ssz_serialize_uint128(in_mem, out_buf, &out_size);
    else if (bit_size == 256)
        ser_err = ssz_serialize_uint256(in_mem, out_buf, &out_size);
    else
    {
        free(in_mem);
        free(expected_data);
        free(out_buf);
        valid_failed++;
        record_failure(folder_name, folder_path, "Unsupported bit size in serialization");
        return;
    }
    free(in_mem);
    if (ser_err != SSZ_SUCCESS)
    {
        snprintf(err_msg, sizeof(err_msg), "Serialization error: %d", ser_err);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
        free(out_buf);
        free(expected_data);
        return;
    }
    if (out_size != dec_size)
    {
        snprintf(err_msg, sizeof(err_msg), "Size mismatch: expected %zu, got %zu", dec_size, out_size);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
        free(out_buf);
        free(expected_data);
        return;
    }
    if (memcmp(out_buf, expected_data, out_size) != 0)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Content mismatch");
        free(out_buf);
        free(expected_data);
        return;
    }
    valid_passed++;
    size_t chunk_count = (out_size + SSZ_BYTES_PER_CHUNK - 1) / SSZ_BYTES_PER_CHUNK;
    uint8_t *packed_chunks = malloc(chunk_count * SSZ_BYTES_PER_CHUNK);
    if (!packed_chunks)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Memory allocation failed for packed chunks");
        free(out_buf);
        free(expected_data);
        return;
    }
    size_t packed_chunk_count = 0;
    ssz_error_t pack_err = ssz_pack(out_buf, 1, out_size, packed_chunks, &packed_chunk_count);
    if (pack_err != SSZ_SUCCESS)
    {
        snprintf(err_msg, sizeof(err_msg), "Packing error: %d", pack_err);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
        free(packed_chunks);
        free(out_buf);
        free(expected_data);
        return;
    }
    uint8_t merkle_root[SSZ_BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed_chunks, packed_chunk_count, chunk_count, merkle_root);
    if (merkle_err != SSZ_SUCCESS)
    {
        snprintf(err_msg, sizeof(err_msg), "Merkleization error: %d", merkle_err);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
        free(packed_chunks);
        free(out_buf);
        free(expected_data);
        return;
    }
    free(packed_chunks);
    char meta_yaml_path[1024];
    strncpy(meta_yaml_path, serialized_file_path, sizeof(meta_yaml_path));
    meta_yaml_path[sizeof(meta_yaml_path) - 1] = '\0';
    char *last_slash = strrchr(meta_yaml_path, '/');
    if (last_slash != NULL)
        strcpy(last_slash + 1, "meta.yaml");
    else
        snprintf(meta_yaml_path, sizeof(meta_yaml_path), "meta.yaml");
    size_t yaml_size = 0;
    uint8_t *yaml_data = read_yaml_field(meta_yaml_path, "root", &yaml_size);
    if (!yaml_data)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Failed to read 'root' field from meta.yaml");
        free(out_buf);
        free(expected_data);
        return;
    }
    if (yaml_size != SSZ_BYTES_PER_CHUNK)
    {
        snprintf(err_msg, sizeof(err_msg), "Meta.yaml 'root' field size mismatch: expected %d, got %zu", SSZ_BYTES_PER_CHUNK, yaml_size);
        valid_failed++;
        record_failure(folder_name, folder_path, err_msg);
    }
    else if (memcmp(yaml_data, merkle_root, SSZ_BYTES_PER_CHUNK) != 0)
    {
        valid_failed++;
        record_failure(folder_name, folder_path, "Meta.yaml 'root' field does not match Merkle root");
    }
    free(yaml_data);
    free(out_buf);
    free(expected_data);
}

void process_invalid_serialized_file(const char *folder_name, const char *folder_path, const char *serialized_file_path)
{
    total_invalid_tests++;
    size_t comp_size = 0;
    unsigned char *comp_data = read_file(serialized_file_path, &comp_size);
    if (!comp_data)
    {
        invalid_failed++;
        record_failure(folder_name, folder_path, "Failed to read serialized file");
        return;
    }
    size_t dec_size = 0;
    unsigned char *decoded_data = snappy_decode(comp_data, comp_size, &dec_size);
    free(comp_data);
    if (!decoded_data)
    {
        invalid_failed++;
        record_failure(folder_name, folder_path, "Failed to decode Snappy data");
        return;
    }
    unsigned int bit_size;
    char variant[64];
    if (sscanf(folder_name, "uint_%u_%63s", &bit_size, variant) != 2)
    {
        invalid_failed++;
        record_failure(folder_name, folder_path, "Folder name does not match expected pattern");
        free(decoded_data);
        return;
    }
    ssz_error_t des_err;
    if (bit_size == 8)
    {
        uint8_t dummy;
        des_err = ssz_deserialize_uint8(decoded_data, dec_size, &dummy);
    }
    else if (bit_size == 16)
    {
        uint16_t dummy;
        des_err = ssz_deserialize_uint16(decoded_data, dec_size, &dummy);
    }
    else if (bit_size == 32)
    {
        uint32_t dummy;
        des_err = ssz_deserialize_uint32(decoded_data, dec_size, &dummy);
    }
    else if (bit_size == 64)
    {
        uint64_t dummy;
        des_err = ssz_deserialize_vector_uint64(decoded_data, dec_size, 1, &dummy);
    }
    else if (bit_size == 128)
    {
        uint8_t dummy[16];
        des_err = ssz_deserialize_uint128(decoded_data, dec_size, dummy);
    }
    else if (bit_size == 256)
    {
        uint8_t dummy[32];
        des_err = ssz_deserialize_uint256(decoded_data, dec_size, dummy);
    }
    else
    {
        invalid_failed++;
        record_failure(folder_name, folder_path, "Unsupported bit size");
        free(decoded_data);
        return;
    }
    if (des_err == SSZ_SUCCESS)
    {
        invalid_failed++;
        record_failure(folder_name, folder_path, "Deserialization unexpectedly succeeded");
    }
    else
    {
        invalid_passed++;
    }
    free(decoded_data);
}

int main(void)
{
    char valid_dir_path[1024];
    snprintf(valid_dir_path, sizeof(valid_dir_path), "%s/valid", TESTS_DIR);
    DIR *dir = opendir(valid_dir_path);
    if (!dir)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char folder_path[1024];
        snprintf(folder_path, sizeof(folder_path), "%s/%s", valid_dir_path, entry->d_name);
        struct stat statbuf;
        if (stat(folder_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode))
            continue;
        char serialized_file_path[1024];
        snprintf(serialized_file_path, sizeof(serialized_file_path), "%s/serialized.ssz_snappy", folder_path);
        process_serialized_file(entry->d_name, folder_path, serialized_file_path);
    }
    closedir(dir);
    char invalid_dir_path[1024];
    snprintf(invalid_dir_path, sizeof(invalid_dir_path), "%s/invalid", TESTS_DIR);
    DIR *dir_invalid = opendir(invalid_dir_path);
    if (!dir_invalid)
    {
        perror("opendir");
        return EXIT_FAILURE;
    }
    while ((entry = readdir(dir_invalid)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char folder_path[1024];
        snprintf(folder_path, sizeof(folder_path), "%s/%s", invalid_dir_path, entry->d_name);
        struct stat statbuf;
        if (stat(folder_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode))
            continue;
        char serialized_file_path[1024];
        snprintf(serialized_file_path, sizeof(serialized_file_path), "%s/serialized.ssz_snappy", folder_path);
        process_invalid_serialized_file(entry->d_name, folder_path, serialized_file_path);
    }
    closedir(dir_invalid);
    printf("\nValid tests: %d passed, %d failed, out of %d\n", valid_passed, valid_failed, total_valid_tests);
    printf("Invalid tests: %d passed, %d failed, out of %d\n", invalid_passed, invalid_failed, total_invalid_tests);
    if (failure_count > 0)
    {
        for (int i = 0; i < failure_count; i++)
        {
            printf("Folder %s: FAILED - %s (Path: %s)\n", failures[i].folder_name, failures[i].message, failures[i].folder_path);
        }
    }
    return EXIT_SUCCESS;
}