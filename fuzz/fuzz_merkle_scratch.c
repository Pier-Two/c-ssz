#include <stddef.h>
#include <stdint.h>

#include "ssz.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ssz_chunk_t chunks[128] = {{{0u}}};
    ssz_chunk_t scratch_chunks[64] = {{{0u}}};
    ssz_chunk_t out_root;
    ssz_merkle_scratch_t valid_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 64u,
    };
    ssz_merkle_scratch_t tiny_scratch = {
        .chunks = scratch_chunks,
        .chunk_count = 1u,
    };
    ssz_merkle_scratch_t invalid_scratch = {
        .chunks = NULL,
        .chunk_count = 1u,
    };

    if (data != NULL)
    {
        size_t fill = size;
        size_t cap = sizeof(chunks);
        if (fill > cap)
        {
            fill = cap;
        }
        for (size_t i = 0u; i < fill; i++)
        {
            ((uint8_t *)chunks)[i] = data[i];
        }
    }

    (void)ssz_merkleize(chunks, 128u, 128u, &valid_scratch, NULL, &out_root);
    (void)ssz_merkleize(chunks, 128u, 128u, &tiny_scratch, NULL, &out_root);
    (void)ssz_merkleize(chunks, 128u, 128u, &invalid_scratch, NULL, &out_root);

    return 0;
}
