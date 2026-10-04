# Writes the Luce case table (fetch/infrastructure/http/tests_fetch_http_cases.lucb) from the
# oracle's expected.txt: python3 gen_luce_cases.py > .../tests_fetch_http_cases.lucb
import sys

lines = open('expected.txt', encoding='latin-1').read().split('\n')


def unescape(s):
    if s == '\\-':
        return b''
    out, i = bytearray(), 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            c = s[i + 1]
            if c == 'n':
                out.append(10); i += 2; continue
            if c == 't':
                out.append(9); i += 2; continue
            if c == '\\':
                out.append(92); i += 2; continue
            if c == 'x' and i + 3 < len(s):
                out.append(int(s[i + 2:i + 4], 16)); i += 4; continue
        out.append(ord(s[i])); i += 1
    return bytes(out)


def blit(data):
    r = []
    for b in data:
        if b == 0x5c:
            r.append('\\\\')
        elif b == 0x22:
            r.append('\\"')
        elif b < 0x20 or b >= 0x7f:
            r.append('\\x%02X' % b)
        else:
            r.append(chr(b))
    return 'b"' + ''.join(r) + '"'


cases = []
i = 0
while i < len(lines):
    line = lines[i]
    if not line:
        i += 1
        continue
    fields = line.split('\t')
    expected = lines[i + 1]
    assert expected.startswith('= '), expected
    args = [unescape(f) for f in fields[1:]]
    cases.append((fields[0], args, expected[2:].encode('latin-1')))
    i += 2

out = sys.stdout
out.write('''#==============================================================================================
#
#   tests_fetch_http_cases - LibHTTP's and Fetch/Infrastructure/HTTP's results pinned to the
#   reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibHTTP (Header, HeaderList, Method, Status) and
#       Fetch/Infrastructure/HTTP (CORS, MIME, Statuses) on the same inputs
#       (luce-browser-tools, oracles/luce-browser-engine/fetch_http: cases.txt, oracle.cpp,
#       gen_luce_cases.py). Each case is an operation, its arguments separated by 0x1F, and the
#       oracle's result line; tests_fetch_http_1 runs the operation and prints its result the
#       way the oracle does.
#
#==============================================================================================

''')
out.write('## The cases (generated).\n')
out.write('let fetch_http_cases: FetchHttpCase[%d] = [\n' % len(cases))
for op, args, expected in cases:
    out.write('    FetchHttpCase(op = "%s", argument_count = %d, arguments = %s, expected = %s),\n' % (op, len(args), blit(b'\x1f'.join(args)), blit(expected)))
out.write(']\n')
