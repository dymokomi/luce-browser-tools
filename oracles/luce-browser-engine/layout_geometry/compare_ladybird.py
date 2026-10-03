# Compares the oracle's output (expected.txt) for the cases taken from Ladybird's tests ("# block-and-inline/NAME"
# comments in cases.txt) with the layout half of Ladybird's own expectation files.
root = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibWeb/Layout/expected/'
lines = open('cases.txt').read().split('\n')
names = []
pending = None
for l in lines:
    if l.startswith('# ') and '/' in l and ' ' not in l[2:]:
        pending = l[2:]
    elif l and not l.startswith('#'):
        names.append(pending)
        pending = None
outs = []
cur = None
for l in open('expected.txt').read().split('\n'):
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
