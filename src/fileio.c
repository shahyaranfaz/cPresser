#include "fileio.h"
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 4000000

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

    if (size <= 0) {
        fclose(fp);
        *read_size = 0;
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(fp);
        return NULL;
    }

    size_t total_read = 0;

    while (total_read < (size_t) size) {
        size_t to_read = (size_t)size - total_read;
        to_read = to_read > CHUNK_SIZE ? CHUNK_SIZE : to_read;
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

static size_t write_buffer(FILE *fp, const unsigned char *buffer, const size_t write_size) {
    size_t total_written = 0;
    while (total_written < write_size) {
        size_t to_write = write_size - total_written;
        if (to_write > CHUNK_SIZE)
            to_write = CHUNK_SIZE;
        const size_t bytes = fwrite(buffer + total_written, 1, to_write, fp);
        if (bytes == 0) {
            if (ferror(fp))
                perror("Error writing file");
            return total_written;
        }
        total_written += bytes;
    }
    return total_written;
}

size_t write_file(const char *filename, const unsigned char* buffer, const size_t write_size) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;
    const size_t written = write_buffer(fp, buffer, write_size);
    fclose(fp);
    return written;
}

size_t write_file_parts(const char *filename,
                        const unsigned char *prefix,
                        const size_t prefix_size,
                        const unsigned char *buffer,
                        const size_t buffer_size) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;

    const size_t prefix_written = write_buffer(fp, prefix, prefix_size);
    size_t buffer_written = 0;
    if (prefix_written == prefix_size)
        buffer_written = write_buffer(fp, buffer, buffer_size);

    fclose(fp);
    return prefix_written + buffer_written;
}
