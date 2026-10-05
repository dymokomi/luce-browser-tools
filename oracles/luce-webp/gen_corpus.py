#!/usr/bin/env python3
"""Synthesize WebP test files for luce-webp with libwebp 1.6.0's tools (cwebp, img2webp,
gif2webp, webpmux; Homebrew's build of the same version Ladybird links).

    gen_corpus.py OUTDIR [--small]

Sources are made with Pillow (gradients, noise, shapes, few-color images, alpha ramps) and
taken from Pillow's public-domain "hopper" photo and its GIF test images. The files cover
lossy (qualities, methods, segments, spatial noise shaping, filter strengths and sharpness,
the simple and complex loop filters, partitions), alpha (raw and lossless ALPH, every filter,
alpha quality), lossless (every effort level, near-lossless, palettes of 2 to 256 colors for
pixel bundling, the color cache), odd sizes from 1x1 up, ICC/EXIF/XMP chunks, and
animations (img2webp lossy, lossless and mixed with key frame spacing and durations; gif2webp
from GIFs with offsets, disposal and transparency), plus damaged copies of some (truncated,
bit flips). `--small` writes the subset luce-webp commits as its fixtures.
"""
import os, random, shutil, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image, ImageDraw

PILLOW = Path("/Users/sedov/Dev/luce_dev/.donors/pillow-tests/Tests/images")
ICC = Path("/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibGfx/test-inputs/icc/p3-v4.icc")


