# Compares the oracle's output (expected.txt) for the cases taken from Ladybird's tests ("# block-and-inline/NAME"
# comments in cases.txt) with the layout half of Ladybird's own expectation files.
# Usage: python3 compare_ladybird.py [CASES EXPECTED] (default: cases.txt expected.txt; r45's are cases_inline.txt
# expected_inline.txt).
import os, sys
root = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibWeb/Layout/expected/'
cases_file, expected_file = (sys.argv[1], sys.argv[2]) if len(sys.argv) > 2 else ('cases.txt', 'expected.txt')
lines = open(cases_file).read().split('\n')
names = []
pending = None
for l in lines:
    if l.startswith('# ') and ' ' not in l[2:] and os.path.exists(root + l[2:] + '.txt'):
        pending = l[2:]
    elif l and not l.startswith('#'):
        names.append(pending)
        pending = None
outs = []
cur = None
for l in open(expected_file).read().split('\n'):
    if l.startswith('case ') or l.startswith('later '):
        cur = []
        outs.append(cur)
    elif cur is not None and l:
        cur.append(l[2:])
for n, o in zip(names, outs):
    if not n:
        continue
    t = open(root + n + '.txt').read().split('\n\n')[0].rstrip().split('\n')
    print(('OK   ' if t == o else 'DIFF ') + n)
