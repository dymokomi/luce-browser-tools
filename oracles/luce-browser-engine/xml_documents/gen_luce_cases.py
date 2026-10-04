# Writes the Luce case table (tests_xml_document_builder_cases.lucb) from the oracle's expected.txt.
# Usage: ./oracle cases.txt > expected.txt; python3 gen_luce_cases.py > OUT.lucb
# Parse error messages are libxml2's in the reference build and luce-xml's in the port: both sides
# write them as "*" (tests_xml_document_builder normalizes its own output the same way).
import sys
src = open('expected.txt', encoding='utf-8').read().split('\n')
cases = []
deviating = 0
skipping = False
for line in src:
    if line.startswith('case '):
        cases.append([line[5:].split('\t'), []])
        skipping = False
    elif line.startswith('deviates '):
        deviating += 1
        skipping = True
    elif line and cases and not skipping:
        cases[-1][1].append(line)

def normalize(line):
    if line.startswith('error '):
        return 'error *'
    marker = '"Failed to parse XML document: '
    at = line.find(marker)
    if at >= 0:
        return line[:at] + marker + '*"'
    if line.startswith('exception SyntaxError ') and line != 'exception SyntaxError Document element has sibling nodes':
        return 'exception SyntaxError *'
    return line

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
        elif ord(ch) < 0x20 or ord(ch) == 0x7f or ord(ch) == 0xfeff: r.append('\\u{%x}' % ord(ch))
        else: r.append(ch)
    return '"' + ''.join(r) + '"'

out = sys.stdout
out.write('''#==============================================================================================
#
#   tests_xml_document_builder_cases - region p2x's XML documents pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that runs the
#       reference Ladybird build's LibWeb (XMLDocumentBuilder over LibXML's libxml2 parser) on
#       the same inputs (luce-browser-tools, oracles/luce-browser-engine/xml_documents:
#       cases.txt, oracle.cpp, gen_luce_cases.py, build.sh). tests_xml_document_builder runs each
#       case through the port and compares. Parse error messages are written "*" (libxml2's and
#       luce-xml's words differ).
#
#==============================================================================================

## One oracle case: the mode, its arguments, and the reference build's output.
struct TestXmlDocumentCase:
    let mode: str
    let arguments: str[2]
    let expected: str

''')
out.write('## Every oracle case, in cases.txt order (%d more, marked "!" there, deviate on purpose and are\n## pinned by tests_xml_document_builder instead).\n' % deviating)
out.write('let test_xml_document_cases: TestXmlDocumentCase[%d] = [\n' % len(cases))
for fields, lines in cases:
    mode = fields[0]
    args = [unescape(a) for a in fields[1:]] + [''] * (3 - len(fields))
    expected = ''.join(normalize(l) + '\n' for l in lines)
    out.write('    TestXmlDocumentCase(mode = %s, arguments = [%s], expected = %s),\n' % (lit(mode), ', '.join(lit(a) for a in args[:2]), lit(expected)))
out.write(']\n')
