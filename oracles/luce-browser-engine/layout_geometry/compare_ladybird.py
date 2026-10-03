# Compares the oracle's output (expected.txt) for the cases taken from Ladybird's tests ("# block-and-inline/NAME"
# comments in cases.txt) with Ladybird's own expectation files: the layout half (r44, r45), or with --full the whole
# file, the layout, paint tree and stacking context dumps (r50).
# Usage: python3 compare_ladybird.py [CASES EXPECTED] (default: cases.txt expected.txt; r45's are cases_inline.txt
# expected_inline.txt).
import os, sys
root = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibWeb/Layout/expected/'
full = '--full' in sys.argv
args = [a for a in sys.argv[1:] if a != '--full']
cases_file, expected_file = (args[0], args[1]) if len(args) > 1 else ('cases.txt', 'expected.txt')
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
for o in outs:
    while o and o[-1] == '':
        o.pop()
for n, o in zip(names, outs):
    if not n:
        continue
    text = open(root + n + '.txt').read()
    t = (text.rstrip() if full else text.split('\n\n')[0].rstrip()).split('\n')
    if not full:
        o = o[:o.index('') if '' in o else len(o)]
    print(('OK   ' if t == o else 'DIFF ') + n)
