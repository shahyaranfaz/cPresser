cmake_minimum_required(VERSION 3.16)

project(cPress LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CPRESS_ENABLE_NATIVE "Enable CPU-specific optimizations such as -march=native" OFF)
set(CPRESS_LZ77_MAX_CHAIN 32 CACHE STRING "Maximum hash-chain probes for LZ77 match search")

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

set(CPRESS_TARGETS core lz77 huffman rle xor_delta cPress)

foreach(target ${CPRESS_TARGETS})
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    if(CPRESS_ENABLE_NATIVE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -march=native)
    endif()
endforeach()

target_compile_definitions(lz77 PRIVATE CPRESS_LZ77_MAX_CHAIN=${CPRESS_LZ77_MAX_CHAIN})
