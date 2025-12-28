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
        test_files/test.c
        test_files/lz77_test_debug.c
)
