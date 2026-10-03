# Writes the Luce case table (tests_selector_cases.lucb) from the selector oracle's output
# (selector_expected.txt, written by `./oracle > selector_expected.txt` after ./build.sh).
import sys
src = open('selector_expected.txt', encoding='utf-8').read().split('\n')
cases = []
for line in src:
    if line.startswith('case '):
        mode, text = line[5:].split('\t', 1)
        cases.append([mode, text, []])
    elif line:
        cases[-1][2].append(line)

def lit(s):
    r = []
    for ch in s:
        if ch == '\\': r.append('\\\\')
        elif ch == '"': r.append('\\"')
        elif ch == '\n': r.append('\\n')
        elif ch == '\t': r.append('\\t')
        elif ch == '{': r.append('{')
        elif ord(ch) < 0x20 or ord(ch) == 0x7f: r.append('\\u{%x}' % ord(ch))
        else: r.append(ch)
    return '"' + ''.join(r) + '"'

out = sys.stdout
out.write('''#==============================================================================================
#
#   tests_selector_cases - the selector cases pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's selector parser and CSS::Selector / PageSelector members on
#       the same inputs (luce-browser-tools, oracles/luce-browser-engine/selector_engine:
#       selector_cases.txt, selectors.cpp, gen_selector_cases.py). tests_selector runs each
#       case through the port and compares the output.
#
#==============================================================================================

## One oracle case: what to run, on what input, and the reference build's output.
struct TestSelectorCase:
    let mode: str
    let input: str
    let expected: str

''')
out.write('## Every oracle case, in selector_cases.txt order.\n')
out.write('let test_selector_cases: TestSelectorCase[%d] = [\n' % len(cases))
for mode, text, lines in cases:
    expected = ''.join(l + '\n' for l in lines)
    out.write('    TestSelectorCase(mode = %s, input = %s, expected = %s),\n' % (lit(mode), lit(text), lit(expected)))
out.write(']\n')
