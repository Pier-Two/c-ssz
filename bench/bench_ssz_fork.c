#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ssz_serialize.h"
#include "ssz_deserialize.h"
#include "ssz_constants.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
static double get_time_in_microseconds(void) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000000.0 / (double)frequency.QuadPart;
}
#else
#include <sys/time.h>
static double get_time_in_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}
#endif

static void run_subtest(void (*test_func)(void), const char* name, int iterations)
{
    double start = get_time_in_microseconds();
    for (int i = 0; i < iterations; i++) {
        test_func();
    }
    double end = get_time_in_microseconds();
    double elapsed = end - start;
    double avg = elapsed / iterations;
    printf("\nBenchmark for %s: total = %.2f microseconds, avg = %.2f microseconds\n", name, elapsed, avg);
}

typedef struct
{
    uint8_t previous_version[4];
    uint8_t current_version[4];
    uint64_t epoch;
} Fork;

static ssz_error_t serialize_fork(
    const Fork *fork_data,
    uint8_t *out_buffer,
    size_t out_buffer_size,
    size_t *out_actual_size)
{
    if (!fork_data || !out_buffer || !out_actual_size)
    {
        return SSZ_ERROR_SERIALIZATION;
    }

    uint8_t temp[16];
    memcpy(temp, fork_data->previous_version, 4);
    memcpy(temp + 4, fork_data->current_version, 4);

    uint64_t epoch_le = fork_data->epoch;
    temp[8] = (uint8_t)(epoch_le >> 0);
    temp[9] = (uint8_t)(epoch_le >> 8);
    temp[10] = (uint8_t)(epoch_le >> 16);
    temp[11] = (uint8_t)(epoch_le >> 24);
    temp[12] = (uint8_t)(epoch_le >> 32);
    temp[13] = (uint8_t)(epoch_le >> 40);
    temp[14] = (uint8_t)(epoch_le >> 48);
    temp[15] = (uint8_t)(epoch_le >> 56);

    size_t element_count = 1;
    size_t element_sizes[1] = {16};
    size_t written_size = out_buffer_size;
    ssz_error_t err = ssz_serialize_vector(
        temp,
        element_count,
        element_sizes,
        false,
        out_buffer,
        &written_size);

    if (err == SSZ_SUCCESS)
    {
        if (written_size > out_buffer_size)
        {
            return SSZ_ERROR_SERIALIZATION;
        }
        *out_actual_size = written_size;
    }

    return err;
}

static ssz_error_t deserialize_fork(
    const uint8_t *buffer,
    size_t buffer_size,
    Fork *out_fork)
{
    if (!buffer || !out_fork)
    {
        return SSZ_ERROR_DESERIALIZATION;
    }

    size_t field_sizes[3] = {4, 4, 8};
    uint8_t temp[4 + 4 + 8];

    ssz_error_t ret = ssz_deserialize_vector(
        buffer,
        buffer_size,
        3,
        field_sizes,
        false,
        temp);

    if (ret != SSZ_SUCCESS)
    {
        return ret;
    }

    memcpy(out_fork->previous_version, temp, 4);
    memcpy(out_fork->current_version, temp + 4, 4);

    uint64_t epoch_le = 0;
    epoch_le |= ((uint64_t)temp[8]) << 0;
    epoch_le |= ((uint64_t)temp[9]) << 8;
    epoch_le |= ((uint64_t)temp[10]) << 16;
    epoch_le |= ((uint64_t)temp[11]) << 24;
    epoch_le |= ((uint64_t)temp[12]) << 32;
    epoch_le |= ((uint64_t)temp[13]) << 40;
    epoch_le |= ((uint64_t)temp[14]) << 48;
    epoch_le |= ((uint64_t)temp[15]) << 56;
    out_fork->epoch = epoch_le;

    return SSZ_SUCCESS;
}

static void test_fork_subtest(void)
{
    Fork original = {
        {0x03, 0x00, 0x00, 0x00},
        {0x04, 0x00, 0x00, 0x00},
        269568};

    uint8_t serialized[32];
    memset(serialized, 0, sizeof(serialized));
    size_t serialized_size = 0;

    ssz_error_t err = serialize_fork(&original, serialized, sizeof(serialized), &serialized_size);
    if (err != SSZ_SUCCESS)
    {
        return;
    }

    Fork decoded;
    memset(&decoded, 0, sizeof(decoded));

    err = deserialize_fork(serialized, serialized_size, &decoded);
    if (err != SSZ_SUCCESS)
    {
        return;
    }
}

int main(void)
{
    int iterations = 100000;
    run_subtest(test_fork_subtest, "fork_subtest", iterations);
    return 0;
}