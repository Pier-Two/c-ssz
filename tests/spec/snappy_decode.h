#ifndef TESTS_SPEC_SNAPPY_DECODE_H
#define TESTS_SPEC_SNAPPY_DECODE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    SNAPPY_OK = 0,
    SNAPPY_INVALID_INPUT = 1,
    SNAPPY_BUFFER_TOO_SMALL = 2,
} snappy_status;

snappy_status snappy_uncompressed_length(
    const char *compressed,
    size_t compressed_length,
    size_t *result);

snappy_status snappy_uncompress(
    const char *compressed,
    size_t compressed_length,
    char *uncompressed,
    size_t *uncompressed_length);

#ifdef __cplusplus
}
#endif

#endif
