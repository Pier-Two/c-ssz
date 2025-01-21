#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "benchmark.h"
#include "ssz_deserialize.h"

typedef struct {
    size_t bit_size;
    uint8_t buffer[32];
    size_t buffer_size;
} ssz_uintN_deserialize_test_t;

typedef struct {
    uint8_t buffer[1];
    size_t buffer_size;
} ssz_boolean_deserialize_test_t;

typedef struct {
    size_t num_bits;
    uint8_t buffer[32768];
    size_t buffer_size;
} ssz_bitvector_deserialize_test_t;

typedef struct {
    size_t max_bits;
    uint8_t buffer[65537];
    size_t buffer_size;
} ssz_bitlist_deserialize_test_t;

typedef struct {
    uint8_t buffer[131072];
    size_t buffer_size;
    size_t element_count;
    size_t field_sizes[16384];
    bool is_variable_size;
} ssz_vector_deserialize_test_t;

typedef struct {
    uint8_t buffer[131072];
    size_t buffer_size;
    size_t max_element_count;
    size_t field_sizes[16384];
    bool is_variable_size;
} ssz_list_deserialize_test_t;

static void test_uintN_deserialize(void* user_data) {
    ssz_uintN_deserialize_test_t* test_data = (ssz_uintN_deserialize_test_t*)user_data;
    uint64_t out_value = 0;
    ssz_deserialize_uintN(
        test_data->buffer,
        test_data->buffer_size,
        test_data->bit_size,
        &out_value
    );
}

static void test_boolean_deserialize(void* user_data) {
    ssz_boolean_deserialize_test_t* test_data = (ssz_boolean_deserialize_test_t*)user_data;
    bool out_value = false;
    ssz_deserialize_boolean(
        test_data->buffer,
        test_data->buffer_size,
        &out_value
    );
}

static void test_bitvector_deserialize(void* user_data) {
    ssz_bitvector_deserialize_test_t* test_data = (ssz_bitvector_deserialize_test_t*)user_data;
    bool out_bits[262144];
    ssz_deserialize_bitvector(
        test_data->buffer,
        test_data->buffer_size,
        test_data->num_bits,
        out_bits
    );
}

static void test_bitlist_deserialize(void* user_data) {
    ssz_bitlist_deserialize_test_t* test_data = (ssz_bitlist_deserialize_test_t*)user_data;
    bool out_bits[524288];
    size_t out_actual_bits = 0;
    ssz_deserialize_bitlist(
        test_data->buffer,
        test_data->buffer_size,
        test_data->max_bits,
        out_bits,
        &out_actual_bits
    );
}

static void test_vector_deserialize(void* user_data) {
    ssz_vector_deserialize_test_t* test_data = (ssz_vector_deserialize_test_t*)user_data;
    uint64_t out_elements[16384];
    ssz_deserialize_vector(
        test_data->buffer,
        test_data->buffer_size,
        test_data->element_count,
        test_data->field_sizes,
        test_data->is_variable_size,
        out_elements
    );
}

static void test_list_deserialize(void* user_data) {
    ssz_list_deserialize_test_t* test_data = (ssz_list_deserialize_test_t*)user_data;
    uint64_t out_elements[16384];
    size_t out_actual_count = 0;
    ssz_deserialize_list(
        test_data->buffer,
        test_data->buffer_size,
        test_data->max_element_count,
        test_data->field_sizes,
        test_data->is_variable_size,
        out_elements,
        &out_actual_count
    );
}

static void run_uintN_deserialize_benchmarks(void) {
    ssz_uintN_deserialize_test_t tests[] = {
        {8,   {0}, 1},
        {16,  {0}, 2},
        {32,  {0}, 4},
        {64,  {0}, 8}
    };
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        memset(tests[i].buffer, 0xFF, tests[i].buffer_size);
        bench_ssz_stats_t stats = bench_ssz_run_benchmark(
            test_uintN_deserialize,
            &tests[i],
            100000,
            1000000
        );
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_deserialize_uint%zu", tests[i].bit_size);
        bench_ssz_print_stats(label, &stats);
    }
}

static void run_boolean_deserialize_benchmarks(void) {
    ssz_boolean_deserialize_test_t tests[] = {
        {{0x00}, 1},
        {{0x01}, 1}
    };
    for (int i = 0; i < 2; i++) {
        bench_ssz_stats_t stats = bench_ssz_run_benchmark(
            test_boolean_deserialize,
            &tests[i],
            100000,
            1000000
        );
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_deserialize_boolean %s", tests[i].buffer[0] ? "true" : "false");
        bench_ssz_print_stats(label, &stats);
    }
}

static void run_bitvector_deserialize_benchmarks(void) {
    ssz_bitvector_deserialize_test_t test_data;
    test_data.num_bits = 262144;
    test_data.buffer_size = 262144 / 8;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_bitvector_deserialize,
        &test_data,
        100,
        100
    );
    bench_ssz_print_stats("Benchmark ssz_deserialize_bitvector 262144 bits", &stats);
}

static void run_bitlist_deserialize_benchmarks(void) {
    ssz_bitlist_deserialize_test_t test_data;
    test_data.max_bits = 524288;
    test_data.buffer_size = (524288 / 8) + 1;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    test_data.buffer[test_data.buffer_size - 1] = 0x01;
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_bitlist_deserialize,
        &test_data,
        10,
        10
    );
    bench_ssz_print_stats("Benchmark ssz_deserialize_bitlist 524288 bits", &stats);
}

static void run_vector_deserialize_benchmarks(void) {
    ssz_vector_deserialize_test_t test_data;
    memset(test_data.buffer, 0, sizeof(test_data.buffer));
    test_data.buffer_size = 8 * 16384;
    test_data.element_count = 16384;
    for (size_t i = 0; i < 16384; i++) {
        test_data.field_sizes[i] = 8;
    }
    test_data.is_variable_size = false;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_vector_deserialize,
        &test_data,
        5000,
        10000
    );
    bench_ssz_print_stats("Benchmark ssz_deserialize_vector 16384 uint64s", &stats);
}

static void run_list_deserialize_benchmarks(void) {
    ssz_list_deserialize_test_t test_data;
    memset(test_data.buffer, 0, sizeof(test_data.buffer));
    test_data.buffer_size = 8 * 8192;
    test_data.max_element_count = 8192;
    for (size_t i = 0; i < 8192; i++) {
        test_data.field_sizes[i] = 8;
    }
    test_data.is_variable_size = false;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_list_deserialize,
        &test_data,
        5000,
        10000
    );
    bench_ssz_print_stats("Benchmark ssz_deserialize_list 8192 uint64s", &stats);
}

static void run_all_deserialization_benchmarks(void) {
    run_uintN_deserialize_benchmarks();
    run_boolean_deserialize_benchmarks();
    run_bitvector_deserialize_benchmarks();
    run_bitlist_deserialize_benchmarks();
    run_vector_deserialize_benchmarks();
    run_list_deserialize_benchmarks();
}

int main(void) {
    run_all_deserialization_benchmarks();
    return 0;
}