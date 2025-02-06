#include "bench.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>

static double bench_ssz_get_time_in_nanoseconds(void) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1e9 / (double)frequency.QuadPart;
}

#elif defined(__APPLE__)
#include <mach/mach_time.h>

static double bench_ssz_get_time_in_nanoseconds(void) {
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t time = mach_absolute_time();
    return (double)(time * timebase.numer) / (double)timebase.denom;
}

#else
#include <time.h>

static double bench_ssz_get_time_in_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

#endif

bench_ssz_stats_t bench_ssz_run_benchmark(
    bench_ssz_test_func_t test_func,
    void *user_data,
    unsigned long warmup_iterations,
    unsigned long measured_iterations) {
    
    bench_ssz_stats_t stats = {0};

    for (unsigned long i = 0; i < warmup_iterations; i++) {
        test_func(user_data);
    }

    if (measured_iterations == 0) {
        return stats;
    }

    double *durations = (double *)malloc(measured_iterations * sizeof(double));
    if (!durations) {
        return stats;
    }

    for (unsigned long i = 0; i < measured_iterations; i++) {
        double start = bench_ssz_get_time_in_nanoseconds();
        test_func(user_data);
        double end = bench_ssz_get_time_in_nanoseconds();
        durations[i] = end - start;
    }

    stats.iterations = measured_iterations;
    stats.total_time_ns = 0;
    stats.min_time_ns = durations[0];
    stats.max_time_ns = durations[0];

    for (unsigned long i = 0; i < measured_iterations; i++) {
        stats.total_time_ns += durations[i];
        if (durations[i] < stats.min_time_ns) stats.min_time_ns = durations[i];
        if (durations[i] > stats.max_time_ns) stats.max_time_ns = durations[i];
    }
    stats.avg_time_ns = stats.total_time_ns / measured_iterations;

    double sum_sq_diff = 0.0;
    for (unsigned long i = 0; i < measured_iterations; i++) {
        double diff = durations[i] - stats.avg_time_ns;
        sum_sq_diff += diff * diff;
    }
    if (measured_iterations > 1) {
        stats.variance_ns2 = sum_sq_diff / (measured_iterations - 1);
        stats.stddev_ns = sqrt(stats.variance_ns2);
    } else {
        stats.variance_ns2 = 0.0;
        stats.stddev_ns = 0.0;
    }

    free(durations);
    return stats;
}

void bench_ssz_print_stats(const char* label, const bench_ssz_stats_t* stats) {
    if (!label || !stats) return;

    printf("\nBenchmark: %s\n", label);
    printf("Iterations: %lu\n", stats->iterations);
    printf("Total time: %.3f ns\n", stats->total_time_ns);
    printf("Average:    %.3f ns\n", stats->avg_time_ns);
    printf("Min:        %.3f ns\n", stats->min_time_ns);
    printf("Max:        %.3f ns\n", stats->max_time_ns);
    printf("Std Dev:    %.3f ns\n", stats->stddev_ns);
}