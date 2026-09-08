# cPresser

A lossless file compression tool written in C that tries multiple compression stages and keeps each stage only when it reduces the data size.

---

## 🗜️ Features

* **Multi-Algorithm Compression**: Automatically applies the best combination of:
  - Delta (XOR) Encoding
  - Run-Length Encoding (RLE)
  - Huffman Coding
  - LZ77 Dictionary Compression
* **Intelligent Optimization**: Only applies algorithms that actually reduce file size
* **Measured Result**: Reduces the 211,938,580-byte Silesia corpus by 46.97% with byte-for-byte round-trip verification
* **Simple CLI Interface**: Easy-to-use command-line interface
* **Lossless Compression**: Perfect reconstruction of original files

---

## 🚀 Getting Started

### Prerequisites

* C Compiler (GCC or Clang)
* CMake 3.16 or higher

### Installation

```bash
git clone https://github.com/shahyaranfaz/cPresser.git
cd cPresser
mkdir build && cd build
cmake ..
make
```

---

## 📖 Usage

Run the compiled executable:

```bash
./cPress
```

Follow the interactive prompts:
- Enter `c` to compress a file
- Enter `d` to decompress a file
- Enter `x` to exit

### Tests

```bash
ctest --test-dir build --output-on-failure
```

**Compression Example:**
```
Compress (c), Decompress (d), or Exit (x): c
Enter filename: myfile.txt
Success! Output written to 'myfile.txt.cPressed' (1234 bytes, 0.045 seconds)
```

**Decompression Example:**
```
Compress (c), Decompress (d), or Exit (x): d
Enter filename: myfile.txt.cPressed
Success! Output written to 'myfile.txt.cPressed.original' (5678 bytes, 0.032 seconds)
```

---

## 📁 Project Structure

```
cPresser/
├── include/           # Header files
│   ├── fileio.h
│   ├── xor_delta.h
│   ├── rle.h
│   ├── lz77.h
│   ├── huffman.h
│   └── bit_buffer.h
├── src/              # Source files
│   ├── cPresser.c    # Main program logic
│   ├── fileio.c      # File I/O operations
│   ├── xor_delta.c   # Delta encoding
│   ├── rle.c         # Run-length encoding
│   ├── lz77.c        # LZ77 compression
│   └── huffman.c     # Huffman coding
├── CMakeLists.txt    # Build configuration
└── README.md         # Project documentation
```

---

## 🔧 How It Works

cPresser uses a smart compression pipeline:

1. **Delta Encoding**: XORs consecutive bytes to reduce entropy
2. **RLE**: Compresses repeated byte sequences
3. **Huffman Coding**: Variable-length encoding based on symbol frequency
4. **LZ77**: Dictionary-based compression finding repeated patterns

Each algorithm is applied only if it reduces the file size. The compression settings are stored in the output file header for automatic decompression.

## Benchmark

The checked-in benchmark downloads the canonical 12-file Silesia corpus mirror, validates every file against the published corpus size, compresses each file, decompresses it, and compares SHA-256 hashes before reporting results.

```bash
python benchmarks/benchmark_silesia.py build/cPress
```

The verified aggregate result is **112,385,535 compressed bytes from 211,938,580 original bytes**: 53.03% of the original size, or a **46.97% reduction**. The earlier 44.3% reduction claim is therefore supported and superseded by the reproducible result. See [the full benchmark results](benchmarks/results/silesia-2026-09-08.md) for per-file ratios, timings, environment, and methodology.

### Memory use

cPresser processes a complete file in memory, and compression stages may allocate additional buffers. The practical file-size limit therefore depends on available memory; the project does not claim a fixed 1 GB limit.

---

## 📊 Compression Algorithms

### Delta (XOR) Encoding
Reduces redundancy by storing differences between consecutive bytes using XOR operations.

### Run-Length Encoding (RLE)
Efficiently compresses sequences of repeated bytes (e.g., "AAAAA" → "A,5").

### Huffman Coding
Assigns shorter bit codes to frequently occurring symbols, reducing overall size.

### LZ77
Finds and replaces repeated patterns with references to earlier occurrences in a sliding window.

---

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## 📬 Contact

For questions or suggestions, please open an issue or contact [Shahyar Anfaz](https://github.com/shahyaranfaz).
