#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <snappy-c.h>
#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "ssz_merkle.h"
#include "yaml_parser.h"

#ifndef TESTS_DIR
#define TESTS_DIR "tests/fixtures/general/phase0/ssz_generic/uints"
#endif

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

void process_serialized_file(const char *folder_name, const char *serialized_file_path)
{
    size_t comp_size = 0;
    unsigned char *comp_data = read_file(serialized_file_path, &comp_size);
    if (!comp_data)
    {
        fprintf(stderr, "Failed to read data from %s\n", serialized_file_path);
        return;
    }
    size_t dec_size = 0;
    unsigned char *expected_data = snappy_decode(comp_data, comp_size, &dec_size);
    free(comp_data);
    if (!expected_data)
    {
        fprintf(stderr, "Failed to decode Snappy data from %s\n", serialized_file_path);
        return;
    }
    unsigned int bit_size;
    char variant[64];
    if (sscanf(folder_name, "uint_%u_%63s", &bit_size, variant) != 2)
    {
        free(expected_data);
        return;
    }
    void *in_mem = NULL;
    ssz_error_t des_err;
    if (bit_size == 8)
    {
        in_mem = malloc(sizeof(uint8_t));
        if (!in_mem)
        {
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
            free(expected_data);
            return;
        }
        des_err = ssz_deserialize_uint256(expected_data, dec_size, in_mem);
    }
    else
    {
        free(expected_data);
        return;
    }
    if (des_err != SSZ_SUCCESS)
    {
        fprintf(stderr, "Deserialization error in folder %s: %d\n", folder_name, des_err);
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
        return;
    }
    free(in_mem);
    printf("\nFolder: %s | BitSize: %u | Variant: %s\n", folder_name, bit_size, variant);
    if (ser_err != SSZ_SUCCESS)
    {
        fprintf(stderr, "Serialization error in folder %s: %d\n", folder_name, ser_err);
    }
    else
    {
        if (out_size != dec_size)
            printf("Size mismatch for folder %s: expected %zu, got %zu\n", folder_name, dec_size, out_size);
        else if (memcmp(out_buf, expected_data, out_size) != 0)
            printf("Content mismatch for folder %s\n", folder_name);
        else
            printf("Folder %s: re-serialized output matches expected data!\n", folder_name);
    }
    size_t chunk_count = (out_size + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK;
    uint8_t *packed_chunks = malloc(chunk_count * BYTES_PER_CHUNK);
    if (!packed_chunks)
    {
        perror("malloc");
        free(out_buf);
        free(expected_data);
        return;
    }
    size_t packed_chunk_count = 0;
    ssz_error_t pack_err = ssz_pack(out_buf, 1, out_size, packed_chunks, &packed_chunk_count);
    if (pack_err != SSZ_SUCCESS)
    {
        fprintf(stderr, "Packing error in folder %s: %d\n", folder_name, pack_err);
        free(packed_chunks);
        free(out_buf);
        free(expected_data);
        return;
    }
    uint8_t merkle_root[BYTES_PER_CHUNK];
    ssz_error_t merkle_err = ssz_merkleize(packed_chunks, packed_chunk_count, chunk_count, merkle_root);
    if (merkle_err != SSZ_SUCCESS)
    {
        fprintf(stderr, "Merkleization error in folder %s: %d\n", folder_name, merkle_err);
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
        fprintf(stderr, "Failed to read 'root' field from %s\n", meta_yaml_path);
    }
    else
    {
        if (yaml_size != BYTES_PER_CHUNK)
            printf("Meta.yaml 'root' field size mismatch for folder %s: expected %d, got %zu\n", folder_name, BYTES_PER_CHUNK, yaml_size);
        else if (memcmp(yaml_data, merkle_root, BYTES_PER_CHUNK) != 0)
            printf("Meta.yaml 'root' field does not match Merkle root for folder %s.\n", folder_name);
        else
            printf("Meta.yaml 'root' field matches Merkle root for folder %s!\n", folder_name);
        free(yaml_data);
    }
    free(out_buf);
    free(expected_data);
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
        process_serialized_file(entry->d_name, serialized_file_path);
    }
    closedir(dir);
    return EXIT_SUCCESS;
}