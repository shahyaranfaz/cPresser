#include "fileio.h"
#include "lz77.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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