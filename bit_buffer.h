#ifndef CPRESS_BIT_BUFFER_H
#define CPRESS_BIT_BUFFER_H

#include <stdint.h>

typedef struct {
    uint64_t accumulator;
    int count;
    unsigned char* buffer;
    uint64_t buff_index;
} BitBuffer;

static inline void init_bit_buffer(BitBuffer* bit_buffer, unsigned char* buffer, const uint64_t buff_index) {
    bit_buffer->accumulator = 0;
    bit_buffer->count = 0;
    bit_buffer->buffer = buffer;
    bit_buffer->buff_index = buff_index;
}

#define WRITE_BITS(bb, val, nbits) do { \
    (bb)->accumulator |= (uint64_t)(val) << (64 - (nbits) - (bb)->count); \
    (bb)->count += (nbits); \
    while ((bb)->count >= 8) { \
        (bb)->buffer[(bb)->buff_index++] = (bb)->accumulator >> 56; \
        (bb)->accumulator <<= 8; \
        (bb)->count -= 8; \
    } \
} while(0)

#define READ_BITS(bit_buffer, result, nbits) \
    do { \
        int _n = (nbits); \
        while ((bit_buffer)->count < _n) { \
            (bit_buffer)->accumulator = ((bit_buffer)->accumulator << 8) | (bit_buffer)->buffer[(bit_buffer)->buff_index++]; \
            (bit_buffer)->count += 8; \
        } \
        int _shift = (bit_buffer)->count - _n; \
        (result) = ((bit_buffer)->accumulator >> _shift) & ((1ULL << _n) - 1); \
        (bit_buffer)->count = _shift; \
        if ((bit_buffer)->count > 0) \
            (bit_buffer)->accumulator &= (1ULL << (bit_buffer)->count) - 1; \
        else \
            (bit_buffer)->accumulator = 0; \
    } while (0)

#define FLUSH_BITS(bit_buffer) \
    if ((bit_buffer)->count > 0) { \
        (bit_buffer)->buffer[(bit_buffer)->buff_index++] = (unsigned char)((bit_buffer)->accumulator << (8 - (bit_buffer)->count)); \
        (bit_buffer)->count = 0; \
        (bit_buffer)->accumulator = 0; \
    }

#endif //CPRESS_BIT_BUFFER_H