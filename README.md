# cPresser

cPresser is a lossless file compressor written in C. It tries 4 compression methods and keeps a stage only when it makes the data smaller:

1. Delta XOR encoding
2. Run-length encoding (RLE)
3. Huffman coding
4. LZ77

The output header records which stages were used, allowing the decompressor to reverse them automatically.

## Build

You need a C compiler and CMake 3.16 or newer.

```bash
git clone https://github.com/shahyaranfaz/cPresser.git
cd cPresser
cmake -S . -B build
cmake --build build
```

The executable is written to the build directory. With a multi-configuration generator, it may be inside a configuration directory such as `build/Release`.

## Compress and decompress

Run the executable and choose an action at the prompt:

```bash
./build/cPress
```

Use `c` to compress a file:

```text
Compress (c), Decompress (d), or Exit (x): c
Enter filename: myfile.txt
Success! Output written to 'myfile.txt.cPressed' (1234 bytes, 0.045 seconds)
```

Use `d` to restore a `.cPressed` file:

```text
Compress (c), Decompress (d), or Exit (x): d
Enter filename: myfile.txt.cPressed
Success! Output written to 'myfile.txt.cPressed.original' (5678 bytes, 0.032 seconds)
```

Enter `x` to quit.

## Tests

The test suite checks round trips for single-byte input, RLE length boundaries, repeated and structured text, all byte values, and deterministic random data.

```bash
ctest --test-dir build --output-on-failure
```

## Silesia benchmark

The benchmark covers all 12 files in the Silesia compression corpus. It checks the published size of each source file, compresses and decompresses it, then compares the restored file with the source using SHA-256.

```bash
python benchmarks/benchmark_silesia.py build/cPress
```

The recorded run reduced 211,938,580 bytes to 112,385,535 bytes. The compressed output was 53.03% of the original size, a 46.97% reduction. See the [full results](benchmarks/results/silesia-2026-09-08.md) for per-file ratios, timings, environment, and methodology.

## Memory use

cPresser reads a complete file into memory, and compression stages may allocate additional buffers. The practical file-size limit depends on available memory.

## License

[MIT](LICENSE)
