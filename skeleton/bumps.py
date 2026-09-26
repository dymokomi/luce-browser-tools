#!/usr/bin/env python3
"""List declarations and functions placed above the module their donor file belongs to."""
import collections
import generate
p, nm = generate.build_plan()
c = collections.Counter()
for d in sorted(p.decls.values(), key=lambda d: d.q or ''):
    if d.kind in ('record', 'enum', 'tmpl', 'inst') and d.origin and d.module != d.origin and not d.info.get('foreign'):
        c[(d.origin, d.module)] += 1
        print(f'decl\t{d.origin}->{d.module}\t{d.q}\t{d.file}')
for F in p.funcs:
    if F.origin and F.module != F.origin:
        c[('fn ' + F.origin, F.module)] += 1
for k, v in sorted(c.items()):
    print('#', k, v)
