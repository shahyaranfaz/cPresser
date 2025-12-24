#include <stdint.h>
#include <stdio.h>

#define SEARCH_BUFFER 64000
#define LOOKAHEAD_BUFFER 256

typedef struct {
    uint16_t distance;
    uint8_t length;
    unsigned char symbol;
} Token;

typedef struct {
    int distance;
    int length;
} Match;

//FILE INPUT
void read_file(char* file_name);

//COMPRESSION
Match find_longest_match();
Token build_token();
void tokenize();
void encode();
void write_file(char* file_name);

//DECOMPRESSION
void decode();

int main(void) { return 0; }

