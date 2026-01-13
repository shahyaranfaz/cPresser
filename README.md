# cPresser

A high-performance file compression tool written in C that intelligently combines multiple compression algorithms to achieve optimal file size reduction.

---

## 🗜️ Features

* **Multi-Algorithm Compression**: Automatically applies the best combination of:
  - Delta (XOR) Encoding
  - Run-Length Encoding (RLE)
  - Huffman Coding
  - LZ77 Dictionary Compression
* **Intelligent Optimization**: Only applies algorithms that actually reduce file size
* **Fast Processing**: Optimized C implementation with efficient data structures
* **Large File Support**: Handles files up to 1GB
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

This project is licensed under the MIT License. See the LICENSE file for details.

---

## 📬 Contact

For questions or suggestions, please open an issue or contact [Shahyar Anfaz](https://github.com/shahyaranfaz).
