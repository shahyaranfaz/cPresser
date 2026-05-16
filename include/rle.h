#ifndef CPRESS_RLE_H
#define CPRESS_RLE_H

#include <stddef.h>

unsigned char *rle_compress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

unsigned char *rle_decompress(const unsigned char *in_buffer, size_t in_size, size_t *out_size);

size_t rle_compressed_size(const unsigned char *in_buffer, size_t in_size);

#endif //CPRESS_RLE_H
