#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SEARCH_BUFFER 64000
#define LOOKAHEAD_BUFFER 256
#define READ_SIZE 128000000

//FILE INPUT
unsigned char *read_file(const char *filename, size_t *out_size) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) {
        fclose(fp);
        return NULL;
    }

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(fp);
        return NULL;
    }

    size_t total_read = 0;

    while (total_read < (size_t) size) {
        size_t to_read = size - total_read;
        if (to_read > READ_SIZE)
            to_read = READ_SIZE;

        size_t bytes = fread(buffer + total_read, 1, to_read, fp);
        if (bytes == 0) break;

        total_read += bytes;
    }

    fclose(fp);

    if (total_read != (size_t) size) {
        free(buffer);
        return NULL;
    }

    *out_size = total_read;
    return buffer;
}

//COMPRESSION
typedef struct {
    uint16_t distance;
    uint8_t length;
} Match;

#define HASH_BITS   16
#define HASH_SIZE   (1 << HASH_BITS)
#define HASH_MASK   (HASH_SIZE - 1)
#define HASH_SHIFT  5
#define MIN_MATCH   3
#define MAX_MATCH   258
#define MAX_CHAIN   128
int head[HASH_SIZE];
int prev[SEARCH_BUFFER];

//same one as deflate
#define UPDATE_HASH(h, c) (h = (((h) << HASH_SHIFT) ^ (c)) & HASH_MASK)

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
                    while (len + 4 <= max_len) { \
                        if (*(uint32_t*)(buffer + pos + len) != *(uint32_t*)(buffer + chain_pos + len)) \
                            break; \
                        len += 4; \
                    } \
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

Match find_longest_match(const unsigned char *buffer, const size_t pos, const size_t buffer_size) {
    Match match = {0,0};
    if (pos + MIN_MATCH > buffer_size) return match;

    int hash = buffer[pos] & HASH_MASK;
    for (int k = 1; k < MIN_MATCH; k++)
        UPDATE_HASH(hash, buffer[pos+k]);

    int best_len = 0;
    int best_pos = 0;
    MATCH_HASH(hash, pos, buffer, buffer_size - pos, best_len, best_pos);

    if (best_len >= MIN_MATCH) {
        match.distance = (uint16_t)(pos - best_pos);
        match.length = (uint8_t)(best_len);
    }
    return match;
}

size_t tokenize(unsigned char* buffer, const Match match, const unsigned char symbol) {
    size_t i = 0;
    buffer[i++] = match.distance >> 8;
    buffer[i++] = match.distance & 0xFF;
    buffer[i++] = match.length;
    buffer[i++] = symbol;
    return i;
}

unsigned char *compress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    unsigned char *out_buffer = malloc(in_size * 4);
    if (!out_buffer) return NULL;
    size_t i = 0, j = 0;
    while (i < in_size) {
        const Match match = find_longest_match(in_buffer, i, in_size);
        const unsigned char next_symbol = (i + match.length < in_size) ? in_buffer[i + match.length] : 0;
        j += tokenize(&out_buffer[j], match, next_symbol);
        i += match.length + 1;
    }
    *out_size = j;
    return out_buffer;
}
//HUFFMAN

//DECOMPRESSION
unsigned char* decompress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    unsigned char *out_buffer = malloc(in_size);
    if (!out_buffer) return NULL;
    size_t i = 0, j = 0;
    while (i + 3 < in_size) {
        const uint16_t distance = (in_buffer[i] << 8) | in_buffer[i + 1];
        const uint8_t length = in_buffer[i+2];
        const unsigned char symbol = in_buffer[i + 3];
        i += 4;

        if (distance > j) {
            free(out_buffer);
            return NULL;
        }

        unsigned char* tmp = realloc(out_buffer,j + length + 1);
        if (!tmp) {
            free(out_buffer);
            return NULL;
        }
        out_buffer = tmp;

        for (size_t k = 0; k < length; k++) {
            out_buffer[j] = out_buffer[j-distance];
            j++;
        }
        out_buffer[j++] = symbol;
    }
    *out_size = j;
    return out_buffer;
}

int main(void) {
    printf("Compress (c) or Decompress (d): ");
    char mode;
    scanf("%c", &mode);

    printf("Enter filename: ");
    char input_filename[256];
    scanf("%s", input_filename);
}

