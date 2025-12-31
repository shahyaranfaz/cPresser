#include "xor_delta.h"

#include <stdlib.h>

unsigned char *delta_compress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    unsigned char *out_buffer = malloc(in_size);
    if (out_buffer == NULL) return NULL;

    out_buffer[0] = in_buffer[0];
    for (size_t i = 1; i < in_size; i++)
        out_buffer[i] = in_buffer[i] ^ in_buffer[i-1];

    *out_size = in_size;
    return out_buffer;
}

unsigned char *delta_decompress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    unsigned char *out_buffer = malloc(in_size);
    if (out_buffer == NULL) return NULL;

    out_buffer[0] = in_buffer[0];
    for (size_t i = 1; i < in_size; i++)
        out_buffer[i] = in_buffer[i] ^ out_buffer[i-1];

    *out_size = in_size;
    return out_buffer;
}