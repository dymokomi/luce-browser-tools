# Writes the Luce case table of region p2d (src/web/loader/tests_loader_cases.lucb) from the
# oracle's expected.txt: python3 gen_luce_cases.py > .../tests_loader_cases.lucb
import sys


def unescape(s):
    if s == '\\-':
        return ''
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            c = s[i + 1]
            if c == 'n':
                out.append('\n'); i += 2; continue
            if c == 't':
                out.append('\t'); i += 2; continue
            if c == '\\':
                out.append('\\'); i += 2; continue
            if c == 'x':
                out.append(chr(int(s[i + 2:i + 4], 16))); i += 4; continue
        out.append(s[i]); i += 1
    return ''.join(out)


def literal(text):
    r = []
    for ch in text:
        if ch == '\\':
            r.append('\\\\')
        elif ch == '"':
            r.append('\\"')
        elif ch == '\n':
            r.append('\\n')
        elif ch == '\t':
            r.append('\\t')
        elif ord(ch) < 0x20 or ord(ch) >= 0x7f:
            r.append('\\u{%x}' % ord(ch))
        else:
            r.append(ch)
    return '"' + ''.join(r) + '"'


cases = []
for line in open('expected.txt', encoding='utf-8').read().split('\n'):
    if not line:
        continue
    fields = line.split('\t')
    assert len(fields) == 5, line
    cases.append([unescape(f) for f in fields])

out = sys.stdout
out.write('''#==============================================================================================
#
#   tests_loader_cases - region p2d's results pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's AsciiStringMatcher and ContentFilter, ProxyMappings,
#       create_potential_CORS_request, translate_a_preload_destination, Traits<PreloadKey>,
#       Core::guess_mime_type_based_on_filename, Requests::network_error_to_string, the user
#       agent, the Last-Modified format and load_error_page on the same inputs
#       (luce-browser-tools, oracles/luce-browser-engine/loader_resources: oracle.cpp,
#       gen_luce_cases.py). Each case is an operation, three arguments and the oracle's result;
#       tests_loader_1 runs the operation and prints its result the way the oracle does. The
#       user agent and platform cases are macOS arm64's (the oracle's target).
#
#==============================================================================================

## One case: the operation, its arguments and the expected result.
struct LoaderCase:
    var op: str
    var a: str
    var b: str
    var c: str
    var expected: str

''')
out.write('## The cases (generated).\n')
out.write('let loader_cases: LoaderCase[%d] = [\n' % len(cases))
for op, a, b, c, expected in cases:
    out.write('    LoaderCase(op = %s, a = %s, b = %s, c = %s, expected = %s),\n' % (literal(op), literal(a), literal(b), literal(c), literal(expected)))
out.write(']\n')
