#include "benchmark.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
static double bench_ssz_get_time_in_microseconds(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000000.0 / (double)frequency.QuadPart;
}
#else
#include <sys/time.h>
static double bench_ssz_get_time_in_microseconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}
#endif

#include <stdio.h>

bench_ssz_stats_t bench_ssz_run_benchmark(
    bench_ssz_test_func_t test_func,
    void *user_data,
    unsigned long warmup_iterations,
    unsigned long measured_iterations)
{
    for (unsigned long i = 0; i < warmup_iterations; i++)
    {
        test_func(user_data);
    }

    double start_time = bench_ssz_get_time_in_microseconds();

    for (unsigned long i = 0; i < measured_iterations; i++)
    {
        test_func(user_data);
    }

    double end_time = bench_ssz_get_time_in_microseconds();

    bench_ssz_stats_t stats;
    stats.total_time_us = end_time - start_time;
    stats.iterations = measured_iterations;
    stats.avg_time_us = stats.total_time_us / (double)measured_iterations;
    return stats;
}

void bench_ssz_print_stats(const char *label, const bench_ssz_stats_t *stats)
{
    if (!label || !stats)
    {
        return;
    }

    printf("\nBenchmark: %s\n", label);
    printf("Iterations: %lu\n", stats->iterations);
    printf("Total time: %.3f microseconds\n", stats->total_time_us);
    printf("Average:    %.3f microseconds\n", stats->avg_time_us);
}