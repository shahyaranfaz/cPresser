#include <stdint.h>
#include <stdio.h>

#define SEARCH_BUFFER 64000
#define LOOKAHEAD_BUFFER 256

typedef struct Token {
    uint16_t distance;
    uint8_t length;
    unsigned char symbol;
} Token;

int main(void) { return 0; }

