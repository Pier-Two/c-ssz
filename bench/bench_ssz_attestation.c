#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/sha.h>
#include "bench.h"
#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"
#include "yaml_parser.h"
#include "ssz_merkle.h"
#include "ssz_utils.h"

#define YAML_FILE_PATH "./bench/data/attestation.yaml"
#define MAX_VALIDATORS_PER_COMMITTEE 2048

typedef struct {
    uint64_t epoch;
    uint8_t root[32];
} Checkpoint;

typedef struct {
    uint64_t slot;
    uint64_t index;
    uint8_t beacon_block_root[32];
    Checkpoint source;
    Checkpoint target;
} AttestationData;

typedef struct {
    uint64_t length;
    bool *data;
} AggregationBits;

typedef struct {
    AggregationBits aggregation_bits;
    AttestationData data;
    uint8_t signature[96];
} Attestation;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} DynamicBuffer;

static void db_init(DynamicBuffer *db) {
    db->data = NULL;
    db->size = 0;
    db->capacity = 0;
}

static void db_free(DynamicBuffer *db) {
    free(db->data);
    db->data = NULL;
    db->size = 0;
    db->capacity = 0;
}

static ssz_error_t db_ensure_capacity(DynamicBuffer *db, size_t needed) {
    if (db->size + needed <= db->capacity) return SSZ_SUCCESS;
    size_t new_capacity = db->capacity == 0 ? 128 : db->capacity;
    while (new_capacity < db->size + needed) new_capacity *= 2;
    uint8_t *temp = realloc(db->data, new_capacity);
    if (!temp) return SSZ_ERROR_DESERIALIZATION;
    db->data = temp;
    db->capacity = new_capacity;
    return SSZ_SUCCESS;
}

static ssz_error_t db_append(DynamicBuffer *db, const uint8_t *bytes, size_t len) {
    ssz_error_t err = db_ensure_capacity(db, len);
    if (err != SSZ_SUCCESS) return err;
    memcpy(db->data + db->size, bytes, len);
    db->size += len;
    return SSZ_SUCCESS;
}

static void print_hex(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) printf("%02x", data[i]);
    printf("\n");
}

static void print_bits_array(const AggregationBits *agg_bits) {
    for (size_t i = 0; i < agg_bits->length; i++) {
        printf("%d", agg_bits->data[i] ? 1 : 0);
    }
    printf("\n");
}

