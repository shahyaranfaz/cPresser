#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>

unsigned char *read_file(const char *filename, size_t *read_size);

size_t write_file(const char *filename, const unsigned char* buffer, size_t write_size);

#endif
