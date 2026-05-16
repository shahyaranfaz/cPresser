#include "fileio.h"
#include "xor_delta.h"
#include "rle.h"
#include "lz77.h"
#include "huffman.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILE_SIZE 1000000000

typedef struct {
    int delta;
    int rle;
    int huffman;
    int lz77;
} Settings;

typedef struct {
    unsigned char* data;
    size_t size;
    Settings settings;
    int owns_data;
} CompressionResult;

static void free_compression_result(CompressionResult* result) {
    if (result->owns_data) free(result->data);
    result->data = NULL;
    result->size = 0;
    result->owns_data = 0;
}

static CompressionResult compress_stages(const unsigned char* input,
                                         const size_t input_size,
                                         const Settings initial_settings) {
    CompressionResult result = {
        .data = (unsigned char*)input,
        .size = input_size,
        .settings = initial_settings,
        .owns_data = 0
    };

    size_t rle_size = rle_compressed_size(result.data, result.size);
    if (rle_size < result.size) {
        unsigned char* rle_compressed = rle_compress(result.data, result.size, &rle_size);
        if (rle_compressed != NULL) {
            free_compression_result(&result);
            result.data = rle_compressed;
            result.size = rle_size;
            result.settings.rle = 1;
            result.owns_data = 1;
        }
    }

    size_t huffman_size = 0;
    unsigned char* huffman_compressed = huffman_compress(result.data, result.size, &huffman_size);
    if (huffman_compressed != NULL){
        if (huffman_size < result.size) {
            free_compression_result(&result);
            result.data = huffman_compressed;
            result.size = huffman_size;
            result.settings.huffman = 1;
            result.owns_data = 1;
        } else {
            free(huffman_compressed);
        }
    }

    size_t lz77_size = 0;
    unsigned char* lz77_compressed = lz77_compress(result.data, result.size, &lz77_size);
    if (lz77_compressed != NULL) {
        if (lz77_size < result.size) {
            free_compression_result(&result);
            result.data = lz77_compressed;
            result.size = lz77_size;
            result.settings.lz77 = 1;
            result.owns_data = 1;
        } else {
            free(lz77_compressed);
        }
    }

    return result;
}

size_t compress_file(const char* file_name) {
    size_t original_size;
    unsigned char* original_file = read_file(file_name, &original_size);
    if (!original_file) {
        printf("Error reading file: %s\n", file_name);
        return 0;
    }

    if (original_size == 0) {
        printf("Nothing to compress!\n");
        free(original_file);
        return 0;
    }

    Settings settings = {0};
    CompressionResult best = compress_stages(original_file, original_size, settings);

    size_t delta_size = 0;
    unsigned char* delta_file = delta_compress(original_file, original_size, &delta_size);
    if (delta_file != NULL) {
        Settings delta_settings = {.delta = 1, .rle = 0, .huffman = 0, .lz77 = 0};
        CompressionResult delta_result = compress_stages(delta_file, delta_size, delta_settings);
        if (delta_result.size < best.size) {
            free_compression_result(&best);
            best = delta_result;
        } else {
            free_compression_result(&delta_result);
        }
        free(delta_file);
    }

    unsigned char* write_buffer = malloc(best.size + 4);
    if (!write_buffer) {
        printf("Error allocating memory for compression\n");
        free_compression_result(&best);
        free(original_file);
        return 0;
    }

    memcpy(write_buffer + 4, best.data, best.size);
    write_buffer[0] = best.settings.delta;
    write_buffer[1] = best.settings.rle;
    write_buffer[2] = best.settings.huffman;
    write_buffer[3] = best.settings.lz77;

    char output_name[512];
    snprintf(output_name, sizeof(output_name), "%s.cPressed", file_name);
    size_t written = write_file(output_name, write_buffer, best.size + 4);

    free(write_buffer);
    free_compression_result(&best);
    free(original_file);

    return written;
}

size_t decompress_file(const char* file_name) {
    size_t original_size;
    unsigned char* original_file = read_file(file_name, &original_size);
    if (!original_file) {
        printf("Error reading file: %s\n", file_name);
        return 0;
    }

    if (original_size == 0) {
        printf("Nothing to decompress!\n");
        free(original_file);
        return 0;
    }

    unsigned char* decompressed_file = original_file + 4;
    size_t decompressed_size = original_size - 4;
    Settings settings = {original_file[0], original_file[1], original_file[2], original_file[3]};
    int compressions = 0;

    if (settings.lz77) {
        size_t lz77_size = 0;
        unsigned char* lz77_decompressed = lz77_decompress(decompressed_file, decompressed_size, &lz77_size);
        if (!lz77_decompressed) { free(original_file); return 0; }
        decompressed_file = lz77_decompressed;
        decompressed_size = lz77_size;
        compressions++;
    }

    if (settings.huffman) {
        size_t huffman_size = 0;
        unsigned char* huffman_decompressed = huffman_decompress(decompressed_file, decompressed_size, &huffman_size);
        if (compressions != 0) free(decompressed_file);
        if (!huffman_decompressed) {
            free(original_file);
            return 0;
        }
        decompressed_file = huffman_decompressed;
        decompressed_size = huffman_size;
        compressions++;
    }

    if (settings.rle) {
        size_t rle_size = 0;
        unsigned char* rle_decompressed = rle_decompress(decompressed_file, decompressed_size, &rle_size);
        if (compressions != 0) free(decompressed_file);
        if (!rle_decompressed) {
            free(original_file);
            return 0;
        }
        decompressed_file = rle_decompressed;
        decompressed_size = rle_size;
        compressions++;
    }

    if (settings.delta) {
        size_t delta_size = 0;
        unsigned char* delta_decompressed = delta_decompress(decompressed_file, decompressed_size, &delta_size);
        if (compressions != 0) free(decompressed_file);
        if (!delta_decompressed) {
            free(original_file);
            return 0;
        }
        decompressed_file = delta_decompressed;
        decompressed_size = delta_size;
        compressions++;
    }

    char output_name[512];
    snprintf(output_name, sizeof(output_name), "%s.original", file_name);
    size_t written = write_file(output_name, decompressed_file, decompressed_size);
    if (compressions != 0) free(decompressed_file);
    free(original_file);

    return written;
}

int main() {
    printf("Welcome to cPresser!\n");

    while (1) {
        printf("\nCompress (c), Decompress (d), or Exit (x): ");
        char mode;
        if (scanf(" %c", &mode) != 1) {
            printf("Error reading mode.\n");
            break;
        }
        mode = (char) toupper((unsigned char) mode);

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

        size_t output_size = 0;
        char output_filename[512];
        clock_t start_time = clock();

        if (mode == 'C') {
            output_size = compress_file(input_filename);
            snprintf(output_filename, sizeof(output_filename), "%s.cPressed", input_filename);
        } else {
            output_size = decompress_file(input_filename);
            snprintf(output_filename, sizeof(output_filename), "%s.original", input_filename);
        }
        clock_t end_time = clock();

        if (output_size == 0) continue;

        printf("Success! Output written to '%s' (%zu bytes, %.3f seconds)\n",
               output_filename, output_size, (double) (end_time - start_time) / CLOCKS_PER_SEC);
    }
    return 0;
}
