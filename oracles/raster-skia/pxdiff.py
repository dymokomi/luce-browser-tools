#!/usr/bin/env python3
"""pxdiff.py A.png B.png [N] : list the first N differing pixels (premultiplied RGBA)."""
import sys
from PIL import Image
def premul(path):
    img = Image.open(path).convert("RGBA"); w, h = img.size; px = list(img.getdata())
    out = []
    for (r, g, b, a) in px:
        def pm(c):
            prod = c * a + 128
            return (prod + (prod >> 8)) >> 8
        out.append((pm(r), pm(g), pm(b), a))
    return w, h, out
wa, ha, a = premul(sys.argv[1]); wb, hb, b = premul(sys.argv[2])
n = int(sys.argv[3]) if len(sys.argv) > 3 else 20
k = 0
for i in range(len(a)):
    if a[i] != b[i]:
        print(f"{i % wa},{i // wa}: {a[i]} vs {b[i]}")
        k += 1
        if k >= n: break
