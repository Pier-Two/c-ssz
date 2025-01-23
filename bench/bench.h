#ifndef BENCH_H  
#define BENCH_H  

typedef void (*bench_ssz_test_func_t)(void* user_data);  

typedef struct {  
    double total_time_us;  
    double avg_time_us;  
    unsigned long iterations;  
} bench_ssz_stats_t;  

bench_ssz_stats_t bench_ssz_run_benchmark(  
    bench_ssz_test_func_t test_func,  
    void* user_data,  
    unsigned long warmup_iterations,  
    unsigned long measured_iterations  
);  

void bench_ssz_print_stats(const char* label, const bench_ssz_stats_t* stats);  

#endif /* BENCH_H */  