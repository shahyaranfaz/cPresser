#!/usr/bin/env python3

import random
import subprocess
import sys
import tempfile
from pathlib import Path


CASES = {
    "single-byte": b"x",
    "rle-255-byte-boundary": b"\x00" * 255,
    "rle-256-byte-boundary": b"\x00" * 256,
    "repeated-text": (b"space invaders " * 2048),
    "structured-text": b"\n".join(
        f'<player id="{index}">score={index * 17}</player>'.encode()
        for index in range(1000)
    ),
    "all-byte-values": bytes(range(256)) * 32,
    "deterministic-random": random.Random(0).randbytes(16384),
}


def run_command(executable, directory, commands):
    result = subprocess.run(
        [executable],
        cwd=directory,
        input="\n".join(commands) + "\n",
        text=True,
        capture_output=True,
        timeout=60,
    )
    if result.returncode != 0:
        raise AssertionError(result.stdout + result.stderr)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_roundtrip.py /path/to/cPress")
    executable = str(Path(sys.argv[1]).resolve())

    with tempfile.TemporaryDirectory() as temp_directory:
        directory = Path(temp_directory)
        for name, payload in CASES.items():
            source = directory / f"{name}.bin"
            source.write_bytes(payload)

            run_command(executable, directory, ["c", source.name, "x"])
            compressed = source.with_name(source.name + ".cPressed")
            if not compressed.is_file():
                raise AssertionError(f"{name}: compressor did not create output")

            run_command(executable, directory, ["d", compressed.name, "x"])
            restored = compressed.with_name(compressed.name + ".original")
            if restored.read_bytes() != payload:
                raise AssertionError(f"{name}: decompressed bytes differ from input")

            print(f"PASS {name}: {len(payload)} -> {compressed.stat().st_size} bytes")


if __name__ == "__main__":
    main()
