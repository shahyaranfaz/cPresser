#ifndef LZ77_H
#define LZ77_H

#include <stddef.h>

unsigned char *lz77_compress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

unsigned char *lz77_decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

#endif
