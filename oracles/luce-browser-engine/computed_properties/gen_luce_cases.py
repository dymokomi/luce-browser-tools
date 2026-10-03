# Writes the Luce case table (tests_computed_properties_cases.lucb) from cases.txt and the oracle's
# expected.txt (./oracle > expected.txt), leaving out the cases marked "!". Usage: python3 gen_luce_cases.py OUT.lucb
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
    reader, _, declarations = line.partition('\t')
    rows.append('    TestComputedPropertiesCase(reader = %s, declarations = %s, expected = %s),' % (lit(reader), lit(declarations), lit(result)))
out = open(sys.argv[1], 'w')
out.write('''#==============================================================================================
#
#   tests_computed_properties_cases - ComputedProperties' readers pinned to the reference build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that builds a
#       ComputedProperties from initial and declared values with the reference build's
#       StyleComputer::compute_value_of_property and calls one reader per case
#       (luce-browser-tools, oracles/luce-browser-engine/computed_properties: oracle.cpp,
#       cases.txt, gen_luce_cases.py).
#
#==============================================================================================

## One oracle case: the reader, the declarations ("property: value; ...") and the reference
## build's printout of the reader's result.
struct TestComputedPropertiesCase:
    let reader: str
    let declarations: str
    let expected: str

## Every oracle case, in cases.txt order.
let test_computed_properties_cases: TestComputedPropertiesCase[%d] = [
''' % len(rows))
out.write('\n'.join(rows) + '\n]\n')
