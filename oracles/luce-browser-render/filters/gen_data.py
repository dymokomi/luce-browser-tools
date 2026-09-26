# Turns the oracle's output into Luce test-data fragments (tests_cpu_filter_data_N.lucb).
# Pixels are 0xAARRGGBB hex words; "zN" stands for N transparent pixels.
import sys
out_dir = sys.argv[1]
scenes = []
cur = None
for line in open(sys.argv[2] if len(sys.argv) > 2 else 'out.txt'):
    parts = line.split()
    if not parts: continue
    if parts[0] == 'scene':
        cur = [parts[1], int(parts[2]), int(parts[3]), []]
        scenes.append(cur)
    elif cur and parts[0] not in ('srgb', 'inv_srgb'):
        cur[3].extend(parts)
def encode(px):
    toks = []
    zeros = 0
    for p in px:
        if p == '00000000':
            zeros += 1
            continue
        if zeros:
            toks.append('z%d' % zeros); zeros = 0
        toks.append(p)
    if zeros: toks.append('z%d' % zeros)
    lines, cur = [], ''
    for t in toks:
        if len(cur) + len(t) + 1 > 96:
            lines.append(cur); cur = t
        else:
            cur = (cur + ' ' + t) if cur else t
    if cur: lines.append(cur)
    return lines
header = '''#==============================================================================================
#
#   tests_cpu_filter_data_%d - Skia's pixels for the image filter tests (part %d)
#
#   DESCRIPTION:
#       Test vectors for tests_cpu_filter*.lucb: for each scene, the premultiplied pixels
#       (0xAARRGGBB words, rows top to bottom; "zN" is N transparent pixels) that Skia m144's
#       raster SkCanvas produces when the donor's Gfx::Filter graph filters a saveLayer (or a
#       backdrop), printed locally by a program over the reference build of Ladybird 47c82b38d0.
#
#==============================================================================================
'''
chunks, size, part = [], 0, []
for s in scenes:
    body = encode(s[3])
    part.append((s, body)); size += len(body) + 4
    if size > 520:
        chunks.append(part); part, size = [], 0
if part: chunks.append(part)
for n, part in enumerate(chunks, 1):
    out = [header % (n, n)]
    for s, body in part:
        out.append('## Skia\'s pixels for the "%s" scene (%dx%d).' % (s[0], s[1], s[2]))
        out.append('let filter_skia_%s: str = """' % s[0])
        out.extend(body)
        out.append('"""')
        out.append('')
    open('%s/tests_cpu_filter_data_%d.lucb' % (out_dir, n), 'w').write('\n'.join(out))
print(len(chunks), 'fragments', [sum(len(b)+4 for _, b in p) for p in chunks])
