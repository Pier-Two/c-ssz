#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "benchmark.h"
#include "ssz_serialize.h"

typedef struct
{
    size_t bit_size;
    uint8_t value[32];
} ssz_uintN_test_t;

typedef struct
{
    bool bits[262144];
    size_t num_bits;
} ssz_bitvector_test_t;

typedef struct
{
    bool bits[524288];
    size_t num_bits;
} ssz_bitlist_test_t;

typedef struct
{
    uint64_t items[16384];
    size_t element_count;
} ssz_vector_test_t;

typedef struct
{
    uint64_t items[8192];
    size_t element_count;
} ssz_list_test_t;

static void test_uintN_serialize(void *user_data)
{
    ssz_uintN_test_t *test_data = (ssz_uintN_test_t *)user_data;
    uint8_t out_buf[32];
    size_t out_size = sizeof(out_buf);
    ssz_serialize_uintN(test_data->value, test_data->bit_size, out_buf, &out_size);
}

static void test_boolean_serialize(void *user_data)
{
    bool value = *(bool *)user_data;
    uint8_t out_buf[1];
    size_t out_size = sizeof(out_buf);
    ssz_serialize_boolean(value, out_buf, &out_size);
}

static void test_bitvector_serialize(void *user_data)
{
    ssz_bitvector_test_t *test_data = (ssz_bitvector_test_t *)user_data;
    uint8_t out_buf[32768];
    size_t out_size = sizeof(out_buf);
    ssz_serialize_bitvector(test_data->bits, test_data->num_bits, out_buf, &out_size);
}

static void test_bitlist_serialize(void *user_data)
{
    ssz_bitlist_test_t *test_data = (ssz_bitlist_test_t *)user_data;
    uint8_t out_buf[65537];
    size_t out_size = sizeof(out_buf);
    ssz_serialize_bitlist(test_data->bits, test_data->num_bits, out_buf, &out_size);
}

static void test_vector_serialize(void *user_data)
{
    ssz_vector_test_t *test_data = (ssz_vector_test_t *)user_data;
    uint8_t out_buf[131072];
    size_t out_size = sizeof(out_buf);
    size_t element_sizes[16384];
    for (size_t i = 0; i < test_data->element_count; i++)
    {
        element_sizes[i] = 8;
    }
    ssz_serialize_vector(test_data->items, test_data->element_count, element_sizes, false, out_buf, &out_size);
}

static void test_list_serialize(void *user_data)
{
    ssz_list_test_t *test_data = (ssz_list_test_t *)user_data;
    uint8_t out_buf[65536];
    size_t out_size = sizeof(out_buf);
    size_t element_sizes[8192];
    for (size_t i = 0; i < test_data->element_count; i++)
    {
        element_sizes[i] = 8;
    }
    ssz_serialize_list(test_data->items, test_data->element_count, element_sizes, false, out_buf, &out_size);
}

static void run_uintN_benchmarks(void)
{
    ssz_uintN_test_t tests[] = {
        {8, {0}},
        {16, {0}},
        {32, {0}},
        {64, {0}},
        {128, {0}},
        {256, {0}}};
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++)
    {
        size_t byte_size = tests[i].bit_size / 8;
        memset(tests[i].value, 0xFF, byte_size);
        bench_ssz_stats_t stats = bench_ssz_run_benchmark(
            test_uintN_serialize,
            &tests[i],
            100000,
            1000000);
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_serialize_uint%zu", tests[i].bit_size);
        bench_ssz_print_stats(label, &stats);
    }
}

static void run_boolean_benchmarks(void)
{
    bool booleans[] = {false, true};
    for (int i = 0; i < 2; i++)
    {
        bench_ssz_stats_t stats = bench_ssz_run_benchmark(
            test_boolean_serialize,
            &booleans[i],
            100000,
            1000000);
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_serialize_boolean %s", booleans[i] ? "true" : "false");
        bench_ssz_print_stats(label, &stats);
    }
}

static void run_bitvector_benchmarks(void)
{
    ssz_bitvector_test_t test_data;
    test_data.num_bits = 262144;
    for (size_t i = 0; i < test_data.num_bits; i++)
    {
        test_data.bits[i] = true;
    }
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_bitvector_serialize,
        &test_data,
        100,
        100);
    bench_ssz_print_stats("Benchmark ssz_serialize_bitvector 262144 bits", &stats);
}

static void run_bitlist_benchmarks(void)
{
    ssz_bitlist_test_t test_data;
    test_data.num_bits = 524288;
    for (size_t i = 0; i < test_data.num_bits; i++)
    {
        test_data.bits[i] = true;
    }
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_bitlist_serialize,
        &test_data,
        10,
        10);
    bench_ssz_print_stats("Benchmark ssz_serialize_bitlist 524288 bits", &stats);
}

static void run_vector_benchmarks(void)
{
    ssz_vector_test_t test_data;
    test_data.element_count = 16384;
    for (size_t i = 0; i < test_data.element_count; i++)
    {
        test_data.items[i] = 0xFFFFFFFFFFFFFFFFULL;
    }
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_vector_serialize,
        &test_data,
        5000,
        10000);
    bench_ssz_print_stats("Benchmark ssz_serialize_vector 16384 uint64s", &stats);
}

static void run_list_benchmarks(void)
{
    ssz_list_test_t test_data;
    test_data.element_count = 8192;
    for (size_t i = 0; i < test_data.element_count; i++)
    {
        test_data.items[i] = 0xFFFFFFFFFFFFFFFFULL;
    }
    bench_ssz_stats_t stats = bench_ssz_run_benchmark(
        test_list_serialize,
        &test_data,
        5000,
        10000);
    bench_ssz_print_stats("Benchmark ssz_serialize_list 8192 uint64s", &stats);
}

static void run_all_benchmarks(void)
{
    run_uintN_benchmarks();
    run_boolean_benchmarks();
    run_bitvector_benchmarks();
    run_bitlist_benchmarks();
    run_vector_benchmarks();
    run_list_benchmarks();
}

int main(void)
{
    run_all_benchmarks();
    return 0;
}