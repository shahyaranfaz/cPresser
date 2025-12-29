cmake_minimum_required(VERSION 4.1)
project(cPress C)

set(CMAKE_C_STANDARD 11)

add_executable(cPress main.c
        lz77.c
        lz77.h
        fileio.c
        fileio.h
        huffman.c
        huffman.h
        bit_buffer.h
        xor_delta.c
        xor_delta.h
        rle.c
        rle.h
)
