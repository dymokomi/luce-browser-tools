# Writes the Luce case table (html/tests_form_controls_cases.lucb) from the oracle's expected.txt.
# Usage: ./build.sh; ./oracle cases.txt > expected.txt; python3 gen_luce_cases.py > OUT.lucb
import sys
src = open('expected.txt', encoding='utf-8').read().split('\n')
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
            c = s[i + 1]; out.append('\n' if c == 'n' else '\t' if c == 't' else '\r' if c == 'r' else c); i += 2
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
        elif ch == '\r': r.append('\\r')
        elif ord(ch) < 0x20 or ord(ch) == 0x7f: r.append('\\u{%x}' % ord(ch))
        else: r.append(ch)
    return '"' + ''.join(r) + '"'

out = sys.stdout
out.write('''#==============================================================================================
#
#   tests_form_controls_cases - form control cases pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb on the same inputs (luce-browser-tools,
#       oracles/luce-browser-engine/form_controls: cases.txt, oracle.cpp, gen_luce_cases.py,
#       build.sh). tests_form_controls runs each case through the port and compares the output.
#
#==============================================================================================

## One oracle case: the mode, its arguments, and the reference build's output.
struct TestFormControlsCase:
    let mode: str
    let arguments: str[5]
    let expected: str

''')
out.write('## Every oracle case, in cases.txt order (%d more, marked "!" in cases.txt, wait for other regions).\n' % later)
out.write('let test_form_controls_cases: TestFormControlsCase[%d] = [\n' % len(cases))
for fields, lines in cases:
    mode = fields[0]
    args = [unescape(a) for a in fields[1:]] + [''] * (6 - len(fields))
    expected = ''.join(l + '\n' for l in lines)
    out.write('    TestFormControlsCase(mode = %s, arguments = [%s], expected = %s),\n' % (lit(mode), ', '.join(lit(a) for a in args[:5]), lit(expected)))
out.write(']\n')
