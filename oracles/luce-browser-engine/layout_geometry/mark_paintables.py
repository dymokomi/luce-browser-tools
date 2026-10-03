# r50: marks with "!" the layout-mode cases of a cases file whose expected paint tree has a paintable that other
# regions make (MarkerPaintable, ImagePaintable, FieldSetPaintable, ...: region r53's stubs), so gen_luce_cases.py
# leaves them out until those regions land. Usage: python3 mark_paintables.py CASES EXPECTED
import re, sys
cases_file, expected_file = sys.argv[1], sys.argv[2]
waiting = re.compile(r'^ +(MarkerPaintable|ImagePaintable|FieldSetPaintable|CheckBoxPaintable|RadioButtonPaintable|SVG[A-Za-z]*Paintable|CanvasPaintable|VideoPaintable|AudioPaintable|NavigableContainerViewportPaintable) \(')
outs = []
for l in open(expected_file).read().split('\n'):
    if l.startswith('case ') or l.startswith('later '):
        outs.append(False)
    elif outs and waiting.match(l):
        outs[-1] = True
lines = open(cases_file).read().split('\n')
index = 0
marked = 0
for i, l in enumerate(lines):
    if not l or l.startswith('#'):
        continue
    if outs[index] and not l.startswith('!'):
        lines[i] = '!' + l
        marked += 1
    index += 1
assert index == len(outs)
open(cases_file, 'w').write('\n'.join(lines))
print('marked', marked)
