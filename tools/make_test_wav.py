#!/usr/bin/env python3
"""Generate assets/audio/blip.wav (0.25s 440Hz sine) without binary blobs."""
import math, struct, pathlib

SR = 22050
DUR = 0.25
FREQ = 440.0
N = int(SR * DUR)
out = pathlib.Path(__file__).resolve().parent.parent / "assets" / "audio" / "blip.wav"
out.parent.mkdir(parents=True, exist_ok=True)

samples = bytearray()
for i in range(N):
    t = i / SR
    env = 1.0 - (i / N)  # linear decay so no click
    v = int(12000 * env * math.sin(2 * math.pi * FREQ * t))
    samples += struct.pack("<h", v)

data = bytes(samples)
wav = (b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE"
       + b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, SR, SR * 2, 2, 16)
       + b"data" + struct.pack("<I", len(data)) + data)
out.write_bytes(wav)
print(f"wrote {out} ({len(wav)} bytes, {N} samples)")
