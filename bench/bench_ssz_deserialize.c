#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bench.h"
#include "ssz_deserialize.h"

#define BENCH_ITER_WARMUP_UINTN 50000
#define BENCH_ITER_MEASURED_UINTN 100000
#define BENCH_ITER_WARMUP_BOOLEAN 50000
#define BENCH_ITER_MEASURED_BOOLEAN 100000
#define BENCH_ITER_WARMUP_BITVECTOR 5000
#define BENCH_ITER_MEASURED_BITVECTOR 10000
#define BENCH_ITER_WARMUP_BITLIST 5000
#define BENCH_ITER_MEASURED_BITLIST 10000
#define BENCH_ITER_WARMUP_VECTOR 5000
#define BENCH_ITER_MEASURED_VECTOR 10000
#define BENCH_ITER_WARMUP_LIST 5000
#define BENCH_ITER_MEASURED_LIST 10000

typedef struct
{
    size_t bit_size;
    uint8_t buffer[32];
    size_t buffer_size;
} ssz_uintN_deserialize_test_t;

typedef struct
{
    uint8_t buffer[1];
    size_t buffer_size;
} ssz_boolean_deserialize_test_t;

typedef struct
{
    size_t num_bits;
    uint8_t buffer[32768];
    size_t buffer_size;
} ssz_bitvector_deserialize_test_t;

typedef struct
{
    size_t max_bits;
    uint8_t buffer[65537];
    size_t buffer_size;
} ssz_bitlist_deserialize_test_t;

typedef struct
{
    size_t bit_size;
    uint8_t buffer[524288];
    size_t buffer_size;
    size_t element_count;
} ssz_vector_deserialize_test_t;

typedef struct
{
    size_t bit_size;
    uint8_t buffer[524292];
    size_t buffer_size;
    size_t max_length;
} ssz_list_deserialize_test_t;

static void test_uintN_deserialize(void *user_data)
{
    ssz_uintN_deserialize_test_t *test_data = (ssz_uintN_deserialize_test_t *)user_data;
    switch (test_data->bit_size)
    {
    case 8:
    {
        uint8_t out_value = 0;
        ssz_deserialize_uint8(test_data->buffer, test_data->buffer_size, &out_value);
        break;
    }
    case 16:
    {
        uint16_t out_value = 0;
        ssz_deserialize_uint16(test_data->buffer, test_data->buffer_size, &out_value);
        break;
    }
    case 32:
    {
        uint32_t out_value = 0;
        ssz_deserialize_uint32(test_data->buffer, test_data->buffer_size, &out_value);
        break;
    }
    case 64:
    {
        uint64_t out_value = 0;
        ssz_deserialize_uint64(test_data->buffer, test_data->buffer_size, &out_value);
        break;
    }
    case 128:
    {
        uint8_t out_value[16] = {0};
        ssz_deserialize_uint128(test_data->buffer, test_data->buffer_size, out_value);
        break;
    }
    case 256:
    {
        uint8_t out_value[32] = {0};
        ssz_deserialize_uint256(test_data->buffer, test_data->buffer_size, out_value);
        break;
    }
    default:
        break;
    }
}

static void test_boolean_deserialize(void *user_data)
{
    ssz_boolean_deserialize_test_t *test_data = (ssz_boolean_deserialize_test_t *)user_data;
    bool out_value = false;
    ssz_deserialize_boolean(test_data->buffer, test_data->buffer_size, &out_value);
}

static void test_bitvector_deserialize(void *user_data)
{
    ssz_bitvector_deserialize_test_t *test_data = (ssz_bitvector_deserialize_test_t *)user_data;
    bool out_bits[262144];
    ssz_deserialize_bitvector(test_data->buffer, test_data->buffer_size, test_data->num_bits, out_bits);
}

static void test_bitlist_deserialize(void *user_data)
{
    ssz_bitlist_deserialize_test_t *test_data = (ssz_bitlist_deserialize_test_t *)user_data;
    bool out_bits[524288];
    size_t out_actual_bits = 0;
    ssz_deserialize_bitlist(test_data->buffer, test_data->buffer_size, test_data->max_bits, out_bits, &out_actual_bits);
}

