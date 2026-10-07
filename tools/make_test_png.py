#!/usr/bin/env python3
"""Generate assets/textures/test.png (64x64 checker + red border) without binary blobs."""
import struct, zlib, pathlib

W = H = 64
out = pathlib.Path(__file__).resolve().parent.parent / "assets" / "textures" / "test.png"
out.parent.mkdir(parents=True, exist_ok=True)

raw = b""
for y in range(H):
    raw += b"\x00"  # filter byte
    for x in range(W):
        border = x < 3 or y < 3 or x >= W - 3 or y >= H - 3
        if border:
            raw += bytes((230, 40, 40, 255))
        else:
            checker = ((x // 8) + (y // 8)) % 2 == 0
            raw += bytes((32, 200, 220, 255)) if checker else bytes((245, 245, 245, 255))

def chunk(tag, data):
    c = tag + data
    return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

png = (b"\x89PNG\r\n\x1a\n"
       + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0))
       + chunk(b"IDAT", zlib.compress(raw, 9))
       + chunk(b"IEND", b""))
out.write_bytes(png)
print(f"wrote {out} ({len(png)} bytes)")
