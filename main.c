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

Match find_longest_match(unsigned char *buffer, size_t pos, size_t buf_size);

size_t tokenize(unsigned char* buffer, Match match, unsigned char symbol) {
    size_t i = 0;
    buffer[i++] = match.distance >> 8;
    buffer[i++] = match.distance & 0xFF;
    buffer[i++] = match.length;
    buffer[i++] = symbol;
    return i;
}

unsigned char *compress(unsigned char *in_buffer, const size_t in_size, size_t *out_size) {
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

        unsigned char* tmp = realloc(out_buffer,j + length + 1);
        if (!tmp) {
            free(out_buffer);
            return NULL;
        }
        out_buffer = tmp;

        for (size_t k = 0; k < length; k++) {
            if (distance > j) {
                free(out_buffer);
                return NULL;
            }
            out_buffer[j] = out_buffer[j-distance];
            j++;
        }
        out_buffer[j++] = symbol;
    }
    *out_size = j;
    return out_buffer;
}

// --- MAIN ---
int main(void) {
    return 0;
}