static void test_vector_deserialize(void *user_data)
{
    ssz_vector_deserialize_test_t *test_data = (ssz_vector_deserialize_test_t *)user_data;
    switch (test_data->bit_size)
    {
    case 1:
    {
        bool out_elements[16384];
        ssz_deserialize_vector_bool(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 8:
    {
        uint8_t out_elements[16384];
        ssz_deserialize_vector_uint8(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 16:
    {
        uint16_t out_elements[16384];
        ssz_deserialize_vector_uint16(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 32:
    {
        uint32_t out_elements[16384];
        ssz_deserialize_vector_uint32(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 64:
    {
        uint64_t out_elements[16384];
        ssz_deserialize_vector_uint64(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 128:
    {
        uint8_t out_elements[16 * 16384];
        ssz_deserialize_vector_uint128(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    case 256:
    {
        uint8_t out_elements[32 * 16384];
        ssz_deserialize_vector_uint256(test_data->buffer, test_data->buffer_size, test_data->element_count, out_elements);
        break;
    }
    default:
        break;
    }
}

static void test_list_deserialize(void *user_data)
{
    ssz_list_deserialize_test_t *test_data = (ssz_list_deserialize_test_t *)user_data;
    switch (test_data->bit_size)
    {
    case 1:
    {
        bool out_elements[16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_bool(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 8:
    {
        uint8_t out_elements[16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint8(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 16:
    {
        uint16_t out_elements[16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint16(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 32:
    {
        uint32_t out_elements[16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint32(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 64:
    {
        uint64_t out_elements[16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint64(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 128:
    {
        uint8_t out_elements[16 * 16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint128(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    case 256:
    {
        uint8_t out_elements[32 * 16384];
        size_t out_actual_count = 0;
        ssz_deserialize_list_uint256(test_data->buffer, test_data->buffer_size, test_data->max_length, out_elements, &out_actual_count);
        break;
    }
    default:
        break;
    }
}

static void run_uintN_deserialize_benchmarks(void)
{
    ssz_uintN_deserialize_test_t tests[] = {
        {8, {0}, 1},
        {16, {0}, 2},
        {32, {0}, 4},
        {64, {0}, 8},
        {128, {0}, 16},
        {256, {0}, 32}};
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++)
    {
        memset(tests[i].buffer, 0xFF, tests[i].buffer_size);
        bench_stats_t stats = bench_run_benchmark(test_uintN_deserialize, &tests[i], BENCH_ITER_WARMUP_UINTN, BENCH_ITER_MEASURED_UINTN);
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_deserialize_uint%zu", tests[i].bit_size);
        bench_print_stats(label, &stats);
    }
}

static void run_boolean_deserialize_benchmarks(void)
{
    ssz_boolean_deserialize_test_t tests[] = {
        {{0x00}, 1},
        {{0x01}, 1}};
    for (int i = 0; i < 2; i++)
    {
        bench_stats_t stats = bench_run_benchmark(test_boolean_deserialize, &tests[i], BENCH_ITER_WARMUP_BOOLEAN, BENCH_ITER_MEASURED_BOOLEAN);
        char label[64];
        snprintf(label, sizeof(label), "Benchmark ssz_deserialize_boolean %s", tests[i].buffer[0] ? "true" : "false");
        bench_print_stats(label, &stats);
    }
}

static void run_bitvector_deserialize_benchmarks(void)
{
    ssz_bitvector_deserialize_test_t test_data;
    test_data.num_bits = 262144;
    test_data.buffer_size = 262144 / 8;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    bench_stats_t stats = bench_run_benchmark(test_bitvector_deserialize, &test_data, BENCH_ITER_WARMUP_BITVECTOR, BENCH_ITER_MEASURED_BITVECTOR);
    bench_print_stats("Benchmark ssz_deserialize_bitvector", &stats);
}

static void run_bitlist_deserialize_benchmarks(void)
{
    ssz_bitlist_deserialize_test_t test_data;
    test_data.max_bits = 524288;
    test_data.buffer_size = (524288 / 8) + 1;
    memset(test_data.buffer, 0xFF, test_data.buffer_size);
    test_data.buffer[test_data.buffer_size - 1] = 0x01;
    bench_stats_t stats = bench_run_benchmark(test_bitlist_deserialize, &test_data, BENCH_ITER_WARMUP_BITLIST, BENCH_ITER_MEASURED_BITLIST);
    bench_print_stats("Benchmark ssz_deserialize_bitlist", &stats);
}

static void run_vector_deserialize_benchmarks(void)
{
    ssz_vector_deserialize_test_t tests[] = {
        {1, {0}, 0, 16384},
        {8, {0}, 0, 16384},
        {16, {0}, 0, 16384},
        {32, {0}, 0, 16384},
        {64, {0}, 0, 16384},
        {128, {0}, 0, 16384},
        {256, {0}, 0, 16384}};
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++)
    {
        if (tests[i].bit_size == 1)
        {
            tests[i].buffer_size = (tests[i].element_count + 7) / 8;
        }
        else
        {
            tests[i].buffer_size = (tests[i].bit_size / 8) * tests[i].element_count;
        }
        memset(tests[i].buffer, 0xFF, tests[i].buffer_size);
        bench_stats_t stats = bench_run_benchmark(test_vector_deserialize, &tests[i], BENCH_ITER_WARMUP_VECTOR, BENCH_ITER_MEASURED_VECTOR);
        char label[64];
        if (tests[i].bit_size == 1)
        {
            snprintf(label, sizeof(label), "Benchmark ssz_deserialize_vector_bool");
        }
        else
        {
            snprintf(label, sizeof(label), "Benchmark ssz_deserialize_vector_uint%zu", tests[i].bit_size);
        }
        bench_print_stats(label, &stats);
    }
}

static void run_list_deserialize_benchmarks(void)
{
    ssz_list_deserialize_test_t tests[] = {
        {1, {0}, 0, 16384},
        {8, {0}, 0, 16384},
        {16, {0}, 0, 16384},
        {32, {0}, 0, 16384},
        {64, {0}, 0, 16384},
        {128, {0}, 0, 16384},
        {256, {0}, 0, 16384}};
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++)
    {
        size_t element_size = 0;
        if (tests[i].bit_size == 1)
        {
            element_size = (tests[i].max_length + 7) / 8;
        }
        else
        {
            element_size = (tests[i].bit_size / 8) * tests[i].max_length;
        }
        tests[i].buffer_size = 4 + element_size;
        memset(tests[i].buffer, 0xFF, tests[i].buffer_size);
        uint32_t data_size = (uint32_t)element_size;
        tests[i].buffer[0] = (uint8_t)(data_size & 0xFF);
        tests[i].buffer[1] = (uint8_t)((data_size >> 8) & 0xFF);
        tests[i].buffer[2] = (uint8_t)((data_size >> 16) & 0xFF);
        tests[i].buffer[3] = (uint8_t)((data_size >> 24) & 0xFF);
        bench_stats_t stats = bench_run_benchmark(test_list_deserialize, &tests[i], BENCH_ITER_WARMUP_LIST, BENCH_ITER_MEASURED_LIST);
        char label[64];
        if (tests[i].bit_size == 1)
        {
            snprintf(label, sizeof(label), "Benchmark ssz_deserialize_list_bool");
        }
        else
        {
            snprintf(label, sizeof(label), "Benchmark ssz_deserialize_list_uint%zu", tests[i].bit_size);
        }
        bench_print_stats(label, &stats);
    }
}

static void run_all_deserialization_benchmarks(void)
{
    run_uintN_deserialize_benchmarks();
    run_boolean_deserialize_benchmarks();
    run_bitvector_deserialize_benchmarks();
    run_bitlist_deserialize_benchmarks();
    run_vector_deserialize_benchmarks();
    run_list_deserialize_benchmarks();
}

int main(void)
{
    run_all_deserialization_benchmarks();
    return 0;
}