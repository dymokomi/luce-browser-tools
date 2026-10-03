# Writes the Luce case table (layout/tests_layout_geometry_cases.lucb) from the oracle's expected.txt.
# Usage: ./oracle cases.txt > expected.txt; python3 gen_luce_cases.py > OUT.lucb
#        ./oracle cases_inline.txt > expected_inline.txt; python3 gen_luce_cases.py inline > .../tests_layout_inline_cases.lucb
#        ./oracle cases_paint.txt > expected_paint.txt; python3 gen_luce_cases.py paint > .../painting/tests_paintables_cases.lucb
import sys
kind = sys.argv[1] if len(sys.argv) > 1 else ''
inline = kind == 'inline'
paint = kind == 'paint'
src = open('expected_inline.txt' if inline else 'expected_paint.txt' if paint else 'expected.txt', encoding='utf-8').read().split('\n')
cases = []
later = 0
skipping = False
for line in src:
    if line.startswith('case '):
        cases.append([line[5:].split('\t'), []])
        skipping = False
    elif line.startswith('later '):
        later += 1
        skipping = True
    elif line and cases and not skipping:
        cases[-1][1].append(line)

def unescape(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            c = s[i + 1]; out.append('\n' if c == 'n' else '\t' if c == 't' else c); i += 2
        else:
            out.append(s[i]); i += 1
    return ''.join(out)

def lit(s):
    r = []
    for ch in s:
        if ch == '\\': r.append('\\\\')
        elif ch == '"': r.append('\\"')
        elif ch == '\n': r.append('\\n')
        elif ch == '\t': r.append('\\t')
        elif ord(ch) < 0x20 or ord(ch) == 0x7f: r.append('\\u{%x}' % ord(ch))
        else: r.append(ch)
    return '"' + ''.join(r) + '"'

out = sys.stdout
if paint:
    out.write('''#==============================================================================================
#
#   tests_paintables_cases - queries and hit tests of paint trees pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same documents (luce-browser-tools,
#       oracles/luce-browser-engine/layout_geometry: cases_paint.txt, own_paint_cases.py,
#       oracle.cpp, gen_luce_cases.py, build.sh). tests_paintables runs each case through the
#       port and compares the output.
#
#==============================================================================================

''')
    table = 'test_paintables_cases'
elif inline:
    out.write('''#==============================================================================================
#
#   tests_layout_inline_cases - laid-out layout trees with text (inline layout) pinned to the
#                               reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same documents (luce-browser-tools,
#       oracles/luce-browser-engine/layout_geometry: cases_inline.txt, oracle.cpp,
#       gen_luce_cases.py, build.sh). tests_layout_inline runs each case through the port and
#       compares the output.
#
#==============================================================================================

''')
    table = 'test_layout_inline_cases'
else:
    out.write('''#==============================================================================================
#
#   tests_layout_geometry_cases - laid-out layout trees (block layout geometry) pinned to the
#                                 reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same documents (luce-browser-tools,
#       oracles/luce-browser-engine/layout_geometry: cases.txt, oracle.cpp, gen_luce_cases.py,
#       build.sh). tests_layout_geometry runs each case through the port and compares the output.
#
#==============================================================================================

## One oracle case: the mode, the document, its author style sheet, and the reference build's output.
struct TestLayoutGeometryCase:
    let mode: str
    let html: str
    let css: str
    let expected: str

''')
    table = 'test_layout_geometry_cases'
cases_file = 'cases_inline.txt' if inline else 'cases_paint.txt' if paint else 'cases.txt'
out.write('## Every oracle case, in %s order (%d more, marked "!" in %s, wait for other regions).\n' % (cases_file, later, cases_file))
out.write('let %s: TestLayoutGeometryCase[%d] = [\n' % (table, len(cases)))
for args, lines in cases:
    mode = args[0]
    html = unescape(args[1])
    css = unescape(args[2]) if len(args) > 2 else ''
    expected = ''.join(l + '\n' for l in lines)
    out.write('    TestLayoutGeometryCase(mode = %s, html = %s, css = %s, expected = %s),\n' % (lit(mode), lit(html), lit(css), lit(expected)))
out.write(']\n')
