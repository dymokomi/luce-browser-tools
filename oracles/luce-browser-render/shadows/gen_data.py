# Writes luce-browser-render's tests_skia_reference_data_7.lucb from the scenes ./oracle prints:
#   ./oracle > scenes.txt && python3 gen_data.py scenes.txt <render>/src/display_list/tests_skia_reference_data_7.lucb
import sys
scenes = {}
order = []
cur = None
for line in open(sys.argv[1]):
    line = line.strip()
    if line.startswith('scene '):
        cur = line[6:]
        scenes[cur] = []
        order.append(cur)
    elif line and cur is not None and all(len(w) == 8 for w in line.split()):
        scenes[cur].append(line.split())
out = ['''#==============================================================================================
#
#   tests_skia_reference_data_7 - the box shadows DisplayListPlayerSkia renders
#
#   DESCRIPTION:
#       Test vectors for tests_cpu_shadow_skia.lucb: the premultiplied pixels (0xAARRGGBB per
#       pixel, rows top to bottom) that Ladybird's DisplayListPlayerSkia renders for display
#       lists painting outer and inner box shadows (the blur mask filter),
#       recorded locally from the reference build of Ladybird 47c82b38d0 (Skia m144) by
#       luce-browser-tools' oracles/luce-browser-render/shadows.
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
open(sys.argv[2], 'w').write('\n'.join(out))
