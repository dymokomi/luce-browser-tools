#!/usr/bin/env python3
# gen_data.py CASES PATHS OUT: luce-browser-render's src/raster/tests_stroke_skia_data*.lucb
# from cases.txt and the fill paths `./oracle cases.txt 200 200 /dev/null --path` prints.
#   python3 cases.py svg_commands.txt > cases.txt
#   ./oracle cases.txt 200 200 /dev/null --path > paths.txt
#   python3 gen_data.py cases.txt paths.txt <render>/src/raster/tests_stroke_skia_data
import sys
cases = [l.rstrip('\n') for l in open(sys.argv[1]) if l.startswith('stroke')]
paths = [l.strip() for l in open(sys.argv[2])]
assert len(cases) == len(paths)
# The random cases (after the 72 fixed ones) whose path is long are left out, to keep the data small.
keep = [i for i in range(len(cases)) if i < 72 or len(paths[i]) <= 6000]
cases = [cases[i] for i in keep]
paths = [paths[i] for i in keep]
header = '''#==============================================================================================
#
#   tests_stroke_skia_data{part} - the fill paths Skia m144 makes for strokes ({what})
#
#   DESCRIPTION:
#       Test vectors for tests_stroke_skia.lucb, recorded locally from the libskia of the
#       reference build of Ladybird 47c82b38d0 by luce-browser-tools'
#       oracles/luce-browser-render/strokes (skpathutils::FillPathWithPaint with the cull
#       rectangle of a 200x200 device, then the matrix). Each case is a stroke command
#       (width, cap, join, miter limit, anti-aliasing, dash offset and intervals, color,
#       matrix and path, every float as its bits in hex) and the path Skia fills (verbs with
#       their points' bits; 5 is a conic followed by its weight).
#
#==============================================================================================
'''
half = (len(cases) + 1) // 2
for part, (start, stop) in enumerate(((0, half), (half, len(cases))), 1):
    what = 'first half' if part == 1 else 'second half'
    out = [header.format(part=part, what=what)]
    names = []
    for i in range(start, stop):
        names.append(f'stroke_case_{i}')
        out.append(f'## Case {i}: the command, then Skia\'s path.')
        out.append(f'let stroke_case_{i}: str = """{cases[i]}\n{paths[i]}"""')
        out.append('')
    out.append(f'## The cases of this part, in order.')
    out.append(f'let stroke_cases_{part}: str[{len(names)}] = [{", ".join(names)}]')
    out.append('')
    open(f'{sys.argv[3]}_{part}.lucb', 'w').write('\n'.join(out))
