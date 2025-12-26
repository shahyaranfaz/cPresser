#include "lz77.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SEARCH_BUFFER 32768
#define LOOKAHEAD_BUFFER 256

#define HASH_BITS   16
#define HASH_SIZE   (1 << HASH_BITS)
#define HASH_MASK   (HASH_SIZE - 1)
#define HASH_SHIFT  5
#define MIN_MATCH   3
#define MAX_MATCH   255
#define MAX_CHAIN   32

typedef struct {
    uint64_t accumulator;
    int count;
    unsigned char* buffer;
    uint64_t buff_index;
} BitBuffer;

static int32_t head[HASH_SIZE];
static int32_t prev[SEARCH_BUFFER];

#define INIT_HASH() \
    do { \
        memset(head, -1, sizeof(head)); \
        memset(prev, -1, sizeof(prev)); \
    } while(0)

#define UPDATE_HASH(hash, buffer, pos) do { \
    uint32_t _val = *(uint32_t*)(buffer + pos); \
    hash = (_val * 0x1E35A7BD) >> (32 - HASH_BITS); \
} while(0)

#define INSERT_HASH(hash, pos) (head[hash] = (int32_t)(pos))

#define MATCH_HASH(hash, pos, buffer, lookahead, best_len, best_pos) \
do { \
    int32_t _match_pos = head[hash]; \
    *best_len = 0; \
    if (_match_pos >= 0 && (pos - _match_pos) < SEARCH_BUFFER) { \
        if (*(uint32_t*)(buffer + _match_pos) == *(uint32_t*)(buffer + pos)) { \
            int _len = 4; \
            int _max = (lookahead < MAX_MATCH) ? (int)lookahead : MAX_MATCH; \
            while (_len < _max && buffer[pos + _len] == buffer[_match_pos + _len]) \
                _len++; \
            *best_len = _len; \
            *best_pos = _match_pos; \
        } \
    } \
} while(0)

void init_bit_buffer(BitBuffer* bit_buffer, unsigned char* buffer, const uint64_t buff_index) {
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

unsigned char *compress(const unsigned char *in_buffer, const uint64_t in_size, uint64_t *out_size) {
    INIT_HASH();
    unsigned char *out_buffer = malloc(in_size + (in_size / 8) + 9);
    if (!out_buffer) return NULL;
    memcpy(out_buffer, &in_size, 8);

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, out_buffer, 8);

    uint64_t read_index = 0;
    uint32_t prev_len = 0, prev_dist = 0;
    int match = 0;

    while (read_index < in_size) {
        int hash = 0;
        uint32_t curr_len = 0, curr_pos = -1;
        if (read_index + 4 <= in_size) {
            UPDATE_HASH(hash, in_buffer, read_index);
            MATCH_HASH(hash, read_index, in_buffer, in_size - read_index, &curr_len, &curr_pos);
            INSERT_HASH(hash, read_index);
        }
        if (match) {
            if (curr_len > prev_len) {
                WRITE_BITS(&bit_buffer, 0, 1);
                WRITE_BITS(&bit_buffer, in_buffer[read_index], 8);
                prev_len = curr_len;
                prev_dist = (int32_t)(read_index - curr_pos);
                read_index++;
            } else {
                WRITE_BITS(&bit_buffer, 1, 1);
                WRITE_BITS(&bit_buffer, prev_dist, 15);
                WRITE_BITS(&bit_buffer, prev_len, 8);
                read_index += prev_len - 1;
                match = 0;
            }
        } else {
            if (curr_len >= MIN_MATCH) {
                prev_len = curr_len;
                prev_dist = (int32_t)(read_index - curr_pos);
                match = 1;
                read_index++;
            } else {
                WRITE_BITS(&bit_buffer, 0, 1);
                WRITE_BITS(&bit_buffer, in_buffer[read_index], 8);
                read_index++;
            }
        }
    }
    if (match) {
        WRITE_BITS(&bit_buffer, 1, 1);
        WRITE_BITS(&bit_buffer, prev_dist, 15);
        WRITE_BITS(&bit_buffer, prev_len, 8);
    }
    FLUSH_BITS(&bit_buffer);
    *out_size = bit_buffer.buff_index;
    return out_buffer;
}

unsigned char *decompress(const unsigned char *in_buffer, const uint64_t in_size, uint64_t *out_size) {
    uint64_t original_size;
    memcpy(&original_size, in_buffer, 8);
    unsigned char *out_buffer = malloc(original_size);
    if (!out_buffer) return NULL;

    uint64_t write_index = 0;
    unsigned char flag, symbol;
    uint16_t distance;
    uint8_t length;

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, (unsigned char*)in_buffer, 8);

    while (write_index < original_size) {
        READ_BITS(&bit_buffer, flag, 1);
        if (flag == 0) {
            READ_BITS(&bit_buffer, symbol, 8);
            out_buffer[write_index++] = symbol;
        } else {
            READ_BITS(&bit_buffer, distance, 15);
            READ_BITS(&bit_buffer, length, 8);
            if (distance == 0 || distance > write_index) {
                free(out_buffer);
                return NULL;
            }
            for (uint16_t i = 0; i < length; i++) {
                out_buffer[write_index] = out_buffer[write_index - distance];
                write_index++;
            }
        }
    }
    *out_size = write_index;
    return out_buffer;
}