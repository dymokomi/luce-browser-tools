# Marks the cases of cases_inline.txt named on the command line (their Tests/LibWeb/Layout/input path without .html)
# with "!" (they wait for other regions' functions or a fix in another package; the oracle still runs them,
# gen_luce_cases.py leaves them out).
# Usage: python3 mark_later.py [--cases FILE] NAME...   (FILE defaults to cases_inline.txt; r47: cases_grid.txt)
import sys
args = sys.argv[1:]
cases_file = 'cases_inline.txt'
if args[:1] == ['--cases']:
    cases_file = args[1]
    args = args[2:]
names = set(args)
lines = open(cases_file).read().split('\n')
out = []
pending = None
for l in lines:
    if l.startswith('# ') and ' ' not in l[2:]:
        pending = l[2:]
        out.append(l)
        continue
    if pending in names and l and not l.startswith('!') and not l.startswith('#'):
        l = '!' + l
    if l and not l.startswith('#'):
        pending = None
    out.append(l)
open(cases_file, 'w').write('\n'.join(out))
