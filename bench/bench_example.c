/**
 * @file bench_example.c
 * @brief Minimal ubench.h placeholder — verifies the benchmark infrastructure compiles and runs.
 *
 * Replace this file with real benchmarks once the library code exists.
 * Each bench/bench_*.c file is auto-discovered by CMake and built as its own executable.
 */

#include "ubench.h"
#include <string.h>

UBENCH(example, memset_4k) {
    char buf[4096];
    memset(buf, 0xAA, sizeof(buf));
    /* Prevent the compiler from optimising the memset away. */
    ubench_do_nothing(buf);
}

UBENCH_MAIN();
