# Writes the Luce case table (svg/tests_svg_cases.lucb) from the oracle's expected.txt.
# Usage: ./build.sh; ./oracle cases.txt > expected.txt; python3 gen_luce_cases.py > OUT.lucb
#        r56: ./oracle cases_r56.txt > expected_r56.txt; python3 gen_luce_cases.py r56 > .../web/svg/tests_svg_ii_cases.lucb
import sys
r56 = len(sys.argv) > 1 and sys.argv[1] == 'r56'
src = open('expected_r56.txt' if r56 else 'expected.txt', encoding='utf-8').read().split('\n')
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
if r56:
    out.write('''#==============================================================================================
#
#   tests_svg_ii_cases - SVG II elements (gradients, patterns, masks, clip paths, filter
#                        primitives, use, text, a, image, foreignObject) pinned to the reference
#                        build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same documents (luce-browser-tools,
#       oracles/luce-browser-engine/svg_elements: own_r56_cases.py, cases_r56.txt, oracle.cpp,
#       gen_luce_cases.py r56, build.sh). tests_svg_ii runs each case through the port and
#       compares the output; floats are their bits.
#
#==============================================================================================

## One oracle case: the mode, the document, and the reference build's output.
struct TestSvgIiCase:
    let mode: str
    let html: str
    let expected: str

''')
    out.write('## Every oracle case, in cases_r56.txt order.\n')
    out.write('let test_svg_ii_cases: TestSvgIiCase[%d] = [\n' % len(cases))
    for fields, lines in cases:
        expected = ''.join(l + '\n' for l in lines)
        out.write('    TestSvgIiCase(mode = %s, html = %s, expected = %s),\n' % (lit(fields[0]), lit(unescape(fields[1])), lit(expected)))
    out.write(']\n')
    sys.exit(0)
out.write('''#==============================================================================================
#
#   tests_svg_cases - SVG attribute parsing, paths and SVG element cases pinned to the
#                     reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same inputs (luce-browser-tools,
#       oracles/luce-browser-engine/svg_elements: cases.txt, oracle.cpp, gen_luce_cases.py,
#       build.sh). tests_svg runs each case through the port and compares the output; floats
#       are their bits.
#
#==============================================================================================

## One oracle case: the mode, its arguments, and the reference build's output.
struct TestSvgCase:
    let mode: str
    let arguments: str[5]
    let expected: str

''')
out.write('## Every oracle case, in cases.txt order (%d more, marked "!" in cases.txt, wait for other regions).\n' % later)
out.write('let test_svg_cases: TestSvgCase[%d] = [\n' % len(cases))
for fields, lines in cases:
    mode = fields[0]
    args = [unescape(a) for a in fields[1:]]
    args = args + [''] * (5 - len(args))
    expected = ''.join(l + '\n' for l in lines)
    out.write('    TestSvgCase(mode = %s, arguments = [%s], expected = %s),\n' % (lit(mode), ', '.join(lit(a) for a in args[:5]), lit(expected)))
out.write(']\n')
