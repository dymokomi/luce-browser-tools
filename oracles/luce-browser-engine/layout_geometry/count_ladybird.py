# r50: counts the cases taken from Ladybird's Layout tests (named by a "# DIR/NAME" comment) whose layout-mode case
# the port runs (not marked "!"), by directory. Usage: python3 count_ladybird.py CASES...
import os, sys, collections
root = '/Users/sedov/Dev/luce_dev/.donors/ladybird-pin/Tests/LibWeb/Layout/expected/'
run = collections.Counter(); waiting = collections.Counter()
for cases_file in sys.argv[1:]:
    pending = None
    for l in open(cases_file).read().split('\n'):
        if l.startswith('# ') and ' ' not in l[2:] and os.path.exists(root + l[2:] + '.txt'):
            pending = l[2:]
        elif l and not l.startswith('#'):
            mode = l.lstrip('!').split('\t')[0]
            if pending and mode == 'layout':
                d = 'block-and-inline' if pending.startswith('block-and-inline/') else 'other'
                (waiting if l.startswith('!') else run)[d] += 1
            pending = None if mode == 'layout' else pending
for d in sorted(set(run) | set(waiting)):
    print(d, 'run (match all three dumps):', run[d], 'waiting:', waiting[d])
