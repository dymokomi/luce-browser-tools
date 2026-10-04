#!/usr/bin/env python3
"""The luce-browser skeleton generator (DESIGN.md §4.4). Local tool; never committed to a package.

    python3 extract.py -j 13                      # once per donor/build change: ../cache/model.json
    python3 generate.py foundation css ...        # plan everything, emit the named packages

Every package is planned together (one closure, one placement, one naming), so the packages
agree with each other; only the named packages are written, into --out (default ../cache/out,
`luce-browser-<package>/` with the flat layout: src/<module>/, docs/namemap.tsv, regions.tsv,
gc_fields.tsv; package.prisma, README, LICENSE, PIN and test.sh too).

The generator never writes into a ported repository. A later phase is planned against the ported
engine and merged (DESIGN.md §4.4 "Later phases"):

    python3 generate.py engine --phase 1 --baseline ENGINE --lower PINS --out ../cache/out1
    python3 generate.py engine --phase 2 --baseline ENGINE --lower PINS --out ../cache/out2
    python3 merge.py --old ../cache/out1 --new ../cache/out2 --engine ENGINE

--baseline keeps the engine's names and class ids (baseline.py); --lower is where the lower
packages are checked out at bootstrap/PACKAGES's pins (their class ids); merge.py brings over only
what phase 2 adds.
"""
import argparse
import collections
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import names as N  # noqa: E402
import scope as S  # noqa: E402
from clangenv import CACHE, PIN, DONOR  # noqa: E402
from plan import Planner, Decl, lib_of  # noqa: E402
from naming import Namer  # noqa: E402
from emit import Emitter  # noqa: E402
from typemap import walk_refs, has_unmapped  # noqa: E402
import support  # noqa: E402
from baseline import Baseline  # noqa: E402
import package_files  # noqa: E402

ROOT = '/Users/sedov/Dev/luce_dev'
# The default output: a scratch tree beside the cache, never the package repositories themselves.
SCRATCH = os.path.join(CACHE, 'out')


def log(msg):
    sys.stderr.write(msg + '\n')


def build_plan(baseline=None, frozen=None):
    """Plan, place and name. `frozen`: the declarations (key -> module) of the previous phase's plan;
    against a baseline the lower packages are not regenerated, so a declaration new in this phase
    that would land in one of them lives in the engine's `web` module instead."""
    t0 = time.time()
    p = Planner(CACHE + '/model.json', log)
    for name in support.OVR_AK:
        p.decl(('ovr', 'ak', name))
    for name in support.OVR_GC:
        p.decl(('ovr', 'gc', name))
    p.plan()
    for d in p.decls.values():
        if d.level != 'full' and d.kind in ('record', 'inst'):
            d.kind = 'opaque'
        elif d.level != 'full' and d.kind == 'tmpl':
            d.kind = 'opaque_tmpl'
    log(f'plan: {len(p.decls)} declarations, {len(p.funcs)} functions ({time.time() - t0:.1f}s)')
    p.place()
    # vars: module = owner's or file's, bumped for their type
    for v, ref in p.var_tasks:
        parent = v.get('parent')
        origin = p.decls[('rec', parent)].module if parent else S.default_module(v['file'])
        need = {p.decls[k].module for k, bv in walk_refs(ref, True) if k in p.decls}
        v['_module'] = p.bump(origin, need | {origin})
    p.relocate_foreign()
    if frozen is not None:
        for d in p.decls.values():
            if d.kind != 'ovr' and d.module != 'web' and frozen.get(d.key) != d.module:
                log(f'phase {S.PHASE}: {d.q or d.key} is new here; it goes to web (the lower packages are frozen)')
                d.module = 'web'
    # function lookup for vtables
    p.func_by_usr = {}
    for F in p.funcs:
        if F.owner_key and F.owner_key[0] == 'rec':
            p.func_by_usr[(F.owner_key[1], F.fn['usr'])] = F
    assign_class_ids(p)
    p.mixin_usrs = {bu for d in p.decls.values() if d.kind == 'record' and d.level == 'full'
                    for role, t, bu in p.eff_bases(d.key[1]) if role == 'mixin'}
    nm = Namer(p, baseline)
    nm.assign_types()
    generated = generated_names(p)
    nm.assign_funcs(generated)
    for F in p.funcs:
        F.dispatcher = None
        if F.kind == 'impl' and not F.slot and F.owner_key and F.owner_key[0] == 'rec':
            vi = p.vinfo.get(F.owner_key[1])
            if vi and any(s['usr'] == F.fn['usr'] for s in vi['slots']):
                F.dispatcher = F.name[:-len('_impl')] if F.name.endswith('_impl') else F.name + '_dispatch'
    name_vars(p, nm)
    log(f'placed and named ({time.time() - t0:.1f}s)')
    return p, nm