static void print_attestation(const Attestation *f) {
    printf("Attestation:\n");
    printf("  aggregation_bits (binary): ");
    print_bits_array(&f->aggregation_bits);
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

static ssz_error_t ssz_serialize_attestation_data_db(const AttestationData *ad, DynamicBuffer *db) {
    ssz_error_t err;
    uint8_t tmp[64];
    size_t tmp_size = sizeof(tmp);
    err = ssz_serialize_uint64(&ad->slot, tmp, &tmp_size);
    if (err != SSZ_SUCCESS) return err;
    err = db_append(db, tmp, tmp_size);
    if (err != SSZ_SUCCESS) return err;
    tmp_size = sizeof(tmp);
    err = ssz_serialize_uint64(&ad->index, tmp, &tmp_size);
    if (err != SSZ_SUCCESS) return err;
    err = db_append(db, tmp, tmp_size);
    if (err != SSZ_SUCCESS) return err;
    {
        uint8_t vec_buf[64];
        size_t vec_size = sizeof(vec_buf);
        size_t element_count = 32;
        size_t sizes[32];
        for (size_t i = 0; i < element_count; i++) sizes[i] = 1;
        err = ssz_serialize_vector_uint8(ad->beacon_block_root, element_count, vec_buf, &vec_size);
        if (err != SSZ_SUCCESS) return err;
        err = db_append(db, vec_buf, vec_size);
        if (err != SSZ_SUCCESS) return err;
    }
    tmp_size = sizeof(tmp);
    err = ssz_serialize_uint64(&ad->source.epoch, tmp, &tmp_size);
    if (err != SSZ_SUCCESS) return err;
    err = db_append(db, tmp, tmp_size);
    if (err != SSZ_SUCCESS) return err;
    {
        uint8_t vec_buf[64];
        size_t vec_size = sizeof(vec_buf);
        size_t element_count = 32;
        size_t sizes[32];
        for (size_t i = 0; i < element_count; i++) sizes[i] = 1;
        err = ssz_serialize_vector_uint8(ad->source.root, element_count, vec_buf, &vec_size);
        if (err != SSZ_SUCCESS) return err;
        err = db_append(db, vec_buf, vec_size);
        if (err != SSZ_SUCCESS) return err;
    }
    tmp_size = sizeof(tmp);
    err = ssz_serialize_uint64(&ad->target.epoch, tmp, &tmp_size);
    if (err != SSZ_SUCCESS) return err;
    err = db_append(db, tmp, tmp_size);
    if (err != SSZ_SUCCESS) return err;
    {
        uint8_t vec_buf[64];
        size_t vec_size = sizeof(vec_buf);
        size_t element_count = 32;
        size_t sizes[32];
        for (size_t i = 0; i < element_count; i++) sizes[i] = 1;
        err = ssz_serialize_vector_uint8(ad->target.root, element_count, vec_buf, &vec_size);
        if (err != SSZ_SUCCESS) return err;
        err = db_append(db, vec_buf, vec_size);
        if (err != SSZ_SUCCESS) return err;
    }
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_serialize_signature(const uint8_t *signature, DynamicBuffer *db) {
    ssz_error_t err;
    uint8_t vec_buf[128];
    size_t vec_size = sizeof(vec_buf);
    size_t element_count = 96;
    size_t sizes[96];
    for (size_t i = 0; i < element_count; i++) sizes[i] = 1;
    err = ssz_serialize_vector_uint8(signature, element_count, vec_buf, &vec_size);
    if (err != SSZ_SUCCESS) return err;
    err = db_append(db, vec_buf, vec_size);
    if (err != SSZ_SUCCESS) return err;
    return SSZ_SUCCESS;
}

static ssz_error_t ssz_serialize_aggregation_bits(const AggregationBits *bits, DynamicBuffer *db) {
    ssz_error_t err;
    uint8_t temp[256];
    size_t temp_size = sizeof(temp);
    err = ssz_serialize_bitlist(bits->data, bits->length, temp, &temp_size);
    if (err != SSZ_SUCCESS) return err;
    return db_append(db, temp, temp_size);
}

static ssz_error_t serialize_attestation(const Attestation *attestation_data, uint8_t *out_buffer, size_t out_buffer_size, size_t *out_actual_size) {
    DynamicBuffer db_fixed;
    DynamicBuffer db_var;
    DynamicBuffer db_final;
    db_init(&db_fixed);
    db_init(&db_var);
    db_init(&db_final);
    ssz_error_t err = SSZ_SUCCESS;
    err = ssz_serialize_attestation_data_db(&attestation_data->data, &db_fixed);
    if (err != SSZ_SUCCESS) goto fin;
    err = ssz_serialize_signature(attestation_data->signature, &db_fixed);
    if (err != SSZ_SUCCESS) goto fin;
    err = ssz_serialize_aggregation_bits(&attestation_data->aggregation_bits, &db_var);
    if (err != SSZ_SUCCESS) goto fin;
    uint32_t offset_agg_bits = 4 + (uint32_t)db_fixed.size;
    {
        uint8_t tmp[4];
        size_t tmp_size = sizeof(tmp);
        uint32_t val = offset_agg_bits;
        err = ssz_serialize_uint32(&val, tmp, &tmp_size);
        if (err != SSZ_SUCCESS) goto fin;
        err = db_append(&db_final, tmp, tmp_size);
        if (err != SSZ_SUCCESS) goto fin;
    }
    err = db_append(&db_final, db_fixed.data, db_fixed.size);
    if (err != SSZ_SUCCESS) goto fin;
    err = db_append(&db_final, db_var.data, db_var.size);
    if (err != SSZ_SUCCESS) goto fin;
    if (db_final.size > out_buffer_size) {
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

static ssz_error_t deserialize_attestation(const uint8_t *buffer, size_t buffer_size, Attestation *out_attestation) {
    if (buffer_size < 4) return SSZ_ERROR_DESERIALIZATION;
    uint32_t agg_offset = 0;
    ssz_error_t err = ssz_deserialize_uint32(buffer, 4, &agg_offset);
    if (err != SSZ_SUCCESS) return err;
    if (agg_offset > buffer_size) return SSZ_ERROR_DESERIALIZATION;
    size_t fixed_part_size = 224;
    if (agg_offset < 4 + fixed_part_size) return SSZ_ERROR_DESERIALIZATION;
    if (buffer_size < 4 + fixed_part_size) return SSZ_ERROR_DESERIALIZATION;
    if ((agg_offset - 4) != fixed_part_size) return SSZ_ERROR_DESERIALIZATION;
    memset(out_attestation, 0, sizeof(*out_attestation));
    size_t offset = 4;
    err = ssz_deserialize_uint64(buffer + offset, 8, &out_attestation->data.slot);
    if (err != SSZ_SUCCESS) return err;
    offset += 8;
    err = ssz_deserialize_uint64(buffer + offset, 8, &out_attestation->data.index);
    if (err != SSZ_SUCCESS) return err;
    offset += 8;
    memcpy(out_attestation->data.beacon_block_root, buffer + offset, 32);
    offset += 32;
    err = ssz_deserialize_uint64(buffer + offset, 8, &out_attestation->data.source.epoch);
    if (err != SSZ_SUCCESS) return err;
    offset += 8;
    memcpy(out_attestation->data.source.root, buffer + offset, 32);
    offset += 32;
    err = ssz_deserialize_uint64(buffer + offset, 8, &out_attestation->data.target.epoch);
    if (err != SSZ_SUCCESS) return err;
    offset += 8;
    memcpy(out_attestation->data.target.root, buffer + offset, 32);
    offset += 32;
    memcpy(out_attestation->signature, buffer + offset, 96);
    offset += 96;
    if (agg_offset < buffer_size) {
        size_t bitlist_bytes = buffer_size - agg_offset;
        out_attestation->aggregation_bits.data = malloc(MAX_VALIDATORS_PER_COMMITTEE * sizeof(bool));
        if (!out_attestation->aggregation_bits.data) return SSZ_ERROR_DESERIALIZATION;
        size_t actual_bits = 0;
        err = ssz_deserialize_bitlist(buffer + agg_offset, bitlist_bytes, MAX_VALIDATORS_PER_COMMITTEE, out_attestation->aggregation_bits.data, &actual_bits);
        if (err != SSZ_SUCCESS) {
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

static bool read_uint64_field(const char *yaml_path, const char *field_name, uint64_t *out_val) {
    size_t sz = 0;
    uint8_t *data = read_yaml_field(yaml_path, field_name, &sz);
    if (data && sz == 8) {
        uint64_t val = 0;
        for (int i = 0; i < 8; i++) val |= ((uint64_t)data[i]) << (8 * i);
        *out_val = val;
        free(data);
        return true;
    }
    free(data);
    return false;
}

static bool read_32bytes_field(const char *yaml_path, const char *field_name, uint8_t out_buf[32]) {
    size_t sz = 0;
    uint8_t *d = read_yaml_field(yaml_path, field_name, &sz);
    if (d && sz == 32) {
        memcpy(out_buf, d, 32);
        free(d);
        return true;
    }
    free(d);
    return false;
}

static void init_attestation_data_from_yaml(void) {
    memset(&g_original, 0, sizeof(g_original));
    {
        size_t sz = 0;
        uint8_t *data = read_yaml_field(YAML_FILE_PATH, "aggregation_bits", &sz);
        if (data && sz > 0) {
            g_original.aggregation_bits.data = malloc(MAX_VALIDATORS_PER_COMMITTEE * sizeof(bool));
            if (g_original.aggregation_bits.data) {
                size_t actual_bits = 0;
                ssz_error_t e = ssz_deserialize_bitlist(data, sz, MAX_VALIDATORS_PER_COMMITTEE, g_original.aggregation_bits.data, &actual_bits);
                if (e == SSZ_SUCCESS) {
                    g_original.aggregation_bits.length = actual_bits;
                }
            }
        }
        free(data);
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.slot", &tmpval)) g_original.data.slot = tmpval;
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.index", &tmpval)) g_original.data.index = tmpval;
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.beacon_block_root", buf)) memcpy(g_original.data.beacon_block_root, buf, 32);
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.source.epoch", &tmpval)) g_original.data.source.epoch = tmpval;
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.source.root", buf)) memcpy(g_original.data.source.root, buf, 32);
    }
    {
        uint64_t tmpval;
        if (read_uint64_field(YAML_FILE_PATH, "data.target.epoch", &tmpval)) g_original.data.target.epoch = tmpval;
    }
    {
        uint8_t buf[32];
        if (read_32bytes_field(YAML_FILE_PATH, "data.target.root", buf)) memcpy(g_original.data.target.root, buf, 32);
    }
    {
        size_t sz = 0;
        uint8_t *d = read_yaml_field(YAML_FILE_PATH, "signature", &sz);
        if (d && sz == 96) memcpy(g_original.signature, d, 96);
        free(d);
    }
}

static void attestation_bench_test_func_serialize(void *user_data) {
    (void)user_data;
    memset(g_serialized, 0, sizeof(g_serialized));
    g_serialized_size = 0;
    size_t actual_size = 0;
    ssz_error_t err = serialize_attestation(&g_original, g_serialized, sizeof(g_serialized), &actual_size);
    if (err == SSZ_SUCCESS) {
        g_serialized_size = actual_size;
    }
}

static void attestation_bench_test_func_deserialize(void *user_data) {
    (void)user_data;
    Attestation tmp;
    memset(&tmp, 0, sizeof(tmp));
    ssz_error_t err = deserialize_attestation(g_serialized, g_serialized_size, &tmp);
    if (err == SSZ_SUCCESS) {
        if (tmp.aggregation_bits.data) {
            free(tmp.aggregation_bits.data);
            tmp.aggregation_bits.data = NULL;
        }
    }
}

static ssz_error_t hash_tree_root_uint64(uint64_t value, uint8_t *out_root) {
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) buf[i] = (uint8_t)((value >> (8 * i)) & 0xFF);
    uint8_t packed[BYTES_PER_CHUNK];
    size_t chunk_count;
    ssz_error_t err = ssz_pack(buf, 1, 8, packed, &chunk_count);
    if (err != SSZ_SUCCESS) return err;
    return ssz_merkleize(packed, chunk_count, 0, out_root);
}

static ssz_error_t hash_tree_root_bytes(const uint8_t *bytes, size_t length, uint8_t *out_root) {
    size_t chunk_count;
    size_t alloc_size = ((length + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK) * BYTES_PER_CHUNK;
    uint8_t *packed = malloc(alloc_size);
    if (!packed) return SSZ_ERROR_SERIALIZATION;
    ssz_error_t err = ssz_pack(bytes, 1, length, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return err;
}

static ssz_error_t hash_tree_root_checkpoint(const Checkpoint *cp, uint8_t *out_root) {
    uint8_t root_epoch[32], root_root[32];
    ssz_error_t err = hash_tree_root_uint64(cp->epoch, root_epoch);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_bytes(cp->root, 32, root_root);
    if (err != SSZ_SUCCESS) return err;
    uint8_t nodes[2][32];
    memcpy(nodes[0], root_epoch, 32);
    memcpy(nodes[1], root_root, 32);
    return ssz_merkleize((uint8_t*)nodes, 2, 0, out_root);
}

static ssz_error_t hash_tree_root_attestation_data(const AttestationData *data, uint8_t *out_root) {
    uint8_t root_slot[32], root_index[32], root_beacon[32], root_source[32], root_target[32];
    ssz_error_t err = hash_tree_root_uint64(data->slot, root_slot);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_uint64(data->index, root_index);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_bytes(data->beacon_block_root, 32, root_beacon);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_checkpoint(&data->source, root_source);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_checkpoint(&data->target, root_target);
    if (err != SSZ_SUCCESS) return err;
    uint8_t nodes[5][32];
    memcpy(nodes[0], root_slot, 32);
    memcpy(nodes[1], root_index, 32);
    memcpy(nodes[2], root_beacon, 32);
    memcpy(nodes[3], root_source, 32);
    memcpy(nodes[4], root_target, 32);
    return ssz_merkleize((uint8_t*)nodes, 5, 0, out_root);
}

static ssz_error_t hash_tree_root_aggregation_bits(const AggregationBits *bits, uint8_t *out_root) {
    size_t limit = (MAX_VALIDATORS_PER_COMMITTEE + 255) / 256;  
    size_t alloc_size = limit * BYTES_PER_CHUNK;
    uint8_t *packed = malloc(alloc_size);
    if (!packed) return SSZ_ERROR_SERIALIZATION;
    memset(packed, 0, alloc_size);  
    size_t actual_bytes = (bits->length + 7) / 8;
    ssz_error_t err = ssz_pack_bits(bits->data, bits->length, packed, &actual_bytes);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    err = ssz_merkleize(packed, limit, limit, out_root); 
    free(packed);
    if (err != SSZ_SUCCESS) return err;
    err = ssz_mix_in_length(out_root, bits->length, out_root);
    if (err != SSZ_SUCCESS) return err;
    return SSZ_SUCCESS;
}

static ssz_error_t hash_tree_root_signature(const uint8_t *signature, uint8_t *out_root) {
    size_t chunk_count;
    size_t alloc_size = ((96 + BYTES_PER_CHUNK - 1) / BYTES_PER_CHUNK) * BYTES_PER_CHUNK;
    uint8_t *packed = malloc(alloc_size);
    if (!packed) return SSZ_ERROR_SERIALIZATION;
    ssz_error_t err = ssz_pack(signature, 1, 96, packed, &chunk_count);
    if (err != SSZ_SUCCESS) {
        free(packed);
        return err;
    }
    err = ssz_merkleize(packed, chunk_count, 0, out_root);
    free(packed);
    return err;
}

static ssz_error_t hash_tree_root_attestation(const Attestation *att, uint8_t *out_root) {
    uint8_t root_agg[32], root_data[32], root_signature[32];
    ssz_error_t err = hash_tree_root_aggregation_bits(&att->aggregation_bits, root_agg);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_attestation_data(&att->data, root_data);
    if (err != SSZ_SUCCESS) return err;
    err = hash_tree_root_signature(att->signature, root_signature);
    if (err != SSZ_SUCCESS) return err;
    uint8_t nodes[3][32];
    memcpy(nodes[0], root_agg, 32);
    memcpy(nodes[1], root_data, 32);
    memcpy(nodes[2], root_signature, 32);
    return ssz_merkleize((uint8_t*)nodes, 3, 0, out_root);
}

static void print_attestation_tree(const Attestation *att) {
    uint8_t root_agg[32], root_data[32], root_signature[32], final_root[32];
    if (hash_tree_root_aggregation_bits(&att->aggregation_bits, root_agg) != SSZ_SUCCESS) {
        printf("Error computing aggregation_bits root\n");
        return;
    }
    if (hash_tree_root_attestation_data(&att->data, root_data) != SSZ_SUCCESS) {
        printf("Error computing data root\n");
        return;
    }
    if (hash_tree_root_signature(att->signature, root_signature) != SSZ_SUCCESS) {
        printf("Error computing signature root\n");
        return;
    }
    printf("Leaves:\n");
    printf("  aggregation_bits: 0x");
    print_hex(root_agg, 32);
    printf("  data: 0x");
    print_hex(root_data, 32);
    printf("  signature: 0x");
    print_hex(root_signature, 32);
    uint8_t nodes[4][32];
    memcpy(nodes[0], root_agg, 32);
    memcpy(nodes[1], root_data, 32);
    memcpy(nodes[2], root_signature, 32);
    memset(nodes[3], 0, 32);
    printf("Level 1:\n");
    uint8_t parent[2][32];
    for (int i = 0; i < 2; i++) {
        uint8_t concat[64];
        memcpy(concat, nodes[2 * i], 32);
        memcpy(concat + 32, nodes[2 * i + 1], 32);
        SHA256(concat, 64, parent[i]);
        printf("  Node %d: 0x", i);
        print_hex(parent[i], 32);
    }
    printf("Level 2 (Merkle Root):\n");
    uint8_t concat[64];
    memcpy(concat, parent[0], 32);
    memcpy(concat + 32, parent[1], 32);
    SHA256(concat, 64, final_root);
    printf("  Merkle Root: 0x");
    print_hex(final_root, 32);
}

int main(void) {
    init_attestation_data_from_yaml();
    unsigned long warmup_iterations = 0;
    unsigned long measured_iterations = 100000;
    bench_ssz_stats_t stats_serialize = bench_ssz_run_benchmark(attestation_bench_test_func_serialize, NULL, warmup_iterations, measured_iterations);
    bench_ssz_stats_t stats_deserialize = bench_ssz_run_benchmark(attestation_bench_test_func_deserialize, NULL, warmup_iterations, measured_iterations);
    print_attestation(&g_original);
    uint8_t merkle_root[32];
    if (hash_tree_root_attestation(&g_original, merkle_root) == SSZ_SUCCESS) {
        printf("\nDetailed Merkle Tree:\n");
        print_attestation_tree(&g_original);
    } else {
        printf("Failed to compute hash tree root\n");
    }
    printf("\nSerialized form:\n0x");
    print_hex(g_serialized, g_serialized_size);
    bench_ssz_print_stats("SSZ Attestation serialization", &stats_serialize);
    bench_ssz_print_stats("SSZ Attestation deserialization", &stats_deserialize);
    return 0;
}