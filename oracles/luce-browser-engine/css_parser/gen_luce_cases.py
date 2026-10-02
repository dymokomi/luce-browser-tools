# Writes the Luce case table (tests_css_parser_cases.lucb) from the oracle's expected.txt.
import sys
src = open('expected.txt', encoding='utf-8').read().split('\n')
cases = []
for line in src:
    if line.startswith('case '):
        mode, escaped = line[5:].split('\t', 1)
        cases.append([mode, escaped, []])
    elif line:
        cases[-1][2].append(line)

def unescape_case(s):
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
out.write('''#==============================================================================================
#
#   tests_css_parser_cases - the CSS parser's cases pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's CSS::Parser::Parser on the same inputs (luce-browser-tools,
#       oracles/luce-browser-engine/css_parser: cases.txt, oracle.cpp, gen_luce_cases.py).
#       tests_css_parser runs each case through the port and compares the dumps.
#
#==============================================================================================

## One oracle case: what to run, on what input, and the reference build's dump.
struct TestCssParserCase:
    let mode: str
    let input: str
    let expected: str

''')
out.write('## Every oracle case, in cases.txt order.\n')
out.write('let test_css_parser_cases: TestCssParserCase[%d] = [\n' % len(cases))
for mode, escaped, lines in cases:
    expected = ''.join(l + '\n' for l in lines)
    out.write('    TestCssParserCase(mode = %s, input = %s, expected = %s),\n' % (lit(mode), lit(unescape_case(escaped)), lit(expected)))
out.write(']\n')
