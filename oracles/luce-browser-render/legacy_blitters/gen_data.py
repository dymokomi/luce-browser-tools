#!/usr/bin/env python3
# Writes luce-browser-render's src/raster/tests_legacy_blitter_data.lucb from the scenes
# ./oracle prints: ./oracle > scenes.txt && python3 gen_data.py scenes.txt OUT
import sys
scenes, order, cur = {}, [], None
for line in open(sys.argv[1]):
    line = line.strip()
    if line.startswith('scene '):
        cur = line[6:]
        scenes[cur] = []
        order.append(cur)
    elif line and cur is not None:
        scenes[cur].append(line)
out = ['''#==============================================================================================
#
#   tests_legacy_blitter_data - pixels Skia m144's legacy kN32 blitters draw
#
#   DESCRIPTION:
#       Test vectors for tests_legacy_blitter.lucb: the premultiplied pixels (0xAARRGGBB per
#       pixel, rows top to bottom) Skia m144 draws into a kN32 sRGB raster surface for the
#       scenes of luce-browser-tools' oracles/luce-browser-render/legacy_blitters, recorded
#       locally from the libskia of the reference build of Ladybird 47c82b38d0.
#
#==============================================================================================
''']
for name in order:
    out.append(f'## Skia\'s pixels for the "{name}" scene (24x24).')
    out.append(f'let legacy_{name}: str = """')
    out.extend(scenes[name])
    out.append('"""')
    out.append('')
open(sys.argv[2], 'w').write('\n'.join(out))
