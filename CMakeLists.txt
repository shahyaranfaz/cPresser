cmake_minimum_required(VERSION 3.16)

project(cPress LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(core STATIC src/fileio.c)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(lz77 STATIC src/lz77.c)
add_library(huffman STATIC src/huffman.c)
add_library(rle STATIC src/rle.c)
add_library(xor_delta STATIC src/xor_delta.c)

foreach(lib lz77 huffman rle xor_delta)
    target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

add_executable(cPress src/cPresser.c)

target_link_libraries(cPress PRIVATE
        core
        lz77
        huffman
        rle
        xor_delta
)

target_compile_options(cPress PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
