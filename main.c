#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEARCH_BUFFER 65536
#define LOOKAHEAD_BUFFER 256
#define CHUNK_SIZE 128000000
#define MAX_FILE_SIZE 1000000000

//FILE INPUT
unsigned char *read_file(const char *filename, size_t *read_size) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    const long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);

    if (size == 0) {
        fclose(fp);
        *read_size = 0;
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
        size_t to_read = (size_t)size - total_read;
        if (to_read > CHUNK_SIZE)
            to_read = CHUNK_SIZE;

        const size_t bytes = fread(buffer + total_read, 1, to_read, fp);

        if (bytes == 0) {
            if (ferror(fp)) {
                free(buffer);
                fclose(fp);
                return NULL;
            }
            break;
        }

        total_read += bytes;
    }

    fclose(fp);

    if (total_read != (size_t) size) {
        free(buffer);
        return NULL;
    }

    *read_size = total_read;
    return buffer;
}

void write_file(const char *filename, unsigned char* buffer, size_t *write_size) {
    if (!buffer || !write_size || *write_size == 0) return;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error opening file");
        return;
    }

    size_t total_written = 0;

    while (total_written < *write_size) {
        size_t to_write = *write_size - total_written;
        if (to_write > CHUNK_SIZE)
            to_write = CHUNK_SIZE;

        const size_t bytes = fwrite(buffer + total_written, 1, to_write, fp);
        if (bytes == 0) {
            if (ferror(fp)) {
                perror("Error writing file");
            }
            break;
        }

        total_written += bytes;
    }
    if (total_written != *write_size) {
        fprintf(stderr, "Warning: wrote %zu of %zu bytes\n", total_written, *write_size);
    }

    fclose(fp);
    *write_size = total_written;
}

//COMPRESSION
#define HASH_BITS   16
#define HASH_SIZE   (1 << HASH_BITS)
#define HASH_MASK   (HASH_SIZE - 1)
#define HASH_SHIFT  5
#define MIN_MATCH   3
#define MAX_MATCH   258
#define MAX_CHAIN   128
int head[HASH_SIZE];
int prev[SEARCH_BUFFER];

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
    uint16_t distance;
    uint8_t length;
} Match;

Match find_longest_match(const unsigned char *buffer, const size_t pos, const size_t buffer_size) {
    Match match = {0,0};
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
    INIT_HASH();
    unsigned char *out_buffer = malloc(in_size * 4);
    if (!out_buffer) return NULL;
    size_t i = 0, j = 0;
    while (i < in_size) {
        if (i + MIN_MATCH > in_size) {
            out_buffer[j++] = 0;
            out_buffer[j++] = 0;
            out_buffer[j++] = 0;
            out_buffer[j++] = in_buffer[i++];
            continue;
        }

        const Match match = find_longest_match(in_buffer, i, in_size);
        const unsigned char next_symbol = (i + match.length < in_size) ? in_buffer[i + match.length] : 0;
        j += tokenize(&out_buffer[j], match, next_symbol);

        for (size_t k = 1; k <= match.length; k++) {
            if (i + k + MIN_MATCH > in_size)
                break;

            int h = 0;
            for (int t = 0; t < MIN_MATCH; t++)
                UPDATE_HASH(h, in_buffer[i + k + t]);

            INSERT_HASH(h, i + k);
        }
        i += match.length + 1;
    }
    *out_size = j;
    return out_buffer;
}

//DECOMPRESSION
unsigned char* decompress(const unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
    size_t capacity = in_size * 4;
    unsigned char *out_buffer = malloc(capacity);
    if (!out_buffer) return NULL;
    size_t i = 0, j = 0;
    while (i + 3 < in_size) {
        const uint16_t distance = (in_buffer[i] << 8) | in_buffer[i + 1];
        const uint8_t length = in_buffer[i+2];
        const unsigned char symbol = in_buffer[i + 3];
        i += 4;

        if (distance > j || (distance == 0 && length != 0)) {
            free(out_buffer);
            return NULL;
        }

        if (j + length + 1 > capacity && capacity * 2 < INT32_MAX) {
            capacity *= 2;
            unsigned char *tmp = realloc(out_buffer, capacity);
            if (!tmp) { free(out_buffer); return NULL; }
            out_buffer = tmp;
        }

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
    while (1) {
        printf("\nCompress (c), Decompress (d), or Exit (x): ");
        char mode;
        scanf(" %c", &mode);
        mode = (char)toupper((unsigned char)mode);

        if (mode == 'X') break;

        if (mode != 'C' && mode != 'D') {
            printf("Invalid input. Please try again.\n");
            continue;
        }

        printf("Enter filename: ");
        char input_filename[256];
        if (scanf("%255s", input_filename) != 1) {
            printf("Error reading filename.\n");
            continue;
        }

        size_t read_size;
        unsigned char* buffer = read_file(input_filename, &read_size);
        if (!buffer) {
            printf("Error: Could not read file '%s'\n", input_filename);
            continue;
        }

        if (read_size > MAX_FILE_SIZE) {
            printf("Error: File size too large.\n");
            continue;
        }

        size_t write_size;
        unsigned char* out_buffer;
        char output_filename[300];

        if (mode == 'C') {
            out_buffer = compress(buffer, read_size, &write_size);
            snprintf(output_filename, sizeof(output_filename), "%s.cmp", input_filename);
        } else {
            out_buffer = decompress(buffer, read_size, &write_size);
            const size_t len = strlen(input_filename);
            if (len > 4 && strcmp(input_filename + len - 4, ".cmp") == 0) {
                strncpy(output_filename, input_filename, len - 4);
                output_filename[len - 4] = '\0';
            } else {
                snprintf(output_filename, sizeof(output_filename), "%s.out", input_filename);
            }
        }

        if (!out_buffer) {
            printf("Error: %s failed\n", mode == 'C' ? "Compression" : "Decompression");
            free(buffer);
            continue;
        }

        write_file(output_filename, out_buffer, &write_size);
        printf("Success! Output written to '%s' (%zu bytes)\n", output_filename, write_size);

        free(out_buffer);
        free(buffer);
    }

    return 0;
}