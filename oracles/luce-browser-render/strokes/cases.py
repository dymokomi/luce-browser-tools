#!/usr/bin/env python3
# cases.py SVG_COMMANDS > cases.txt: the stroke commands of raster's tests_stroke_skia: the
# StrokePath commands of Screenshot/input/svg-stroke-styles.html (as the CPU player logged them),
# targeted cases (zero-length segments and contours with each cap, closed rectangles with
# each join and a miter limit under sqrt(2), dashed lines with butt caps (SpecialLineRec),
# dashed rectangles (cull_path and the initial join), a path with a trailing move) and
# random paths with curves, joins, caps, miter limits, dashes and transforms.
import random, struct, sys, math

def b(x):
    return '%08x' % struct.unpack('<I', struct.pack('<f', x))[0]

def cmd(w, cap, join, miter, path, dash=(), off=0.0, ctm=(1, 0, 0, 0, 1, 0)):
    return ' '.join(['stroke', b(w), str(cap), str(join), b(miter), 'true', b(off), str(len(dash))] + [b(d) for d in dash] +
                    ['ff000000', '|'] + [b(v) for v in ctm] + ['|'] + path)

def p(*xy):
    return [b(v) for v in xy]

out = [l.rstrip('\n') for l in open(sys.argv[1]) if l.startswith('stroke')]
# raster.LineJoin: 0 miter, 2 round, 3 bevel; LineCap: 0 butt, 1 round, 2 square.
for cap in (0, 1, 2):
    out.append(cmd(8, cap, 0, 4, ['0'] + p(20, 20) + ['1'] + p(20, 20)))
    out.append(cmd(8, cap, 2, 4, ['0'] + p(20, 20) + ['4']))
    out.append(cmd(6, cap, 0, 4, ['0'] + p(10, 10) + ['1'] + p(10, 10) + ['1'] + p(40, 25) + ['1'] + p(40, 25)))
    out.append(cmd(6, cap, 3, 4, ['0'] + p(10, 10) + ['2'] + p(10, 10, 10, 10) + ['3'] + p(30, 5, 40, 30, 50, 10)))
rect = ['0'] + p(10, 10) + ['1'] + p(60, 10) + ['1'] + p(60, 40) + ['1'] + p(10, 40) + ['4']
ccw = ['0'] + p(10, 10) + ['1'] + p(10, 40) + ['1'] + p(60, 40) + ['1'] + p(60, 10) + ['4']
for join in (0, 2, 3):
    out.append(cmd(8, 0, join, 4, rect))
    out.append(cmd(8, 0, join, 4, ccw))
    out.append(cmd(40, 0, join, 4, rect))
out.append(cmd(8, 0, 0, 1.3, rect))
out.append(cmd(3, 0, 0, 4, ['0'] + p(5, 30) + ['1'] + p(195, 30), dash=(7, 3), off=2))
out.append(cmd(3, 0, 0, 4, ['0'] + p(5, 30) + ['1'] + p(150, 140), dash=(5, 2, 1, 2), off=-3))
out.append(cmd(3, 1, 0, 4, ['0'] + p(5, 30) + ['1'] + p(195, 30), dash=(7, 3), off=2))
out.append(cmd(2, 0, 0, 4, rect, dash=(4, 3), off=1))
out.append(cmd(2, 2, 2, 4, ccw, dash=(4, 3), off=5))
out.append(cmd(2, 0, 0, 4, ['0'] + p(-500, 30) + ['1'] + p(700, 30), dash=(6, 4), off=0, ctm=(1, 0, 0, 0, 1, 0)))
out.append(cmd(2, 0, 0, 4, rect + ['0'] + p(10, 10), dash=(3, 3), off=1.5))
rnd = random.Random(7)
for i in range(60):
    w = rnd.choice([1.0, 2.0, 3.5, 6.0, 10.0, 17.0])
    cap, join = rnd.randint(0, 2), rnd.choice([0, 2, 3])
    miter = rnd.choice([4.0, 1.0, 1.2, 10.0])
    dash, off = (), 0.0
    if rnd.random() < 0.3:
        dash = tuple(rnd.choice([1.0, 2.0, 5.0, 7.5]) for _ in range(rnd.choice([2, 4])))
        off = rnd.choice([0.0, 1.0, -2.5, 13.0])
    if rnd.random() < 0.5:
        ctm = (1, 0, 0, 0, 1, 0)
    else:
        a = rnd.uniform(-1, 1)
        s = rnd.choice([0.5, 1.5, 2.0])
        ctm = (s * math.cos(a), -s * math.sin(a), 30, s * math.sin(a), s * math.cos(a), 20)
    path = []
    def pt():
        return p(rnd.uniform(5, 195), rnd.uniform(5, 195))
    path += ['0'] + pt()
    for s in range(rnd.randint(1, 3)):
        v = rnd.choice([1, 2, 3, 5])
        if v == 1: path += ['1'] + pt()
        elif v == 2: path += ['2'] + pt() + pt()
        elif v == 3: path += ['3'] + pt() + pt() + pt()
        else: path += ['5'] + pt() + pt() + [b(rnd.choice([0.5, 0.7071067811865476, 2.0]))]
    if rnd.random() < 0.4:
        path.append('4')
    out.append(cmd(w, cap, join, miter, path, dash, off, ctm))
print('\n'.join(out))
