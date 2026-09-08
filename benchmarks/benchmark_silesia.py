#!/usr/bin/env python3

import argparse
import hashlib
import shutil
import subprocess
import tempfile
import time
import urllib.request
import zipfile
from pathlib import Path


CORPUS_URL = "https://github.com/yewq/Silesia-compression-corpus/raw/main/silesia.zip"
EXPECTED_SIZES = {
    "dickens": 10192446,
    "mozilla": 51220480,
    "mr": 9970564,
    "nci": 33553445,
    "ooffice": 6152192,
    "osdb": 10085684,
    "reymont": 6627202,
    "samba": 21606400,
    "sao": 7251944,
    "webster": 41458703,
    "x-ray": 8474240,
    "xml": 5345280,
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def run_cpress(executable, directory, mode, filename):
    started = time.perf_counter()
    result = subprocess.run(
        [executable],
        cwd=directory,
        input=f"{mode}\n{filename}\nx\n",
        text=True,
        capture_output=True,
    )
    elapsed = time.perf_counter() - started
    if result.returncode != 0 or "Success!" not in result.stdout:
        raise RuntimeError(result.stdout + result.stderr)
    return elapsed


def prepare_corpus(corpus_directory, download_directory):
    if corpus_directory:
        return corpus_directory.resolve()

    download_directory.mkdir(parents=True, exist_ok=True)
    archive = download_directory / "silesia.zip"
    if not archive.exists():
        print(f"Downloading {CORPUS_URL}")
        urllib.request.urlretrieve(CORPUS_URL, archive)

    extracted = download_directory / "silesia"
    if not extracted.exists():
        with zipfile.ZipFile(archive) as corpus_zip:
            corpus_zip.extractall(extracted)
    return extracted


def validate_corpus(directory):
    files = {}
    for name, expected_size in EXPECTED_SIZES.items():
        matches = list(directory.rglob(name))
        if len(matches) != 1 or matches[0].stat().st_size != expected_size:
            raise RuntimeError(f"Corpus validation failed for {name}")
        files[name] = matches[0]
    return files


def main():
    parser = argparse.ArgumentParser(description="Benchmark cPresser on the Silesia corpus")
    parser.add_argument("executable", type=Path)
    parser.add_argument("--corpus", type=Path, help="existing extracted Silesia directory")
    parser.add_argument("--cache", type=Path, default=Path("build/benchmark-cache"))
    args = parser.parse_args()

    executable = str(args.executable.resolve())
    corpus = prepare_corpus(args.corpus, args.cache.resolve())
    files = validate_corpus(corpus)
    total_original = 0
    total_compressed = 0
    total_compress_seconds = 0.0
    total_decompress_seconds = 0.0

    with tempfile.TemporaryDirectory() as temp_directory:
        working_directory = Path(temp_directory)
        print("file,original_bytes,compressed_bytes,ratio_percent,compress_seconds,decompress_seconds")
        for name, source in files.items():
            working_source = working_directory / name
            shutil.copyfile(source, working_source)
            compress_seconds = run_cpress(executable, working_directory, "c", name)
            compressed = working_directory / f"{name}.cPressed"
            decompress_seconds = run_cpress(executable, working_directory, "d", compressed.name)
            restored = working_directory / f"{compressed.name}.original"
            if sha256(working_source) != sha256(restored):
                raise RuntimeError(f"Round-trip verification failed for {name}")

            original_size = working_source.stat().st_size
            compressed_size = compressed.stat().st_size
            ratio = compressed_size / original_size * 100
            print(f"{name},{original_size},{compressed_size},{ratio:.2f},{compress_seconds:.3f},{decompress_seconds:.3f}")
            total_original += original_size
            total_compressed += compressed_size
            total_compress_seconds += compress_seconds
            total_decompress_seconds += decompress_seconds

    ratio = total_compressed / total_original * 100
    reduction = 100 - ratio
    print(f"TOTAL,{total_original},{total_compressed},{ratio:.2f},{total_compress_seconds:.3f},{total_decompress_seconds:.3f}")
    print(f"Verified reduction: {reduction:.2f}% (compressed size: {ratio:.2f}% of original)")


if __name__ == "__main__":
    main()
