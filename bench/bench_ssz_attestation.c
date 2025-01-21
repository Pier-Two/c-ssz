#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "benchmark.h"
#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "yaml_parser.h"

#define YAML_FILE_PATH "./bench/data/attestation.yaml"
#define MAX_VALIDATORS_PER_COMMITTEE 2048

typedef struct
{
    uint64_t epoch;
    uint8_t root[32];
} Checkpoint;

typedef struct
{
    uint64_t slot;
    uint64_t index;
    uint8_t beacon_block_root[32];
    Checkpoint source;
    Checkpoint target;
} AttestationData;

typedef struct
{
    uint64_t length;
    bool *data;
} AggregationBits;

typedef struct
{
    AggregationBits aggregation_bits;
    AttestationData data;
    uint8_t signature[96];
} Attestation;

typedef struct
{
    uint8_t *data;
    size_t size;
    size_t capacity;
} DynamicBuffer;

static void db_init(DynamicBuffer *db)
{
    db->data = NULL;
    db->size = 0;
    db->capacity = 0;
}

static void db_free(DynamicBuffer *db)
{
    free(db->data);
    db->data = NULL;
    db->size = 0;
    db->capacity = 0;
}

static ssz_error_t db_ensure_capacity(DynamicBuffer *db, size_t needed)
{
    if (db->size + needed <= db->capacity)
    {
        return SSZ_SUCCESS;
    }
    size_t new_capacity = db->capacity == 0 ? 128 : db->capacity;
    while (new_capacity < db->size + needed)
    {
        new_capacity *= 2;
    }
    uint8_t *temp = realloc(db->data, new_capacity);
    if (!temp)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    db->data = temp;
    db->capacity = new_capacity;
    return SSZ_SUCCESS;
}

static ssz_error_t db_append(DynamicBuffer *db, const uint8_t *bytes, size_t len)
{
    ssz_error_t err = db_ensure_capacity(db, len);
    if (err != SSZ_SUCCESS)
    {
        return err;
    }
    memcpy(db->data + db->size, bytes, len);
    db->size += len;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_serialize_uint_db(uint64_t value, size_t byte_count, DynamicBuffer *db)
{
    uint8_t tmp[8];
    memset(tmp, 0, sizeof(tmp));
    for (int i = 0; i < (int)byte_count; i++)
    {
        tmp[i] = (uint8_t)((value >> (8 * i)) & 0xff);
    }
    return db_append(db, tmp, byte_count);
}

static inline ssz_error_t ssz_serialize_uint64_db(const uint64_t *value, DynamicBuffer *db)
{
    return ssz_serialize_uint_db(*value, 8, db);
}

static inline ssz_error_t ssz_serialize_uint32_db(uint32_t value, DynamicBuffer *db)
{
    return ssz_serialize_uint_db(value, 4, db);
}

static ssz_error_t ssz_serialize_bytes_db(const uint8_t *src, size_t len, DynamicBuffer *db)
{
    return db_append(db, src, len);
}

static ssz_error_t ssz_serialize_bits_generic_db(const bool *bits, size_t bit_count, DynamicBuffer *db)
{
    size_t byte_count = (bit_count + 7) / 8;
    uint8_t *tmp = malloc(byte_count);
    if (!tmp)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }
    memset(tmp, 0, byte_count);
    for (size_t i = 0; i < bit_count; i++)
    {
        if (bits[i])
        {
            tmp[i / 8] |= (1 << (i % 8));
        }
    }
    ssz_error_t err = db_append(db, tmp, byte_count);
    free(tmp);
    return err;
}

static ssz_error_t ssz_serialize_bitlist_db(const bool *bits, size_t bit_count, DynamicBuffer *db)
{
    return ssz_serialize_bits_generic_db(bits, bit_count, db);
}

static void print_hex(const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        printf("%02x", data[i]);
    }
    printf("\n");
}

static void print_bits_as_hex(const AggregationBits *agg_bits)
{
    size_t bit_count = agg_bits->length;
    size_t byte_count = (bit_count + 7) / 8;
    uint8_t *tmp = (uint8_t *)malloc(byte_count);
    if (!tmp)
    {
        printf("Error: Could not allocate memory for bits printing.\n");
        return;
    }
    memset(tmp, 0, byte_count);
    for (size_t i = 0; i < bit_count; i++)
    {
        if (agg_bits->data[i])
        {
            tmp[i / 8] |= (1 << (i % 8));
        }
    }
    print_hex(tmp, byte_count);
    free(tmp);
}

static void print_attestation(const Attestation *f)
{
    printf("Attestation:\n");
    printf("  aggregation_bits (hex): 0x");
    print_bits_as_hex(&f->aggregation_bits);
    printf("  data.slot: %llu\n", (unsigned long long)f->data.slot);
    printf("  data.index: %llu\n", (unsigned long long)f->data.index);
    printf("  data.beacon_block_root: 0x");
    print_hex(f->data.beacon_block_root, 32);
    printf("  data.source.epoch: %llu\n", (unsigned long long)f->data.source.epoch);
    printf("  data.source.root: 0x");
    print_hex(f->data.source.root, 32);
    printf("  data.target.epoch: %llu\n", (unsigned long long)f->data.target.epoch);
    printf("  data.target.root: 0x");
    print_hex(f->data.target.root, 32);
    printf("  signature: 0x");
    print_hex(f->signature, 96);
}

