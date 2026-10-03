# Writes the Luce case table (tests_fonts_cases.lucb) from cases.txt and the oracle's expected.txt
# (./oracle > expected.txt), leaving out the cases marked "!". Usage: python3 gen_luce_cases.py OUT.lucb
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
lines = [l for l in open(os.path.join(HERE, 'cases.txt')).read().split('\n') if l and not l.startswith('#')]
expected = open(os.path.join(HERE, 'expected.txt')).read().rstrip('\n').split('\n')
assert len(lines) == len(expected)
def lit(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
rows = []
for line, result in zip(lines, expected):
    if line.startswith('!'):
        continue
    fields = line.split('\t') + ['', '']
    rows.append('    TestFontsCase(mode = %s, first = %s, second = %s, expected = %s),' % (lit(fields[0]), lit(fields[1]), lit(fields[2]), lit(result)))
out = open(sys.argv[1], 'w')
out.write('''#==============================================================================================
#
#   tests_fonts_cases - the CSS font cases (region r41) pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's FontComputer, FontFace, ParsedFontFace, FontFeatureData and
#       FontLoading on the same inputs, with the fonts of tests/libweb/fonts in layout test mode
#       (luce-browser-tools, oracles/luce-browser-engine/css_fonts: oracle.cpp, cases.txt,
#       gen_luce_cases.py). tests_fonts runs each case through the port and compares.
#
#==============================================================================================

## One oracle case: the mode, its arguments and the reference build's printout.
struct TestFontsCase:
    let mode: str
    let first: str
    let second: str
    let expected: str

## Every oracle case, in cases.txt order.
let test_fonts_cases: TestFontsCase[%d] = [
''' % len(rows))
out.write('\n'.join(rows) + '\n]\n')
