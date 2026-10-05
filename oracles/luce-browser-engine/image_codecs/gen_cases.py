#!/usr/bin/env python3
"""Writes the synthetic images of region p2img's tests (luce-browser-engine
src/web/platform/tests_image_codec_plugin_luce.lucb holds the same bytes) to cases/, and prints
them as Luce array literals."""
import os, struct, zlib

HERE = os.path.dirname(os.path.abspath(__file__))

def gif_animation(loop):
    # A 2x1 canvas, a global palette of black, red, green, blue; six 1x1 frames.
    out = b"GIF89a" + struct.pack("<HHBBB", 2, 1, 0x81, 0, 0)
    out += bytes([0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255])
    if loop is not None:
        out += b"\x21\xFF\x0BNETSCAPE2.0\x03\x01" + struct.pack("<H", loop) + b"\x00"
    # (left, index, disposal, delay, transparent index)
    frames = [(0, 1, 1, 5, None), (1, 2, 2, 0, None), (0, 3, 3, 1, None), (1, 3, 1, 20, None), (0, 2, 1, 3, None), (1, 0, 1, 7, 0)]
    for left, index, disposal, delay, transparent in frames:
        packed = (disposal << 2) | (1 if transparent is not None else 0)
        out += b"\x21\xF9\x04" + struct.pack("<BHB", packed, delay, transparent or 0) + b"\x00"
        out += b"\x2C" + struct.pack("<HHHHB", left, 0, 1, 1, 0)
        code = 4 | (index << 3) | (5 << 6)  # clear, the index, end of information: 3-bit codes
        out += bytes([2, 2, code & 0xFF, code >> 8, 0])
    return out + b"\x3B"

RGBA_PNG = bytes.fromhex(
    "89504E470D0A1A0A0000000D49484452000000030000000108060000001BE014B40000001549444154789C633891"
    "62D4C02522F7FFFFFFFF0C0023AF0617F445F03C0000000049454E44AE426082")

def dib(width, height, bits, pixels, mask_rows):
    header = struct.pack("<IiiHHIIiiII", 40, width, height * 2, 1, bits, 0, 0, 0, 0, 0, 0)
    return header + pixels + mask_rows

def ico(entries):
    out = struct.pack("<HHH", 0, 1, len(entries))
    offset = 6 + 16 * len(entries)
    body = b""
    for width, height, bits, data in entries:
        out += struct.pack("<BBBBHHII", width, height, 0, 0, 1, bits, len(data), offset + len(body))
        body += data
    return out + body

def ico_png_and_bmp():
    # A 1x1 32-bit BMP entry (B 16, G 32, R 48, A 128) and the 3x1 RGBA PNG: the PNG is larger.
    bmp = dib(1, 1, 32, bytes([16, 32, 48, 128]), bytes(4))
    return ico([(1, 1, 32, bmp), (3, 1, 32, RGBA_PNG)])

def ico_bmp_entries():
    # 2x1 at 24 bits (red, green), 2x1 at 32 bits (blue, half-transparent white), 1x1 at 32 bits: the
    # second is chosen (largest, then most bits).
    rgb = dib(2, 1, 24, bytes([0, 0, 255, 0, 255, 0, 0, 0]), bytes(4))
    rgba = dib(2, 1, 32, bytes([255, 0, 0, 255, 255, 255, 255, 128]), bytes(4))
    small = dib(1, 1, 32, bytes([0, 0, 0, 255]), bytes(4))
    return ico([(2, 1, 24, rgb), (2, 1, 32, rgba), (1, 1, 32, small)])

def bmp_top_down():
    # 2x2, 24 bits, top-down (negative height): red, green / blue, white.
    rows = bytes([0, 0, 255, 0, 255, 0, 0, 0]) + bytes([255, 0, 0, 255, 255, 255, 0, 0])
    info = struct.pack("<IiiHHIIiiII", 40, 2, -2, 1, 24, 0, len(rows), 0, 0, 0, 0)
    return b"BM" + struct.pack("<IHHI", 14 + 40 + len(rows), 0, 0, 54) + info + rows

def bmp_png():
    # A BI_PNG bitmap holding the 3x1 RGBA PNG.
    info = struct.pack("<IiiHHIIiiII", 40, 3, 1, 1, 0, 5, len(RGBA_PNG), 0, 0, 0, 0)
    return b"BM" + struct.pack("<IHHI", 14 + 40 + len(RGBA_PNG), 0, 0, 54) + info + RGBA_PNG

CASES = {
    "animation-loop2.gif": gif_animation(2),
    "animation-loop0.gif": gif_animation(0),
    "animation-noloop.gif": gif_animation(None),
    "ico-png-and-bmp.ico": ico_png_and_bmp(),
    "ico-bmp-entries.ico": ico_bmp_entries(),
    "top-down.bmp": bmp_top_down(),
    "png-in.bmp": bmp_png(),
}

def literal(name, data):
    body = ", ".join("0x%02X" % b for b in data)
    return "let %s: u8[%d] = [%s]" % (name, len(data), body)

if __name__ == "__main__":
    os.makedirs(os.path.join(HERE, "cases"), exist_ok=True)
    for name, data in CASES.items():
        with open(os.path.join(HERE, "cases", name), "wb") as f:
            f.write(data)
        print(literal(name.replace("-", "_").replace(".", "_"), data))