static ssz_error_t ssz_serialize_attestation_data_db(const AttestationData *ad, DynamicBuffer *db)
{
    ssz_error_t err;
    err = ssz_serialize_uint64_db(&ad->slot, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_uint64_db(&ad->index, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_bytes_db(ad->beacon_block_root, 32, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_uint64_db(&ad->source.epoch, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_bytes_db(ad->source.root, 32, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_uint64_db(&ad->target.epoch, db);
    if (err != SSZ_SUCCESS)
        return err;
    err = ssz_serialize_bytes_db(ad->target.root, 32, db);
    if (err != SSZ_SUCCESS)
        return err;
    return SSZ_SUCCESS;
}

static ssz_error_t serialize_attestation(const Attestation *attestation_data, uint8_t *out_buffer, size_t out_buffer_size, size_t *out_actual_size)
{
    DynamicBuffer db_fixed;
    DynamicBuffer db_var;
    DynamicBuffer db_final;
    db_init(&db_fixed);
    db_init(&db_var);
    db_init(&db_final);
    ssz_error_t err = SSZ_SUCCESS;
    err = ssz_serialize_attestation_data_db(&attestation_data->data, &db_fixed);
    if (err != SSZ_SUCCESS)
        goto fin;
    err = ssz_serialize_bytes_db(attestation_data->signature, 96, &db_fixed);
    if (err != SSZ_SUCCESS)
        goto fin;
    err = ssz_serialize_bitlist_db(attestation_data->aggregation_bits.data, attestation_data->aggregation_bits.length, &db_var);
    if (err != SSZ_SUCCESS)
        goto fin;
    uint32_t offset_agg_bits = 4 + (uint32_t)db_fixed.size;
    err = ssz_serialize_uint32_db(offset_agg_bits, &db_final);
    if (err != SSZ_SUCCESS)
        goto fin;
    err = db_append(&db_final, db_fixed.data, db_fixed.size);
    if (err != SSZ_SUCCESS)
        goto fin;
    err = db_append(&db_final, db_var.data, db_var.size);
    if (err != SSZ_SUCCESS)
        goto fin;
    if (db_final.size > out_buffer_size)
    {
        err = SSZ_ERROR_SERIALIZATION;
        goto fin;
    }
    memcpy(out_buffer, db_final.data, db_final.size);
    *out_actual_size = db_final.size;
fin:
    db_free(&db_fixed);
    db_free(&db_var);
    db_free(&db_final);
    return err;
}

static ssz_error_t deserialize_attestation(const uint8_t *buffer, size_t buffer_size, Attestation *out_attestation)
{
    if (buffer_size < 4)
        return SSZ_ERROR_DESERIALIZATION;
    uint32_t agg_offset = 0;
    ssz_error_t err = ssz_deserialize_uintN(buffer, 4, 32, &agg_offset);
    if (err != SSZ_SUCCESS)
        return err;
    if (agg_offset > buffer_size)
        return SSZ_ERROR_DESERIALIZATION;
    size_t fixed_part_size = 224;
    if (agg_offset < 4 + fixed_part_size)
        return SSZ_ERROR_DESERIALIZATION;
    if (buffer_size < 4 + fixed_part_size)
        return SSZ_ERROR_DESERIALIZATION;
    if ((agg_offset - 4) != fixed_part_size)
        return SSZ_ERROR_DESERIALIZATION;
    memset(out_attestation, 0, sizeof(*out_attestation));
    size_t offset = 4;
    err = ssz_deserialize_uintN(buffer + offset, 8, 64, &out_attestation->data.slot);
    if (err != SSZ_SUCCESS)
        return err;
    offset += 8;
    err = ssz_deserialize_uintN(buffer + offset, 8, 64, &out_attestation->data.index);
    if (err != SSZ_SUCCESS)
        return err;
    offset += 8;
    memcpy(out_attestation->data.beacon_block_root, buffer + offset, 32);
    offset += 32;
    err = ssz_deserialize_uintN(buffer + offset, 8, 64, &out_attestation->data.source.epoch);
    if (err != SSZ_SUCCESS)
        return err;
    offset += 8;
    memcpy(out_attestation->data.source.root, buffer + offset, 32);
    offset += 32;
    err = ssz_deserialize_uintN(buffer + offset, 8, 64, &out_attestation->data.target.epoch);
    if (err != SSZ_SUCCESS)
        return err;
    offset += 8;
    memcpy(out_attestation->data.target.root, buffer + offset, 32);
    offset += 32;
    memcpy(out_attestation->signature, buffer + offset, 96);
    offset += 96;
    if (agg_offset < buffer_size)
    {
        size_t bitlist_bytes = buffer_size - agg_offset;
        out_attestation->aggregation_bits.data = malloc(MAX_VALIDATORS_PER_COMMITTEE * sizeof(bool));
        if (!out_attestation->aggregation_bits.data)
            return SSZ_ERROR_DESERIALIZATION;
        size_t actual_bits = 0;
        err = ssz_deserialize_bitlist(buffer + agg_offset, bitlist_bytes, MAX_VALIDATORS_PER_COMMITTEE, out_attestation->aggregation_bits.data, &actual_bits);
        if (err != SSZ_SUCCESS)
        {
            free(out_attestation->aggregation_bits.data);
            out_attestation->aggregation_bits.data = NULL;
            return err;
        }
        out_attestation->aggregation_bits.length = actual_bits;
    }

    return SSZ_SUCCESS;
}

static Attestation g_original;
static uint8_t g_serialized[2048];
static size_t g_serialized_size = 0;

static bool read_uint64_field(const char *yaml_path, const char *field_name, uint64_t *out_val)
{
    size_t sz = 0;
    uint8_t *data = read_yaml_field(yaml_path, field_name, &sz);
    if (data && sz == 8)
    {
        uint64_t val = 0;
        for (int i = 0; i < 8; i++)
        {
            val |= ((uint64_t)data[i]) << (8 * i);
        }
        *out_val = val;
        free(data);
        return true;
    }
    free(data);
    return false;
}

static bool read_32bytes_field(const char *yaml_path, const char *field_name, uint8_t out_buf[32])
{
    size_t sz = 0;
    uint8_t *d = read_yaml_field(yaml_path, field_name, &sz);
    if (d && sz == 32)
    {
        memcpy(out_buf, d, 32);
        free(d);
        return true;
    }
    free(d);
    return false;
}

static void init_attestation_data_from_yaml(void)
{
    memset(&g_original, 0, sizeof(g_original));
    {
        size_t sz = 0;
        uint8_t *data = read_yaml_field(YAML_FILE_PATH, "aggregation_bits", &sz);
        if (data && sz > 0)
        {
            size_t bit_count = sz * 8;
            g_original.aggregation_bits.length = bit_count;
            g_original.aggregation_bits.data = malloc(bit_count * sizeof(bool));
            for (size_t i = 0; i < bit_count; i++)
            {
                bool val = (data[i / 8] >> (i % 8)) & 1;
                g_original.aggregation_bits.data[i] = val;
            }
        }
        free(data);
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.slot", &tmpval))
        {
            g_original.data.slot = tmpval;
        }
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.index", &tmpval))
        {
            g_original.data.index = tmpval;
        }
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.beacon_block_root", buf))
        {
            memcpy(g_original.data.beacon_block_root, buf, 32);
        }
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.source.epoch", &tmpval))
        {
            g_original.data.source.epoch = tmpval;
        }
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.source.root", buf))
        {
            memcpy(g_original.data.source.root, buf, 32);
        }
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.target.epoch", &tmpval))
        {
            g_original.data.target.epoch = tmpval;
        }
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.target.root", buf))
        {
            memcpy(g_original.data.target.root, buf, 32);
        }
    }
    {
        size_t sz = 0;
        uint8_t *d = read_yaml_field(YAML_FILE_PATH, "signature", &sz);
        if (d && sz == 96)
        {
            memcpy(g_original.signature, d, 96);
        }
        free(d);
    }
}

static void attestation_bench_test_func_serialize(void *user_data)
{
    (void)user_data;
    memset(g_serialized, 0, sizeof(g_serialized));
    g_serialized_size = 0;
    size_t actual_size = 0;
    ssz_error_t err = serialize_attestation(&g_original, g_serialized, sizeof(g_serialized), &actual_size);
    if (err != SSZ_SUCCESS)
    {
        printf("Failed to serialize\n");
    }
    else
    {
        g_serialized_size = actual_size;
    }
}

static void attestation_bench_test_func_deserialize(void *user_data)
{
    (void)user_data;
    Attestation tmp;
    memset(&tmp, 0, sizeof(tmp));
    ssz_error_t err = deserialize_attestation(g_serialized, g_serialized_size, &tmp);
    if (err != SSZ_SUCCESS)
    {
        printf("Failed to deserialize\n");
    }
    else
    {
        if (tmp.aggregation_bits.data)
        {
            free(tmp.aggregation_bits.data);
            tmp.aggregation_bits.data = NULL;
        }
    }
}

int main(void)
{
    init_attestation_data_from_yaml();
    unsigned long warmup_iterations = 0;
    unsigned long measured_iterations = 10;
    bench_ssz_stats_t stats_serialize = bench_ssz_run_benchmark(
        attestation_bench_test_func_serialize,
        NULL,
        warmup_iterations,
        measured_iterations);
    bench_ssz_stats_t stats_deserialize = bench_ssz_run_benchmark(
        attestation_bench_test_func_deserialize,
        NULL,
        warmup_iterations,
        measured_iterations);
    
    print_attestation(&g_original);
    bench_ssz_print_stats("SSZ Attestation serialization", &stats_serialize);
    bench_ssz_print_stats("SSZ Attestation deserialization", &stats_deserialize);
    return 0;
}