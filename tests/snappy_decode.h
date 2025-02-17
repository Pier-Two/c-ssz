#ifndef SNAPPY_DECODE_H
#define SNAPPY_DECODE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

  /*
   * snappy_status is used as the return type for our functions.
   * SNAPPY_OK indicates success, SNAPPY_INVALID_INPUT indicates a problem
   * with the compressed data, and SNAPPY_BUFFER_TOO_SMALL indicates that
   * the provided output buffer isn't large enough.
   */
  typedef enum
  {
    SNAPPY_OK = 0,
    SNAPPY_INVALID_INPUT = 1,
    SNAPPY_BUFFER_TOO_SMALL = 2
  } snappy_status;

  /*
   * Given a compressed stream produced by snappy_compress(),
   * snappy_uncompressed_length() determines the length of the uncompressed data.
   *
   * REQUIRES: "compressed" must have been produced by snappy_compress().
   * On success, returns SNAPPY_OK and writes the uncompressed length to *result.
   * On error, returns SNAPPY_INVALID_INPUT.
   */
  snappy_status snappy_uncompressed_length(const char *compressed,
                                           size_t compressed_length,
                                           size_t *result);

  /*
   * Given a compressed stream produced by snappy_compress(),
   * snappy_uncompress() writes the uncompressed data to the provided buffer.
   *
   * Before calling this function, *uncompressed_length should indicate the available
   * size in the output buffer. If that size is insufficient, the function returns
   * SNAPPY_BUFFER_TOO_SMALL (and *uncompressed_length is updated to the required size).
   * On successful decompression, *uncompressed_length is set to the decompressed data length.
   */
  snappy_status snappy_uncompress(const char *compressed,
                                  size_t compressed_length,
                                  char *uncompressed,
                                  size_t *uncompressed_length);

#ifdef __cplusplus
}
#endif

#endif /* SNAPPY_DECODE_H */