def collect_mixins(p, em):
    """The stateful mixins of the engine's cell classes: [(field key, Luce type, C++ name)]."""
    seen = {}
    for d in p.decls.values():
        if d.kind != 'record' or d.level != 'full' or d.module != 'web' or not p.is_cell(d.key[1]):
            continue
        for role, t, bu in p.eff_bases(d.key[1]):
            if role == 'mixin':
                key = N.snake(p.records[bu]['q'].split('::')[-1])
                bd = p.decls[('rec', bu)]
                name = bd.name if bd.module == 'web' else f'{bd.module}.{bd.name}'
                seen.setdefault(key, (key, name, bd.q))
    return [seen[k] for k in sorted(seen)]


def generated_names(p):
    """Names the emitter generates per polymorphic class, reserved before functions are named."""
    gen = collections.defaultdict(set)
    for d in p.decls.values():
        if d.kind == 'record' and d.level == 'full' and p.is_poly(d.key[1]):
            s = N.snake(d.name)
            gen[d.module] |= {f'{s}_vtable', f'{s}_class', f'is_{s}', f'as_{s}', f'as_if_{s}', f'{s}_mixins'}
            u = d.key[1]
            while u is not None:
                for role, t, bu in p.eff_bases(u):
                    if role == 'mixin':
                        gen[d.module].add(f'{s}_{N.snake(p.records[bu]["q"].split("::")[-1])}_vtable')
                u = p.primary(u)
        if d.kind == 'record' and d.level == 'full':
            s = N.snake(d.name)
            gen[d.module] |= {f'{s}_init_fields', f'{s}_children', f'{s}_subtree', f'{s}_inclusive_subtree'}
    # dispatcher names: <owner>_<word> of introduced slots are taken by naming (impl gets _impl)
    return gen


def name_vars(p, nm):
    used = nm.used_funcs
    for v, ref in p.var_tasks:
        module = v['_module']
        parent = v.get('parent')
        if parent:
            base = N.snake(p.decls[('rec', parent)].name) + '_' + N.snake(v['n'])
        else:
            base = nm.free_prefix(module, '::'.join(v['q'].split('::')[:-1])) + N.snake(v['n'])
        base = N.safe(base)
        name = base
        n = 2
        while name in used[module]:
            name = f'{base}_{n}'
            n += 1
        used[module].add(name)
        v['_name'] = name


def assign_class_ids(p):
    """Pre-order class ids per hierarchy root over the full polymorphic records (§2.3)."""
    children = collections.defaultdict(list)
    roots = []
    for d in p.decls.values():
        if d.kind != 'record' or d.level != 'full' or not p.is_poly(d.key[1]):
            continue
        prim = p.primary(d.key[1])
        if prim is not None and p.is_poly(prim) and ('rec', prim) in p.decls:
            children[prim].append(d.key[1])
        else:
            roots.append(d.key[1])
    counter = [0]

    def visit(u):
        first = counter[0]
        counter[0] += 1
        for c in sorted(children[u], key=lambda x: p.records[x]['q']):
            visit(c)
        p.class_ids[u] = (first, counter[0] - 1)

    for r in sorted(roots, key=lambda x: p.records[x]['q']):
        counter[0] = 0
        visit(r)


