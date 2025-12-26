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
#define MIN_MATCH   4
#define MAX_MATCH   255
#define MAX_CHAIN   32

static int64_t head[HASH_SIZE];
static int64_t prev[SEARCH_BUFFER];

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
    int max_len = (lookahead < MAX_MATCH) ? lookahead : MAX_MATCH; \
    while (chain_pos >= 0 && chain_count++ < MAX_CHAIN) { \
        int64_t offset = pos - chain_pos; \
        if (offset <= 0 || offset > SEARCH_BUFFER) { \
            chain_pos = prev[chain_pos & (SEARCH_BUFFER - 1)]; \
            continue; \
        } \
        int safe_len = 0; \
        while (safe_len < max_len && pos + safe_len < buf_size && chain_pos + safe_len < buf_size && buffer[pos + safe_len] == buffer[chain_pos + safe_len]) \
            safe_len++; \
        if (safe_len >= MIN_MATCH && safe_len > *best_len && chain_pos < pos) { \
            *best_len = safe_len; \
            *best_pos = chain_pos; \
        } \
        if (safe_len == MAX_MATCH) break; \
        chain_pos = prev[chain_pos & (SEARCH_BUFFER - 1)]; \
    } \
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

#define WRITE_BITS(buf, pos, value, nbits) \
do { \
    uint64_t _byte = *(pos) / 8; \
    int _bit  = *(pos) % 8; \
    for (int _i = (nbits) - 1; _i >= 0; _i--) { \
        buf[_byte] &= ~(1U << (7 - _bit)); \
        buf[_byte] |= (((value) >> _i) & 1U) << (7 - _bit); \
        if (++_bit == 8) { _bit = 0; _byte++; } \
    } \
    *(pos) += nbits; \
} while(0)

#define READ_BITS(buf, pos, nbits, out) \
    do { \
        (out) = 0; \
        for (int i = 0; i < (nbits); i++) { \
            (out) <<= 1; \
            (out) |= (((buf)[(*(pos)) / 8] >> (7 - ((*(pos)) % 8))) & 1U); \
            (*(pos))++; \
        } \
    } while(0)

unsigned char *compress(const unsigned char *in_buffer, const uint64_t in_size, uint64_t *out_size) {
    INIT_HASH();
    unsigned char *out_buffer = malloc(in_size * 3 + 8);
    if (!out_buffer) return NULL;
    memcpy(out_buffer, &in_size, 8);
    uint64_t read_index = 0, write_index = 64;

    int hash = 0;
    for (int i = 0; i < MIN_MATCH - 1 && i < in_size; i++)
        UPDATE_HASH(hash, in_buffer[i]);

    while (read_index < in_size) {
        if (read_index + MIN_MATCH > in_size) {
            WRITE_BITS(out_buffer, &write_index, 0, 1);
            WRITE_BITS(out_buffer, &write_index, in_buffer[read_index], 8);
            read_index++;
            continue;
        }
        UPDATE_HASH(hash, in_buffer[read_index + MIN_MATCH - 1]);
        const Match match = find_longest_match(in_buffer, read_index, in_size, hash);
        WRITE_BITS(out_buffer, &write_index, match.flag, 1);
        if (match.flag == 0) {
            WRITE_BITS(out_buffer, &write_index, in_buffer[read_index], 8);
            read_index++;
        } else {
            WRITE_BITS(out_buffer, &write_index, match.distance, 15);
            WRITE_BITS(out_buffer, &write_index, match.length, 8);

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
    *out_size = (write_index + 7) / 8;
    return out_buffer;
}

unsigned char *decompress(const unsigned char *in_buffer, const uint64_t in_size, uint64_t *out_size) {
    uint64_t original_size;
    memcpy(&original_size, in_buffer, 8);
    unsigned char *out_buffer = malloc(original_size);
    if (!out_buffer) return NULL;
    uint64_t read_index = 64, write_index = 0;
    unsigned char flag, symbol;
    uint16_t distance;
    uint8_t length;

    while (write_index < original_size) {
        READ_BITS(in_buffer, &read_index, 1, flag);
        if (flag == 0) {
            READ_BITS(in_buffer, &read_index, 8, symbol);
            out_buffer[write_index++] = symbol;
        } else {
            READ_BITS(in_buffer, &read_index, 15, distance);
            READ_BITS(in_buffer, &read_index, 8, length);
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