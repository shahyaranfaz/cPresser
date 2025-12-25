#ifndef FILEIO_H
#define FILEIO_H

#include <stddef.h>

unsigned char *read_file(const char *filename, size_t *read_size);

void write_file(const char *filename, unsigned char* buffer, size_t *write_size);

#endif
