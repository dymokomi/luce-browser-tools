#!/usr/bin/env python3
"""Make APNG test files for luce-png: `gen_apng.py OUTDIR`.

Pillow's APNG writer gives ordinary animations (sub-rectangle frames, every disposal and
blend, a hidden default image, palette, gray and opaque images); a small chunk writer gives
the cases encoders do not write: offsets, OVER on translucent pixels, delays with a zero
numerator or denominator, interlaced frames, chunks between frames, more or fewer frames
than acTL says, sequence numbers out of order, CRCs broken in fcTL, fdAT and IDAT, invalid
fcTL values before and after the image data, the first fcTL off the origin or of another
size, files cut short.
"""
import random, struct, sys, zlib
from pathlib import Path
from PIL import Image, ImageDraw

OUT = Path(sys.argv[1])
OUT.mkdir(parents=True, exist_ok=True)


def chunk(name, body, crc=None):
    data = name.encode() + body
    value = zlib.crc32(data) if crc is None else crc
    return struct.pack(">I", len(body)) + data + struct.pack(">I", value & 0xffffffff)


def scanlines(pixels, width, height, channels, interlaced=False):
    """Raw scanlines with filter 0 (or the seven Adam7 passes)."""
    if not interlaced:
        return b"".join(b"\x00" + bytes(pixels[(y * width) * channels:(y * width + width) * channels]) for y in range(height))
    out = b""
    for xs, ys, dx, dy in [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]:
        for y in range(ys, height, dy):
            row = b"".join(bytes(pixels[(y * width + x) * channels:(y * width + x + 1) * channels]) for x in range(xs, width, dx))
            if row:
                out += b"\x00" + row
    return out


def rgba(width, height, seed, alpha=True):
    rng = random.Random(seed)
    pixels = []
    for y in range(height):
        for x in range(width):
            pixels += [(x * 37 + seed * 11) % 256, (y * 53 + seed * 7) % 256, rng.randrange(256), rng.choice([0, 60, 128, 200, 255]) if alpha else 255]
    return pixels


class Writer:
    """An APNG built chunk by chunk, color type 6 (or 2) at 8 bits."""

    def __init__(self, width, height, color=6, interlaced=False):
        self.width, self.height, self.color, self.interlaced = width, height, color, interlaced
        self.parts = [b"\x89PNG\r\n\x1a\n", chunk("IHDR", struct.pack(">IIBBBBB", width, height, 8, color, 0, 0, 1 if interlaced else 0))]
        self.seq = 0

    def actl(self, frames, plays=0):
        self.parts.append(chunk("acTL", struct.pack(">II", frames, plays)))

    def fctl(self, w, h, x, y, num=10, den=100, dispose=0, blend=0, seq=None, crc=None, length=26):
        body = struct.pack(">IIIIIHHBB", self.seq if seq is None else seq, w, h, x, y, num, den, dispose, blend)[:length]
        self.parts.append(chunk("fcTL", body, crc))
        self.seq += 1

    def data(self, w, h, seed, first=False, split=1, crc=None, seq=None, alpha=True):
        channels = 4 if self.color == 6 else 3
        pixels = rgba(w, h, seed, alpha)
        if channels == 3:
            pixels = [v for i, v in enumerate(pixels) if i % 4 != 3]
        stream = zlib.compress(scanlines(pixels, w, h, channels, self.interlaced))
        size = (len(stream) + split - 1) // split
        for i in range(split):
            piece = stream[i * size:(i + 1) * size]
            if first:
                self.parts.append(chunk("IDAT", piece, crc))
            else:
                self.parts.append(chunk("fdAT", struct.pack(">I", self.seq if seq is None else seq) + piece, crc))
                self.seq += 1

    def raw(self, name, body):
        self.parts.append(chunk(name, body))

    def save(self, name, end=True, cut=None):
        data = b"".join(self.parts) + (chunk("IEND", b"") if end else b"")
        (OUT / f"{name}.png").write_bytes(data[:cut] if cut else data)


