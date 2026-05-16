#include "rle.h"
#include "bit_buffer.h"

#include <stdint.h>
#include <stdlib.h>
#define MAX_RUN 255

size_t rle_compressed_size(const unsigned char *in_buffer, const size_t in_size) {
    size_t read_index = 0;
    size_t bit_count = 0;

    while (read_index < in_size) {
        const uint8_t match = in_buffer[read_index];
        size_t j = read_index + 1;
        while (j < in_size && match == in_buffer[j] && j - read_index <= MAX_RUN) {
            j++;
        }
        bit_count += (j - read_index == 1) ? 9 : 17;
        read_index = j;
    }

    return (bit_count + 7) / 8;
}

unsigned char *rle_compress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    unsigned char *out_buffer = malloc((in_size * 9 + 7) / 8);
    if (!out_buffer) return NULL;
    size_t read_index = 0;

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, out_buffer, 0);

    while (read_index < in_size) {
        const uint8_t match = in_buffer[read_index];
        size_t j = read_index + 1;
        while (j < in_size && match == in_buffer[j] && j - read_index <= MAX_RUN) {
            j++;
        }
        if (j-read_index == 1) {
            WRITE_BITS(&bit_buffer, 0, 1);
            WRITE_BITS(&bit_buffer, match, 8);
        } else {
            WRITE_BITS(&bit_buffer, 1, 1);
            WRITE_BITS(&bit_buffer, match, 8);
            WRITE_BITS(&bit_buffer, j - read_index, 8);
        }
        read_index = j;
    }
    FLUSH_BITS(&bit_buffer);
    *out_size = bit_buffer.buff_index;
    unsigned char* tmp = realloc(out_buffer, *out_size);
    if (tmp) out_buffer = tmp;
    return out_buffer;
}

unsigned char *rle_decompress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    size_t curr_size = in_size * 2;
    unsigned char *out_buffer = malloc(curr_size);
    if (!out_buffer) return NULL;

    size_t write_index = 0;
    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, (unsigned char*)in_buffer, 0);

    while (bit_buffer.buff_index < in_size) {
        if (curr_size <= write_index + MAX_RUN) {
            while (write_index + MAX_RUN >= curr_size) curr_size *= 2;
            unsigned char* tmp = realloc(out_buffer, curr_size);
            if (!tmp) { free(out_buffer); return NULL; }
            out_buffer = tmp;
        }
        int flag;
        uint8_t match;
        READ_BITS(&bit_buffer, flag, 1);
        READ_BITS(&bit_buffer, match, 8);
        if (flag == 0) {
            out_buffer[write_index++] = match;
        } else {
            uint8_t length;
            READ_BITS(&bit_buffer, length, 8);
            for (uint8_t i = 0; i < length; i++)
                out_buffer[write_index++] = match;
        }
    }
    *out_size = write_index;
    return out_buffer;
}
