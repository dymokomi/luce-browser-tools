# Writes the Luce test data fragment for the scenes printed by ./driver (scenes.txt).
import sys
scenes = {}
order = []
cur = None
for line in open('scenes.txt'):
    line = line.strip()
    if line.startswith('scene '):
        cur = line[6:]
        scenes[cur] = []
        order.append(cur)
    elif line:
        scenes[cur].append(line.split())
out = ['''#==============================================================================================
#
#   tests_skia_reference_data_2 - more pixels DisplayListPlayerSkia renders for the player tests
#
#   DESCRIPTION:
#       Test vectors for tests_cpu_player_2.lucb (paths, glyph runs, layers, image filters,
#       backdrops): the premultiplied pixels (0xAARRGGBB per pixel, rows top to bottom) that
#       Ladybird's DisplayListPlayerSkia renders for the same display lists, recorded locally
#       from the reference build of Ladybird 47c82b38d0, as tests_skia_reference_data.lucb.
#
#==============================================================================================
''']
for name in order:
    rows = scenes[name]
    h = len(rows); w = len(rows[0])
    out.append(f'## Skia\'s pixels for the "{name}" scene ({w}x{h}).')
    out.append(f'let skia_{name}: str = """')
    for r in rows:
        out.append(' '.join(r))
    out.append('"""')
    out.append('')
open(sys.argv[1], 'w').write('\n'.join(out))
