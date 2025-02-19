#ifndef SNAPPY_DECODE_H
#define SNAPPY_DECODE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

typedef enum
{
  SNAPPY_OK = 0,
  SNAPPY_INVALID_INPUT = 1,
  SNAPPY_BUFFER_TOO_SMALL = 2
} snappy_status;

snappy_status snappy_uncompressed_length(const char *compressed,
                                          size_t compressed_length,
                                          size_t *result);

snappy_status snappy_uncompress(const char *compressed,
                                size_t compressed_length,
                                char *uncompressed,
                                size_t *uncompressed_length);

#ifdef __cplusplus
}
#endif

#endif /* SNAPPY_DECODE_H */