def run(*args):
    subprocess.run([str(a) for a in args], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def photo(width, height, seed):
    """A photo-like picture: gradients, shapes and noise."""
    rng = random.Random(seed)
    image = Image.new("RGB", (width, height))
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            pixels[x, y] = ((x * 255) // max(1, width - 1), (y * 255) // max(1, height - 1), ((x + y) * 7) % 256)
    draw = ImageDraw.Draw(image)
    for _ in range(12):
        x0, y0 = rng.randrange(width), rng.randrange(height)
        x1, y1 = x0 + rng.randrange(1, width + 1), y0 + rng.randrange(1, height + 1)
        color = tuple(rng.randrange(256) for _ in range(3))
        if rng.random() < 0.5:
            draw.ellipse((x0, y0, x1, y1), fill=color)
        else:
            draw.rectangle((x0, y0, x1, y1), outline=color, width=rng.randrange(1, 4))
    for _ in range(width * height // 8):
        x, y = rng.randrange(width), rng.randrange(height)
        r, g, b = pixels[x, y]
        n = rng.randrange(-40, 41)
        pixels[x, y] = (max(0, min(255, r + n)), max(0, min(255, g + n)), max(0, min(255, b + n)))
    return image


def with_alpha(image, kind):
    """The picture with an alpha channel: a ramp, a hard mask, or semi-transparent noise."""
    rgba = image.convert("RGBA")
    w, h = rgba.size
    alpha = Image.new("L", (w, h))
    a = alpha.load()
    rng = random.Random(len(kind) + w * h)
    for y in range(h):
        for x in range(w):
            if kind == "ramp":
                a[x, y] = (x * 255) // max(1, w - 1)
            elif kind == "mask":
                a[x, y] = 255 if ((x // 5) + (y // 7)) % 2 else 0
            else:
                a[x, y] = rng.choice([0, 64, 128, 200, 255])
    rgba.putalpha(alpha)
    return rgba


def few_colors(width, height, count, seed):
    """A picture of `count` colors (some translucent), for palettes and pixel bundling."""
    rng = random.Random(seed)
    palette = [tuple(rng.randrange(256) for _ in range(3)) + (rng.choice([255, 255, 128, 0]),) for _ in range(count)]
    image = Image.new("RGBA", (width, height))
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            pixels[x, y] = palette[((x // 3) * 7 + (y // 2) * 3 + rng.randrange(2)) % count]
    return image


def main():
    out = Path(sys.argv[1])
    small = "--small" in sys.argv
    out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="webp-corpus-"))
    made = []

    def cwebp(source, name, *options):
        target = out / f"{name}.webp"
        run("cwebp", "-quiet", *options, source, "-o", target)
        made.append(target)
        return target

    # Sources.
    big = work / "photo.png"
    photo(203, 157, 1).save(big)
    hopper = work / "hopper.png"
    Image.open(PILLOW / "hopper.bmp").convert("RGB").save(hopper)
    alphas = {}
    for kind in ["ramp", "mask", "noise"]:
        alphas[kind] = work / f"alpha-{kind}.png"
        with_alpha(photo(97, 61, 2), kind).save(alphas[kind])
    gray = work / "gray.png"
    photo(64, 48, 3).convert("L").save(gray)
    icc = work / "icc.png"
    photo(40, 30, 4).save(icc, icc_profile=ICC.read_bytes())

    # Lossy: rate control, segments, noise shaping, filters, partitions.
    lossy = [("q75", []), ("q0", ["-q", "0"]), ("q30-m0", ["-q", "30", "-m", "0"]), ("q100-m6", ["-q", "100", "-m", "6"]),
             ("seg1", ["-segments", "1"]), ("seg2-sns0", ["-segments", "2", "-sns", "0"]), ("sns100", ["-sns", "100"]),
             ("f0", ["-f", "0"]), ("f100-sharp7", ["-f", "100", "-sharpness", "7"]), ("sharp3", ["-sharpness", "3"]),
             ("simple", ["-nostrong"]), ("simple-f100", ["-nostrong", "-f", "100", "-q", "20"]), ("af", ["-af"]),
             ("pass6", ["-pass", "6", "-size", "4000"]), ("partlimit", ["-partition_limit", "100", "-q", "95"]),
             ("pre", ["-pre", "2"]), ("sharpyuv", ["-sharp_yuv"]), ("photo-preset", ["-preset", "photo"]),
             ("text-preset", ["-preset", "text"]), ("icon-preset", ["-preset", "icon"])]
    for name, options in (lossy[:8] if small else lossy):
        cwebp(big, f"lossy-{name}", *options)
    if not small:
        cwebp(hopper, "lossy-hopper")
        cwebp(hopper, "lossy-hopper-q10-simple", "-q", "10", "-nostrong")
        cwebp(gray, "lossy-gray")
        # Token partitions (the encoder writes 1, 2, 4 or 8 with -partitions only in the API;
        # partition_limit and size targets exercise partition 0's limits).

    # Odd sizes, lossy and lossless.
    sizes = [(1, 1), (2, 2), (3, 7), (15, 15), (16, 16), (17, 17), (33, 65), (255, 3), (1, 129)]
    for w, h in (sizes[:5] if small else sizes):
        source = work / f"size-{w}x{h}.png"
        photo(w, h, w * 1000 + h).save(source)
        cwebp(source, f"lossy-{w}x{h}", "-q", "60")
        cwebp(source, f"lossless-{w}x{h}", "-lossless")
        alpha_source = work / f"size-{w}x{h}-alpha.png"
        with_alpha(photo(w, h, w + h), "noise").save(alpha_source)
        cwebp(alpha_source, f"alpha-{w}x{h}")

    # Alpha: raw and lossless ALPH, its filters and quality.
    for kind, source in alphas.items():
        for method in ["0", "1"]:
            for filt in (["fast"] if small else ["none", "fast", "best"]):
                cwebp(source, f"alpha-{kind}-m{method}-{filt}", "-alpha_method", method, "-alpha_filter", filt)
        if not small:
            cwebp(source, f"alpha-{kind}-q20", "-alpha_q", "20")
            cwebp(source, f"alpha-{kind}-exact", "-exact")
            cwebp(source, f"alpha-{kind}-lossless", "-lossless")
            cwebp(source, f"alpha-{kind}-lossless-exact", "-lossless", "-exact", "-z", "9")

    # Lossless: effort levels, near-lossless, palettes.
    for z in (["0", "6", "9"] if small else [str(z) for z in range(10)]):
        cwebp(big, f"lossless-z{z}", "-lossless", "-z", z)
    for near in ([] if small else ["0", "40", "80"]):
        cwebp(big, f"near-lossless-{near}", "-near_lossless", near)
    for count in [2, 3, 4, 5, 15, 16, 17, 64, 256]:
        source = work / f"colors-{count}.png"
        few_colors(37 if count % 2 else 40, 23, count, count).save(source)
        if small and count not in (2, 3, 16, 17):
            continue
        cwebp(source, f"palette-{count}", "-lossless")
        cwebp(source, f"palette-{count}-z9", "-lossless", "-z", "9")
        cwebp(source, f"palette-{count}-alpha-lossy", "-q", "50")
    if not small:
        cwebp(hopper, "lossless-hopper", "-lossless")
        cwebp(gray, "lossless-gray", "-lossless", "-z", "9")

    # Metadata chunks.
    cwebp(icc, "icc-lossy", "-metadata", "icc")
    cwebp(icc, "icc-lossless", "-lossless", "-metadata", "icc")
    exif = work / "exif.bin"
    exif.write_bytes(b"Exif\x00\x00MM\x00\x2a\x00\x00\x00\x08\x00\x00")
    xmp = work / "xmp.xml"
    xmp.write_bytes(b"<x:xmpmeta xmlns:x='adobe:ns:meta/'/>")
    base = cwebp(alphas["ramp"], "meta-base")
    target = out / "meta-exif-xmp.webp"
    run("webpmux", "-set", "exif", exif, base, "-o", work / "with-exif.webp")
    run("webpmux", "-set", "xmp", xmp, work / "with-exif.webp", "-o", target)
    made.append(target)
    target = out / "meta-icc-set.webp"
    run("webpmux", "-set", "icc", ICC, base, "-o", target)
    made.append(target)

    # Animations: img2webp from frames of the same size.
    frames = []
    for i in range(5):
        frame = work / f"frame-{i}.png"
        image = photo(48, 36, 50 + i) if i % 2 == 0 else with_alpha(photo(48, 36, 50 + i), ["mask", "noise", "ramp"][i % 3])
        image.save(frame)
        frames.append(frame)
    animations = [("anim-lossless", ["-loop", "3"], []), ("anim-lossy", ["-loop", "0"], ["-lossy", "-q", "50"]),
                  ("anim-mixed", ["-mixed", "-kmin", "1", "-kmax", "2"], []), ("anim-keyframes", ["-kmax", "0"], []),
                  ("anim-nokeyframes", ["-kmin", "0", "-kmax", "0", "-min_size"], [])]
    for name, file_options, frame_options in (animations[:3] if small else animations):
        target = out / f"{name}.webp"
        args = ["img2webp", *file_options]
        for i, frame in enumerate(frames):
            args += ["-d", str(20 * (i + 1)), *frame_options, frame]
        run(*args, "-o", target)
        made.append(target)
    # gif2webp: offsets, disposal, transparency.
    gifs = ["dispose_bgnd.gif", "dispose_none.gif", "dispose_prev.gif", "transparent_dispose.gif", "dispose_bgnd_transparency.gif", "test_extents.gif", "first_frame_transparency.gif", "iss634.gif", "different_transparency.gif", "chi.gif"]
    for gif in (gifs[:4] if small else gifs):
        for mode in ([[]] if small else [[], ["-lossy"], ["-mixed"]]):
            target = out / f"gif-{Path(gif).stem}{'-' + mode[0][1:] if mode else ''}.webp"
            try:
                run("gif2webp", "-quiet", *mode, PILLOW / gif, "-o", target)
                made.append(target)
            except subprocess.CalledProcessError:
                pass

    # Damaged copies: cut short and with flipped bits.
    rng = random.Random(7)
    sources = [f for f in made if f.stat().st_size > 64]
    for source in (sources[::6] if small else sources[::2]):
        data = source.read_bytes()
        for cut in [len(data) // 2, len(data) - 1]:
            (out / f"damaged-cut{cut}-{source.name}").write_bytes(data[:cut])
        for n in range(1 if small else 3):
            changed = bytearray(data)
            for _ in range(1 + n):
                at = rng.randrange(12, len(changed))
                changed[at] ^= 1 << rng.randrange(8)
            (out / f"damaged-flip{n}-{source.name}").write_bytes(bytes(changed))
    shutil.rmtree(work)
    print(f"{len(list(out.glob('*.webp')))} files in {out}")


main()
