#ifndef CPRESS_HUFFMAN_H
#define CPRESS_HUFFMAN_H

#include <stddef.h>

unsigned char *huffman_compress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

unsigned char *huffman_decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

#endif