def pillow():
    frames = []
    for i in range(5):
        image = Image.new("RGBA", (40, 30), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        draw.rectangle((i * 5, i * 3, i * 5 + 15, i * 3 + 10), fill=(255 - i * 40, i * 50, 128, 255 if i % 2 else 140))
        draw.ellipse((20 - i, 5, 35, 25 - i), fill=(i * 60, 200, 255 - i * 30, 90))
        frames.append(image)
    combos = [("pillow-keep-source", [0] * 5, [0] * 5), ("pillow-keep-over", [0] * 5, [1] * 5), ("pillow-background", [1] * 5, [1, 0, 1, 0, 1]),
              ("pillow-previous", [2, 2, 0, 2, 1], [1] * 5), ("pillow-mixed", [0, 1, 2, 0, 1], [1, 0, 1, 1, 0])]
    for name, disposal, blend in combos:
        frames[0].save(OUT / f"{name}.png", save_all=True, append_images=frames[1:], duration=[30, 0, 1, 100, 250], loop=3, disposal=disposal, blend=blend)
    frames[0].save(OUT / "pillow-default-image.png", save_all=True, append_images=frames[1:], duration=50, loop=0, default_image=True)
    for mode in ["P", "L", "LA", "RGB"]:
        converted = [f.convert(mode) for f in frames]
        # Pillow disposes only RGBA frames to the background.
        converted[0].save(OUT / f"pillow-mode-{mode}.png", save_all=True, append_images=converted[1:], duration=40, loop=1, disposal=0, blend=1)
    # A one-frame APNG and a still PNG.
    frames[0].save(OUT / "pillow-one-frame.png", save_all=True, append_images=[], duration=1000)
    frames[1].save(OUT / "still.png")


def hand():
    w = Writer(16, 12)
    w.actl(3, 2)
    w.fctl(16, 12, 0, 0, 0, 0, dispose=2, blend=1)
    w.data(16, 12, 1, first=True, split=2)
    w.raw("tEXt", b"Comment\x00between frames")
    w.fctl(8, 6, 4, 3, num=5, den=0, dispose=1, blend=1)
    w.data(8, 6, 2)
    w.fctl(10, 4, 6, 8, num=0, den=7, dispose=0, blend=1)
    w.data(10, 4, 3, split=3)
    w.save("hand-offsets-over")

    w = Writer(9, 7)
    w.actl(2)
    w.data(9, 7, 4, first=True)
    w.fctl(5, 5, 2, 1, blend=1)
    w.data(5, 5, 5)
    w.fctl(9, 7, 0, 0, dispose=2)
    w.data(9, 7, 6)
    w.save("hand-hidden-default")

    w = Writer(12, 12, interlaced=True)
    w.actl(2)
    w.fctl(12, 12, 0, 0)
    w.data(12, 12, 7, first=True)
    w.fctl(7, 9, 5, 3, blend=1)
    w.data(7, 9, 8)
    w.save("hand-interlaced")

    w = Writer(10, 6, color=2)
    w.actl(2)
    w.fctl(10, 6, 0, 0, blend=1)
    w.data(10, 6, 9, first=True, alpha=False)
    w.fctl(4, 4, 1, 1, blend=1)
    w.data(4, 4, 10, alpha=False)
    w.save("hand-opaque-over")

    # More frames in the file than acTL says, and fewer.
    w = Writer(8, 8)
    w.actl(2)
    for i in range(3):
        w.fctl(8, 8, 0, 0)
        w.data(8, 8, 20 + i, first=(i == 0))
    w.save("hand-extra-frames")
    w = Writer(8, 8)
    w.actl(4)
    for i in range(2):
        w.fctl(8, 8, 0, 0)
        w.data(8, 8, 30 + i, first=(i == 0))
    w.save("hand-missing-frames")

    # Sequence numbers out of order, before and after the image data.
    w = Writer(8, 8)
    w.actl(3)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 40, first=True)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 41, seq=99)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 42)
    w.save("hand-sequence-after")
    w = Writer(8, 8)
    w.actl(1)
    w.fctl(8, 8, 0, 0, seq=5)
    w.data(8, 8, 43, first=True)
    w.save("hand-sequence-before")

    # CRCs broken: fcTL and fdAT (libpng uses them), IDAT (an error).
    w = Writer(8, 8)
    w.actl(2)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 50, first=True)
    w.fctl(8, 8, 0, 0, crc=1234)
    w.data(8, 8, 51, crc=5678)
    w.save("hand-crc-fctl-fdat")
    w = Writer(8, 8)
    w.actl(2)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 52, first=True, crc=1)
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 53)
    w.save("hand-crc-idat")

    # Invalid fcTL: out of range, bad operations; before and after the image data.
    for name, before, args in [("hand-fctl-range-after", False, dict(w=9, h=8, x=0, y=0)), ("hand-fctl-dispose-after", False, dict(w=8, h=8, x=0, y=0, dispose=3)),
                               ("hand-fctl-blend-before", True, dict(w=8, h=8, x=0, y=0, blend=2)), ("hand-fctl-zero-before", True, dict(w=0, h=8, x=0, y=0))]:
        w = Writer(8, 8)
        w.actl(2)
        if before:
            w.fctl(args.pop("w"), args.pop("h"), args.pop("x"), args.pop("y"), **args)
            w.data(8, 8, 60, first=True)
        else:
            w.fctl(8, 8, 0, 0)
            w.data(8, 8, 60, first=True)
            w.fctl(args.pop("w"), args.pop("h"), args.pop("x"), args.pop("y"), **args)
            w.data(8, 8, 61)
        w.fctl(8, 8, 0, 0)
        w.data(8, 8, 62)
        w.save(name)

    # The first fcTL off the origin, of another size, of the wrong length: ignored, so the
    # default image is hidden (and its frame count one more).
    for name, args in [("hand-first-offset", dict(w=6, h=6, x=2, y=2)), ("hand-first-size", dict(w=6, h=6, x=0, y=0)), ("hand-first-length", dict(w=8, h=8, x=0, y=0, length=20))]:
        w = Writer(8, 8)
        w.actl(1)
        w.fctl(args.pop("w"), args.pop("h"), args.pop("x"), args.pop("y"), **args)
        w.data(8, 8, 70, first=True)
        w.fctl(8, 8, 0, 0, blend=1)
        w.data(8, 8, 71)
        w.save(name)

    # An fdAT before the image data, and acTL after it (both ignored).
    w = Writer(8, 8)
    w.actl(2)
    w.fctl(8, 8, 0, 0)
    w.parts.append(chunk("fdAT", struct.pack(">I", w.seq) + b"junk"))
    w.seq += 1
    w.data(8, 8, 80, first=True)
    w.raw("acTL", struct.pack(">II", 9, 9))
    w.fctl(8, 8, 0, 0)
    w.data(8, 8, 81)
    w.save("hand-fdat-before")

    # Files cut short, and without IEND.
    w = Writer(8, 8)
    w.actl(3)
    for i in range(3):
        w.fctl(8, 8, 0, 0, dispose=i % 3)
        w.data(8, 8, 90 + i, first=(i == 0))
    w.save("hand-no-iend", end=False)
    full = b"".join(w.parts) + chunk("IEND", b"")
    for cut in [len(full) * 3 // 4, len(full) // 2, 60]:
        w.save(f"hand-cut-{cut}", cut=cut)


pillow()
hand()
print(f"{len(list(OUT.glob('*.png')))} files in {OUT}")
