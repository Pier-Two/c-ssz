#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "bench.h"
#include "ssz_constants.h"
#include "ssz_merkle.h"

#define BENCH_ITER_WARMUP_MERKLEIZE 5000
#define BENCH_ITER_MEASURED_MERKLEIZE 10000
#define BENCH_ITER_WARMUP_PACK 5000
#define BENCH_ITER_MEASURED_PACK 10000
#define BENCH_ITER_WARMUP_PACK_BITS 5000
#define BENCH_ITER_MEASURED_PACK_BITS 10000
#define BENCH_ITER_WARMUP_MIX_LENGTH 5000
#define BENCH_ITER_MEASURED_MIX_LENGTH 10000
#define BENCH_ITER_WARMUP_MIX_SELECTOR 5000
#define BENCH_ITER_MEASURED_MIX_SELECTOR 10000

typedef struct {
    uint8_t chunks[64 * SSZ_BYTES_PER_CHUNK];
    size_t chunk_count;
    size_t limit;
} ssz_merkleize_test_t;

typedef struct {
    uint8_t values[1024];
    size_t value_size;
    size_t value_count;
} ssz_pack_test_t;

typedef struct {
    bool bits[1024];
    size_t bit_count;
} ssz_pack_bits_test_t;

typedef struct {
    uint8_t root[SSZ_BYTES_PER_CHUNK];
    uint64_t length;
} ssz_mix_length_test_t;

typedef struct {
    uint8_t root[SSZ_BYTES_PER_CHUNK];
    uint8_t selector;
} ssz_mix_selector_test_t;

static void test_merkleize(void *user_data) {
    ssz_merkleize_test_t *test = (ssz_merkleize_test_t *)user_data;
    uint8_t out_root[SSZ_BYTES_PER_CHUNK];
    ssz_merkleize(test->chunks, test->chunk_count, test->limit, out_root);
}

static void test_pack(void *user_data) {
    ssz_pack_test_t *test = (ssz_pack_test_t *)user_data;
    uint8_t out_chunks[2048];
    size_t out_chunk_count = 0;
    ssz_pack(test->values, test->value_size, test->value_count, out_chunks, &out_chunk_count);
}

static void test_pack_bits(void *user_data) {
    ssz_pack_bits_test_t *test = (ssz_pack_bits_test_t *)user_data;
    uint8_t out_chunks[2048];
    size_t out_chunk_count = 0;
    ssz_pack_bits(test->bits, test->bit_count, out_chunks, &out_chunk_count);
}

static void test_mix_in_length(void *user_data) {
    ssz_mix_length_test_t *test = (ssz_mix_length_test_t *)user_data;
    uint8_t out_root[SSZ_BYTES_PER_CHUNK];
    ssz_mix_in_length(test->root, test->length, out_root);
}

static void test_mix_in_selector(void *user_data) {
    ssz_mix_selector_test_t *test = (ssz_mix_selector_test_t *)user_data;
    uint8_t out_root[SSZ_BYTES_PER_CHUNK];
    ssz_mix_in_selector(test->root, test->selector, out_root);
}

static void run_merkleize_benchmarks(void) {
    ssz_merkleize_test_t test_data;
    memset(test_data.chunks, 0xAA, sizeof(test_data.chunks));
    test_data.chunk_count = 64;
    test_data.limit = 64;
    bench_stats_t stats = bench_run_benchmark(test_merkleize, &test_data, BENCH_ITER_WARMUP_MERKLEIZE, BENCH_ITER_MEASURED_MERKLEIZE);
    bench_print_stats("Benchmark ssz_merkleize", &stats);
}

static void run_pack_benchmarks(void) {
    ssz_pack_test_t test_data;
    memset(test_data.values, 0x55, sizeof(test_data.values));
    test_data.value_size = 16; 
    test_data.value_count = 64;
    bench_stats_t stats = bench_run_benchmark(test_pack, &test_data, BENCH_ITER_WARMUP_PACK, BENCH_ITER_MEASURED_PACK);
    bench_print_stats("Benchmark ssz_pack", &stats);
}

static void run_pack_bits_benchmarks(void) {
    ssz_pack_bits_test_t test_data;
    test_data.bit_count = 1024;
    for (size_t i = 0; i < test_data.bit_count; i++) {
        test_data.bits[i] = (i % 2 == 0);
    }
    bench_stats_t stats = bench_run_benchmark(test_pack_bits, &test_data, BENCH_ITER_WARMUP_PACK_BITS, BENCH_ITER_MEASURED_PACK_BITS);
    bench_print_stats("Benchmark ssz_pack_bits", &stats);
}

static void run_mix_in_length_benchmarks(void) {
    ssz_mix_length_test_t test_data;
    memset(test_data.root, 0xBB, sizeof(test_data.root));
    test_data.length = 123456789ULL;
    bench_stats_t stats = bench_run_benchmark(test_mix_in_length, &test_data, BENCH_ITER_WARMUP_MIX_LENGTH, BENCH_ITER_MEASURED_MIX_LENGTH);
    bench_print_stats("Benchmark ssz_mix_in_length", &stats);
}

static void run_mix_in_selector_benchmarks(void) {
    ssz_mix_selector_test_t test_data;
    memset(test_data.root, 0xCC, sizeof(test_data.root));
    test_data.selector = 0x42;
    bench_stats_t stats = bench_run_benchmark(test_mix_in_selector, &test_data, BENCH_ITER_WARMUP_MIX_SELECTOR, BENCH_ITER_MEASURED_MIX_SELECTOR);
    bench_print_stats("Benchmark ssz_mix_in_selector", &stats);
}

static void run_all_benchmarks(void) {
    run_merkleize_benchmarks();
    run_pack_benchmarks();
    run_pack_bits_benchmarks();
    run_mix_in_length_benchmarks();
    run_mix_in_selector_benchmarks();
}

int main(void) {
    run_all_benchmarks();
    return 0;
}
