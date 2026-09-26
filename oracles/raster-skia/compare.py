#!/usr/bin/env python3
"""compare.py A B [--list] : compare every PNG under directory A with the same path under B.

Pixels are compared premultiplied (tiny-skia's premultiply_u8), as the raster tests do.
Prints one line per image: same / N px differ (max delta, first x,y) / missing.
"""
import os, sys
from PIL import Image

def premul(img):
    img = img.convert("RGBA")
    w, h = img.size
    px = img.tobytes()
    out = bytearray(px)
    for i in range(0, len(px), 4):
        a = px[i + 3]
        for c in range(3):
            prod = px[i + c] * a + 128
            out[i + c] = (prod + (prod >> 8)) >> 8
    return w, h, bytes(out)

def compare(a, b):
    wa, ha, pa = premul(Image.open(a))
    wb, hb, pb = premul(Image.open(b))
    if (wa, ha) != (wb, hb):
        return f"size {wa}x{ha} vs {wb}x{hb}"
    if pa == pb:
        return None
    n = 0; mx = 0; first = None
    for i in range(0, len(pa), 4):
        if pa[i:i+4] != pb[i:i+4]:
            n += 1
            d = max(abs(pa[i+k] - pb[i+k]) for k in range(4))
            mx = max(mx, d)
            if first is None:
                first = ((i // 4) % wa, (i // 4) // wa)
    return f"{n} px differ (max {mx}, first {first[0]},{first[1]})"

def main():
    a, b = sys.argv[1], sys.argv[2]
    same = diff = missing = 0
    for root, _, files in os.walk(a):
        for f in sorted(files):
            if not f.endswith(".png"):
                continue
            pa = os.path.join(root, f)
            rel = os.path.relpath(pa, a)
            pb = os.path.join(b, rel)
            if not os.path.exists(pb):
                missing += 1
                print(f"missing  {rel}")
                continue
            r = compare(pa, pb)
            if r is None:
                same += 1
                print(f"same     {rel}")
            else:
                diff += 1
                print(f"DIFF     {rel}: {r}")
    print(f"{same} same, {diff} differ, {missing} missing")

main()
