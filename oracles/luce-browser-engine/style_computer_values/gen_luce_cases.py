# Writes the Luce case table (tests_style_computer_values_cases.lucb) from cases.txt and the oracle's
# expected.txt (./oracle > expected.txt), leaving out the cases marked "!".
# Usage: python3 gen_luce_cases.py OUT.lucb
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
    parts = line.split('\t')
    argument = parts[3] if len(parts) > 3 else '0'
    rows.append('    TestStyleComputerValueCase(function = %s, property = %s, value = %s, argument = %s, expected = %s),' % (lit(parts[0]), lit(parts[1]), lit(parts[2]), float(argument), lit(result)))
out = open(sys.argv[1], 'w')
out.write('''#==============================================================================================
#
#   tests_style_computer_values_cases - StyleComputer's computed values pinned to the reference
#                                       build
#
#   DESCRIPTION:
#       Generated (a test vector table) from the output of a local oracle that parses each value
#       with the reference build's CSS parser and runs the StyleComputer function the case
#       names, or serializes it ("serialize": shorthands; ShorthandStyleValue expands them with
#       StyleComputer::for_each_property_expanding_shorthands) (luce-browser-tools,
#       oracles/luce-browser-engine/style_computer_values: oracle.cpp, cases.txt,
#       gen_luce_cases.py). The oracle's cases whose port reaches InitialValues (font-size,
#       font-weight, math-depth; region r40) are left out.
#
#==============================================================================================

## One oracle case: the StyleComputer function, the property whose value is parsed, the value,
## the function's numeric argument and the reference build's serialization of the result.
struct TestStyleComputerValueCase:
    let function: str
    let property: str
    let value: str
    let argument: f64
    let expected: str

## Every oracle case of cases.txt without "!", in order.
let test_style_computer_value_cases: TestStyleComputerValueCase[%d] = [
%s
]
''' % (len(rows), '\n'.join(rows)))
