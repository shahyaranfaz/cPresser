#include "fileio.h"
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 128000000

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
            if (ferror(fp))
                perror("Error writing file");
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