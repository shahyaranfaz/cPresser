#include "fileio.h"
#include "lz77.h"
#include "huffman.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILE_SIZE 1000000000

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
            free(buffer);
            continue;
        }

        size_t temp_size1, temp_size2;
        unsigned char *temp_buffer1 = NULL, *temp_buffer2 = NULL;
        char output_filename[300];

        clock_t start_time = clock();

        if (mode == 'C') {
            temp_buffer1 = lz77_compress(buffer, read_size, &temp_size1);
            if (!temp_buffer1) {
                printf("Error: LZ77 compression failed\n");
                free(buffer);
                continue;
            }
            temp_buffer2 = huffman_compress(temp_buffer1, temp_size1, &temp_size2);
            free(temp_buffer1);
            if (!temp_buffer2) {
                printf("Error: Huffman compression failed\n");
                free(buffer);
                continue;
            }
            snprintf(output_filename, sizeof(output_filename), "%s.cmp", input_filename);
        } else {
            temp_buffer1 = huffman_decompress(buffer, read_size, &temp_size1);
            if (!temp_buffer1) {
                printf("Error: Huffman decompression failed\n");
                free(buffer);
                continue;
            }
            temp_buffer2 = lz77_decompress(temp_buffer1, temp_size1, &temp_size2);
            free(temp_buffer1);
            if (!temp_buffer2) {
                printf("Error: LZ77 decompression failed\n");
                free(buffer);
                continue;
            }
            const size_t len = strlen(input_filename);
            if (len > 4 && strcmp(input_filename + len - 4, ".cmp") == 0) {
                strncpy(output_filename, input_filename, len - 4);
                output_filename[len - 4] = '\0';
            } else {
                snprintf(output_filename, sizeof(output_filename), "%s.out", input_filename);
            }
        }
        write_file(output_filename, temp_buffer2, &temp_size2);
        clock_t end_time = clock();
        printf("Success! Output written to '%s' (%zu bytes, %.3f seconds)\n",
               output_filename, temp_size2, (double)(end_time - start_time) / CLOCKS_PER_SEC);
        free(temp_buffer2);
        free(buffer);
    }
    return 0;
}
