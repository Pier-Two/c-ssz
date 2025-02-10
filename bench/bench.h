#ifndef BENCH_H
#define BENCH_H

typedef void (*bench_test_func_t)(void* user_data);

typedef struct {
    unsigned long iterations;
    double total_time_ns;
    double avg_time_ns;
    double min_time_ns;
    double max_time_ns;
    double variance_ns2;
    double stddev_ns;
} bench_stats_t;

bench_stats_t bench_run_benchmark(
    bench_test_func_t test_func,
    void* user_data,
    unsigned long warmup_iterations,
    unsigned long measured_iterations
);

void bench_print_stats(const char* label, const bench_stats_t* stats);

#endif /* BENCH_H */