def emit_package(p, nm, package, args):
    out = os.path.join(args.out, f'luce-browser-{package}')
    src = os.path.join(out, 'src')
    for m in S.MODULES:
        if S.PACKAGE_OF[m] == package and m not in S.FOREIGN_MODULES and os.path.isdir(os.path.join(src, m)):
            shutil.rmtree(os.path.join(src, m))
    em = Emitter(p, nm, package, out)
    em.baseline = nm.baseline if package == 'engine' else None
    em.support = {}
    if package == 'foundation':
        em.support['ak'] = support.ak_support()
        em.support['gc'] = support.gc_support()
    em.alias_lets = {}
    em.module_names = collections.defaultdict(set)
    for m in S.MODULES:
        em.module_names[m] = set(nm.used_funcs.get(m, set())) | set(S.MODULES)
    for m, s in generated_names(p).items():
        em.module_names[m] |= s
    for F in p.funcs:
        if getattr(F, 'dispatcher', None):
            em.module_names[F.module].add(F.dispatcher)
    em.realm_usr = next((u for u, r in p.records.items() if r['q'] == 'JS::Realm' and r.get('kind') != 'template'), '')
    if package == 'engine':
        em.support['web'] = support.web_support(collect_mixins(p, em))
    em.emit()
    docs = os.path.join(out, 'docs')
    os.makedirs(docs, exist_ok=True)
    with open(os.path.join(docs, 'namemap.tsv'), 'w') as f:
        f.write('# C++ name\tpackage\tmodule\tLuce name\tfile:line\tkind\n')
        for q, module, name, loc, kind in sorted(em.namemap, key=lambda x: (x[0], x[2])):
            f.write(f'{q}\t{package_of_module(module)}\t{module}\t{name}\t{loc}\t{kind}\n')
    write_regions(p, package, docs)
    with open(os.path.join(docs, 'skipped.tsv'), 'w') as f:
        f.write('# C++ functions of this package\'s regions the generator did not stub (port them by hand)\n')
        f.write('# C++ name\tfile:line\treason\n')
        for q, fl, line, reason in sorted(set(p.skipped)):
            r = S.region_of(fl)
            if r and S.PACKAGE_OF[r[2]] == package and reason in ('parameter pack', 'non-type template parameter',
                                                                   'C variadic', 'deduction guide'):
                f.write(f'{q}\t{fl}:{line}\t{reason}\n')
    write_gc_fields(p, em, package, docs)
    package_files.write(out, package, force=args.meta)
    for m in S.MODULES:
        if S.PACKAGE_OF[m] == package and m not in S.FOREIGN_MODULES:
            fmt_tree(os.path.join(src, m))
    return em.stats


def fmt_one(path):
    import subprocess
    r = subprocess.run(['luce-base', 'fmt', path, '--write'], capture_output=True, text=True)
    return path, r.returncode, r.stderr


def fmt_tree(root):
    """`luce fmt` is the authority on layout (language §3.2): format every emitted fragment."""
    import concurrent.futures
    paths = []
    for dirpath, _, files in os.walk(root):
        paths += [os.path.join(dirpath, f) for f in files if f.endswith('.lucb')]
    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as ex:
        for path, rc, err in ex.map(fmt_one, paths):
            if rc != 0:
                log(f'fmt failed: {path}: {err.strip()[:200]}')


def package_of_module(m):
    return 'luce-browser-' + S.PACKAGE_OF[m]


def write_regions(p, package, docs):
    stems = collections.defaultdict(set)
    for path in iter_donor_files():
        r = S.region_of(path)
        if r:
            stems[r[0]].add(S.stem(path))
    counts = collections.Counter()
    for F in p.funcs:
        if F.region:
            counts[F.region[0]] += 1
    with open(os.path.join(docs, 'regions.tsv'), 'w') as f:
        f.write('# region\tname\tpackage\tmodule\tdonor lines\tfunctions\tfiles\n')
        for rid, rname, module, _ in S.REGIONS:
            if S.PACKAGE_OF[module] != package:
                continue
            files = sorted(stems.get(rid, ()))
            lines = 0
            for st in files:
                for ext in ('.h', '.cpp'):
                    path = donor_path(st + ext)
                    if os.path.exists(path):
                        with open(path, errors='replace') as fh:
                            lines += sum(1 for _ in fh)
            f.write(f'{rid}\t{rname}\t{package_of_module(module)}\t{module}\t{lines}\t{counts[rid]}\t'
                    f'{" ".join(files)}\n')


