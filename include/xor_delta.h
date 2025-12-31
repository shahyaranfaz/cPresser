#ifndef CPRESS_XOR_DELTA_H
#define CPRESS_XOR_DELTA_H

#include <stddef.h>

unsigned char *delta_compress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

unsigned char *delta_decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

#endif //CPRESS_XOR_DELTA_H