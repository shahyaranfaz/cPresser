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

static int32_t head[HASH_SIZE];
static int32_t prev[SEARCH_BUFFER];

#define INIT_HASH() \
    do { \
        memset(head, -1, sizeof(head)); \
        memset(prev, -1, sizeof(prev)); \
    } while(0)

//same one as deflate
#define UPDATE_HASH(hash, curr) (hash = (((hash) << HASH_SHIFT) ^ (curr)) & HASH_MASK)

#define INSERT_HASH(hash, pos) \
    do { \
        prev[(pos) & (SEARCH_BUFFER - 1)] = head[hash]; \
        head[hash] = pos; \
    } while(0)

#define MATCH_HASH(hash, pos, buffer, lookahead, best_len, best_pos, buf_size) \
do { \
    int64_t chain_pos = head[hash]; \
    int chain_count = 0; \
    int max_match = (lookahead < MAX_MATCH) ? (int)lookahead : MAX_MATCH; \
    int cur_best = *best_len; \
    while (chain_pos >= 0 && chain_count++ < MAX_CHAIN) { \
        if (buffer[chain_pos + cur_best] == buffer[pos + cur_best]) { \
            int len = 0; \
            while (len < max_match && buffer[pos + len] == buffer[chain_pos + len]) \
                len++; \
            \
            if (len > cur_best) { \
                cur_best = len; \
                *best_pos = chain_pos; \
                if (len == MAX_MATCH) break; \
            } \
        } \
        chain_pos = prev[chain_pos & (SEARCH_BUFFER - 1)]; \
    } \
    *best_len = cur_best; \
} while(0)

typedef struct {
    unsigned int flag: 1;
    unsigned int distance: 15;
    unsigned int length: 8;
} Match;

static Match find_longest_match(const unsigned char *buffer, const uint64_t pos, const uint64_t buffer_size, int hash) {
    Match match = {.flag = 0};
    if (pos + MIN_MATCH > buffer_size) return match;
    int best_len = 0;
    int64_t best_pos = -1;

    MATCH_HASH(hash, pos, buffer, buffer_size - pos, &best_len, &best_pos, buffer_size);
    INSERT_HASH(hash, pos);

    if (best_len >= MIN_MATCH && best_pos >= 0 && best_pos < (int64_t)pos && pos - best_pos < 0x7FFF) {
        match.flag = 1;
        match.distance = pos - best_pos;
        match.length = (best_len > 255) ? 255 : (uint8_t)best_len;
    }
    return match;
}

typedef struct {
    uint64_t accumulator;
    int count;
    unsigned char* buffer;
    uint64_t buff_index;
} BitBuffer;

void init_bit_buffer(BitBuffer* bit_buffer, unsigned char* buffer, uint64_t buff_index) {
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
    uint64_t read_index = 0, write_index = 8;

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, out_buffer, write_index);

    int hash = 0;
    for (int i = 0; i < MIN_MATCH - 1 && i < in_size; i++)
        UPDATE_HASH(hash, in_buffer[i]);

    while (read_index < in_size) {
        if (read_index + MIN_MATCH > in_size) {
            WRITE_BITS(&bit_buffer, 0, 1);
            WRITE_BITS(&bit_buffer, in_buffer[read_index], 8);
            read_index++;
            continue;
        }
        UPDATE_HASH(hash, in_buffer[read_index + MIN_MATCH - 1]);
        const Match match = find_longest_match(in_buffer, read_index, in_size, hash);
        WRITE_BITS(&bit_buffer, match.flag, 1);
        if (match.flag == 0) {
            WRITE_BITS(&bit_buffer, in_buffer[read_index], 8);
            read_index++;
        } else {
            WRITE_BITS(&bit_buffer, match.distance, 15);
            WRITE_BITS(&bit_buffer, match.length, 8);

            for (uint32_t k = 1; k < match.length; k++) {
                read_index++;
                if (read_index + MIN_MATCH - 1 < in_size) {
                    UPDATE_HASH(hash, in_buffer[read_index + MIN_MATCH - 1]);
                    INSERT_HASH(hash, read_index);
                }
            }
            read_index++;
        }
    }
    FLUSH_BITS(&bit_buffer);
    *out_size = bit_buffer.buff_index;
    void *tmp = realloc(out_buffer, *out_size);
    if (tmp) out_buffer = tmp;
    return out_buffer;
}

unsigned char *decompress(const unsigned char *in_buffer, const uint64_t in_size, uint64_t *out_size) {
    uint64_t original_size;
    memcpy(&original_size, in_buffer, 8);
    unsigned char *out_buffer = malloc(original_size);
    if (!out_buffer) return NULL;

    uint64_t read_index = 8, write_index = 0;
    unsigned char flag, symbol;
    uint16_t distance;
    uint8_t length;

    BitBuffer bit_buffer;
    init_bit_buffer(&bit_buffer, (unsigned char*)in_buffer, read_index);

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