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
#define MAX_MATCH   258
#define MAX_CHAIN   128

static int head[HASH_SIZE];
static int prev[SEARCH_BUFFER];

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

#define MATCH_HASH(hash, pos, buffer, lookahead, best_len, best_pos) \
    do { \
        int chain_pos = head[hash]; \
        int chain_count = 0; \
        int max_len = (lookahead < MAX_MATCH) ? lookahead : MAX_MATCH; \
        \
        while (chain_pos >= 0 && chain_count++ < MAX_CHAIN) { \
            int offset = (pos) - chain_pos; \
            if (offset > 0 && offset <= SEARCH_BUFFER) { \
                if (buffer[pos] == buffer[chain_pos] && \
                    buffer[pos + best_len] == buffer[chain_pos + best_len]) { \
                    int len = 0; \
                    while (len < max_len && buffer[pos + len] == buffer[chain_pos + len]) \
                        len++; \
                    \
                    if (len >= MIN_MATCH && len > best_len) { \
                        best_len = len; \
                        best_pos = chain_pos; \
                    } \
                } \
            } \
            chain_pos = prev[chain_pos & (SEARCH_BUFFER - 1)]; \
        } \
    } while(0)

typedef struct {
    unsigned int flag: 1;
    unsigned int distance: 15;
    unsigned int length: 8;
} Match;

typedef struct {
    unsigned int flag: 1;
    unsigned int symbol: 8;
} Literal;

static Match find_longest_match(const unsigned char *buffer, const size_t pos, const size_t buffer_size) {
    Match match = {0};
    if (pos + MIN_MATCH > buffer_size) return match;

   int hash = 0;
    for (int k = 0; k < MIN_MATCH; k++)
        UPDATE_HASH(hash, buffer[pos+k]);

    int best_len = 0;
    int best_pos = 0;
    MATCH_HASH(hash, pos, buffer, buffer_size - pos, best_len, best_pos);
    INSERT_HASH(hash, pos);

    if (best_len > 255) best_len = 255;

    if (best_len >= MIN_MATCH) {
        match.flag = 1;
        match.distance = pos - best_pos;
        if (match.distance > 0x7FFF) match.distance = 0x7FFF;
        match.length = (uint8_t)(best_len);
    }
    return match;
}

#define WRITE_BITS(buf, pos, value, nbits)                         \
    do {                                                            \
        for (int i = (nbits) - 1; i >= 0; i--) {                   \
            if (((value) >> i) & 1)                                 \
                (buf)[*(pos) / 8] |= 1U << (7 - (*(pos) % 8));     \
            else                                                    \
                (buf)[*(pos) / 8] &= ~(1U << (7 - (*(pos) % 8)));  \
            (*(pos))++;                                             \
        }                                                           \
    } while(0)

#define READ_BITS(buf, pos, nbits, out)                        \
    do {                                                        \
        (out) = 0;                                              \
        for (int i = 0; i < (nbits); i++) {                    \
            (out) <<= 1;                                        \
            (out) |= (((buf)[(*(pos)) / 8] >> (7 - ((*(pos)) % 8))) & 1U); \
            (*(pos))++;                                         \
        }                                                       \
    } while(0)

unsigned char *compress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    INIT_HASH();
    unsigned char *out_buffer = malloc(in_size * 4 + 8);
    if (!out_buffer) return NULL;
    memcpy(out_buffer, &in_size, 8);
    size_t read_index = 0, write_index = 64;
    while (read_index < in_size) {
        if (read_index + MIN_MATCH > in_size) {
            WRITE_BITS(out_buffer, &write_index, 0, 1);
            WRITE_BITS(out_buffer, &write_index, in_buffer[read_index], 8);
            read_index++;
            continue;
        }

        const Match match = find_longest_match(in_buffer, read_index, in_size);
        WRITE_BITS(out_buffer, &write_index, match.flag, 1);
        if (match.flag == 0) {
            WRITE_BITS(out_buffer, &write_index, in_buffer[read_index], 8);
            read_index += 1;
        } else {
            WRITE_BITS(out_buffer, &write_index, match.distance, 15);
            WRITE_BITS(out_buffer, &write_index, match.length, 8);

            for (size_t k = 1; k < match.length; k++) {
                if (read_index + k + MIN_MATCH > in_size) break;
                int h = 0;
                for (int t = 0; t < MIN_MATCH; t++)
                    UPDATE_HASH(h, in_buffer[read_index + k + t]);
                INSERT_HASH(h, read_index + k);
            }

            read_index += match.length;
        }
    }
    if (write_index % 8 != 0)
        WRITE_BITS(out_buffer, &write_index, 0, 8 - (write_index % 8));
    *out_size = (write_index + 7) / 8;
    return out_buffer;
}

unsigned char* decompress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    size_t original_size;
    memcpy(&original_size, in_buffer, sizeof(size_t));
    unsigned char *out_buffer = malloc(original_size);
    if (!out_buffer) return NULL;
    size_t read_index = 64, write_index = 0;
    unsigned char flag, symbol;
    uint16_t distance;
    uint8_t length;

    while (write_index < original_size) {
        if (read_index / 8 >= in_size) {
            free(out_buffer);
            return NULL;
        }

        READ_BITS(in_buffer, &read_index, 1, flag);
        if (flag == 0) {
            READ_BITS(in_buffer, &read_index, 8, symbol);
            out_buffer[write_index++] = symbol;
        } else {
            READ_BITS(in_buffer, &read_index, 15, distance);
            READ_BITS(in_buffer, &read_index, 8, length);
            if (distance > write_index || distance == 0) {
                free(out_buffer);
                return NULL;
            }

            if (write_index + length > original_size) {
                free(out_buffer);
                return NULL;
            }

            for (size_t k = 0; k < length; k++) {
                out_buffer[write_index] = out_buffer[write_index-distance];
                write_index++;
            }
        }
    }
    *out_size = write_index;
    return out_buffer;
}