def donor_path(rel):
    if rel.startswith('Build/'):
        return DONOR + '/Build/release/' + rel[6:]
    return DONOR + '/' + rel


_donor_files = None


def iter_donor_files():
    global _donor_files
    if _donor_files is None:
        _donor_files = []
        for top in ('AK', 'Libraries'):
            for dirpath, _, files in os.walk(os.path.join(DONOR, top)):
                for fn in files:
                    if fn.endswith(('.h', '.cpp')):
                        _donor_files.append(os.path.relpath(os.path.join(dirpath, fn), DONOR))
        for dirpath, _, files in os.walk(os.path.join(DONOR, 'Build/release/Libraries')):
            for fn in files:
                if fn.endswith(('.h', '.cpp')):
                    _donor_files.append('Build/' + os.path.relpath(os.path.join(dirpath, fn), DONOR + '/Build/release'))
    return _donor_files


def write_gc_fields(p, em, package, docs):
    """Per cell class, the fields that hold GC pointers (the visit_edges lint of §4.4)."""
    rows = []
    for d in p.decls.values():
        if d.kind != 'record' or d.level != 'full' or S.PACKAGE_OF[d.module] != package:
            continue
        if not p.is_cell(d.key[1]):
            continue
        for fd, ref in d.info.get('fields', []):
            cells = [k for k, bv in walk_refs(ref, True) if k[0] == 'rec' and p.is_cell(k[1])]
            if cells:
                rows.append((d.q, d.name, fd['n']))
    with open(os.path.join(docs, 'gc_fields.tsv'), 'w') as f:
        f.write('# C++ class\tLuce struct\tfield holding GC pointers (must be visited in visit_edges)\n')
        for r in sorted(rows):
            f.write('\t'.join(r) + '\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('packages', nargs='*', default=S.PACKAGES)
    ap.add_argument('--meta', action='store_true', help='rewrite package.prisma, README, LICENSE, test.sh')
    ap.add_argument('--phase', type=int, default=2, choices=(1, 2),
                    help='plan phase 1 only, or phases 1 and 2 (merge.py diffs the two)')
    ap.add_argument('--baseline', default='',
                    help='a ported luce-browser-engine checkout: keep its names and class ids (baseline.py)')
    ap.add_argument('--lower', default='',
                    help='where luce-browser-{foundation,css,html,render} are checked out at the pins '
                         '(default: beside the baseline)')
    ap.add_argument('--out', default=SCRATCH,
                    help='where luce-browser-<package>/ is written (never a ported repository: the generator '
                         'rewrites whole fragments; merge.py brings what is new into a repository)')
    args = ap.parse_args()
    baseline = Baseline(args.baseline, args.lower or None) if args.baseline else None
    frozen = None
    if baseline is not None and args.phase > 1:
        S.set_phase(args.phase - 1)
        prev, _ = build_plan(baseline)
        frozen = {k: d.module for k, d in prev.decls.items()}
    S.set_phase(args.phase)
    p, nm = build_plan(baseline, frozen)
    report = open(os.path.join(CACHE, 'skipped.tsv'), 'w')
    for q, f, line, reason in sorted(set(p.skipped)):
        report.write(f'{q}\t{f}:{line}\t{reason}\n')
    report.close()
    for package in args.packages:
        t0 = time.time()
        stats = emit_package(p, nm, package, args)
        log(f'{package}: ' + ', '.join(f'{k} {v}' for k, v in sorted(stats.items())) +
            f' ({time.time() - t0:.1f}s)')


if __name__ == '__main__':
    main()
