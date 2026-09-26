#!/usr/bin/env python3
"""Explain the placement of declarations whose C++ qualified name matches argv[1]."""
import sys
sys.argv, pat = sys.argv[:1], sys.argv[1]
import generate
from typemap import walk_refs
p, nm = generate.build_plan()
for d in p.decls.values():
    if d.q == pat:
        print(d.key, d.kind, d.level, 'origin', d.origin, 'module', d.module, d.name)
        for fd, ref in d.info.get('fields', []):
            for k, bv in walk_refs(ref, True):
                dd = p.decls.get(k)
                if dd and bv and not p.visible(d.origin, dd.module):
                    print('   field', fd['n'], '->', dd.q or dd.name, dd.module)
        if d.kind == 'record':
            for role, t, bu in p.eff_bases(d.key[1]):
                if bu:
                    bd = p.decls[('rec', bu)]
                    if not p.visible(d.origin, bd.module): print('   base', role, bd.q, bd.module)
            vi = p.vinfo.get(d.key[1])
            if vi:
                for fn in vi['slots']:
                    params, ret = p.sig(fn, {})
                    for _, r in params + [('ret', ret)]:
                        for k, bv in walk_refs(r, True):
                            dd = p.decls.get(k)
                            if dd and not p.visible(d.origin, dd.module):
                                print('   slot', fn['n'], '->', dd.q or dd.name, dd.module)
