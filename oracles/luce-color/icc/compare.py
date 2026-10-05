#!/usr/bin/env python3
"""Compare two dumps of the skcms oracle / luce-color's icc_tool line by line.

Exact by default. With --ulps N, eight-digit hex tokens (f32 bits) may differ by N units in
the last place, or by --abs D in value, and byte rows (64 bytes a line) by --bytes B, which is
how the NEON build of skcms (skcms_oracle_simd, contracting a*b+c, its own approximations
vectorized) is held against the port.

usage: compare.py A B [--ulps 64] [--abs 1e-4] [--bytes 1]
"""
import argparse
import struct
import sys


def ulps(a, b):
    def ordered(bits):
        return bits if bits < 0x80000000 else 0x80000000 - bits
    fa = struct.unpack("<f", struct.pack("<I", a))[0]
    fb = struct.unpack("<f", struct.pack("<I", b))[0]
    if fa != fa and fb != fb:
        return 0
    return abs(ordered(a) - ordered(b))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("a")
    parser.add_argument("b")
    parser.add_argument("--ulps", type=int, default=0)
    parser.add_argument("--abs", type=float, default=0.0)
    parser.add_argument("--bytes", type=int, default=0)
    args = parser.parse_args()
    a = open(args.a).read().splitlines()
    b = open(args.b).read().splitlines()
    if len(a) != len(b):
        print(f"line counts differ: {len(a)} and {len(b)}")
        return 1
    worst_ulps = 0
    worst_abs = 0.0
    worst_byte = 0
    failures = 0
    profile = ""
    for number, (x, y) in enumerate(zip(a, b), 1):
        if x.startswith("profile "):
            profile = x
        if x == y:
            continue
        tx, ty = x.split(), y.split()
        ok = len(tx) == len(ty)
        for p, q in zip(tx, ty):
            if p == q:
                continue
            if len(p) == 8 and len(q) == 8:
                d = ulps(int(p, 16), int(q, 16))
                fp = struct.unpack("<f", struct.pack("<I", int(p, 16)))[0]
                fq = struct.unpack("<f", struct.pack("<I", int(q, 16)))[0]
                e = abs(fp - fq) if fp == fp and fq == fq else 0.0
                if d > args.ulps:
                    worst_abs = max(worst_abs, e)
                worst_ulps = max(worst_ulps, d)
                ok = ok and (d <= args.ulps or e <= args.abs)
            elif len(p) == len(q) and len(p) % 2 == 0 and len(p) > 8:
                for i in range(0, len(p), 2):
                    d = abs(int(p[i:i + 2], 16) - int(q[i:i + 2], 16))
                    worst_byte = max(worst_byte, d)
                    ok = ok and d <= args.bytes
            else:
                ok = False
        if not ok:
            failures += 1
            if failures <= 10:
                print(f"{profile}: line {number}\n  {x[:160]}\n  {y[:160]}")
    print(f"{len(a)} lines, {failures} outside tolerance; worst {worst_ulps} ulps (beyond them, {worst_abs:.3g} apart at most), worst byte {worst_byte}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
