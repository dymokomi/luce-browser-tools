#!/usr/bin/env python3
"""Bring a phase's new skeleton into a ported package without overwriting ported code.

    python3 generate.py engine --phase 1 --baseline ENGINE --out ../cache/out1
    python3 generate.py engine --phase 2 --baseline ENGINE --out ../cache/out2
    python3 merge.py --old ../cache/out1 --new ../cache/out2 --engine ENGINE [--dry-run]

The two plans are made against the ported engine (its names and class ids, baseline.py). What the
phase-2 plan has and the phase-1 plan has not is what phase 2 adds; of that, merge.py writes only
what the engine does not declare yet:

- a declaration of a types fragment (struct, enum, vtable, class information, cast helper, creation
  helper) is added to the engine's fragment of the same path, after the declaration that precedes
  it in the generated fragment, or in a new fragment. A struct the engine still declares opaque
  (`pub var opaque_: u8`) is replaced by its full declaration. Anything the engine declares
  already (a region filled or renamed it) is left as it is and reported;
- the stubs of a phase-2 region go to `stubs/stub_<region>_<name>[_N].lucb` (split near
  MAX_LINES at donor-file boundaries). A function the engine already defines is left where it is,
  except a closure stub (a `trap("unported...")` body in stubs/stub_closure.lucb): it moves into its
  region's fragment with its signature, as do closure stubs of the region's donor files the
  generator names differently. New closure stubs (virtual functions of classes outside the regions
  that phase 2 pulls in) go to stubs/stub_p2_closure.lucb;
- ORDER, docs/namemap.tsv, docs/regions.tsv and docs/gc_fields.tsv get the new rows.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import scope as S  # noqa: E402

DECL = re.compile(r'^(?:pub\s+)?(?:extern\s+)?(func|struct|enum|union|interface|let|var|type)\s+'
                  r'([A-Za-z_][A-Za-z0-9_]*)')
PORTED = re.compile(r'^## Ported from (\S+?)(?::\d+)? ')
BAR = '#' + '=' * 94
MAX_LINES = 600


class Block:
    """A top-level declaration with the comment lines above it (and its `# mark:` line)."""

    def __init__(self, kind, name, lines):
        self.kind = kind
        self.name = name
        self.lines = lines
        self.mark = None
        self.gap = True

    def text(self):
        return '\n'.join(self.lines)

    def is_opaque_struct(self):
        body = [ln.strip() for ln in self.lines if ln.startswith('    ')]
        return self.kind == 'struct' and body == ['pub var opaque_: u8']

    def is_stub(self):
        body = '\n'.join(ln for ln in self.lines if ln.startswith('    '))
        return self.kind == 'func' and ('trap("unported' in body or 'trap("pure virtual' in body) and \
            body.count('\n') < 3

    def ported_file(self):
        for ln in self.lines:
            m = PORTED.match(ln)
            if m:
                return m.group(1)
        return None


class Fragment:
    """A .lucb file as its header box and a sequence of blocks and loose lines (`# mark:` lines,
    section comments and blank lines stay loose; a block remembers the mark right above it)."""

    def __init__(self, text):
        lines = text.rstrip('\n').split('\n') if text else []
        self.header = []
        i = 0
        if lines and lines[0].startswith('#===='):
            j = 1
            while j < len(lines) and not lines[j].startswith('#===='):
                j += 1
            self.header = lines[:j + 1]
            i = j + 1
        self.items = []          # Block or str (a loose line)
        pending = []
        n = len(lines)
        while i < n:
            line = lines[i]
            if line.startswith('#') and not line.startswith('# mark:'):
                pending.append(line)
                i += 1
                continue
            m = DECL.match(line)
            if m:
                block = pending + [line]
                pending = []
                i += 1
                while i < n:
                    nxt = lines[i]
                    if nxt.startswith((' ', '\t', ']', ')')):
                        block.append(nxt)
                        i += 1
                        continue
                    if nxt == '':
                        k = i
                        while k < n and lines[k] == '':
                            k += 1
                        if k < n and lines[k].startswith((' ', '\t')):
                            block += lines[i:k]
                            i = k
                            continue
                    break
                b = Block(m.group(1), m.group(2), block)
                b.gap = not self.items or self.items[-1] == ''
                for prev in reversed(self.items):
                    if prev == '':
                        continue
                    if isinstance(prev, str) and prev.startswith('# mark:'):
                        b.mark = prev
                    break
                self.items.append(b)
                continue
            self.items += pending
            pending = []
            self.items.append(line)
            i += 1
        self.items += pending

    def blocks(self):
        return [b for b in self.items if isinstance(b, Block)]

    def drop_empty_sections(self):
        """Remove `# mark:` sections left without declarations (their comments go with them)."""
        marks = [i for i, it in enumerate(self.items) if isinstance(it, str) and it.startswith('# mark:')]
        drop = set()
        for a, b in zip(marks, marks[1:] + [len(self.items)]):
            if not any(isinstance(it, Block) for it in self.items[a + 1:b]):
                drop |= set(range(a, b))
        self.items = [it for i, it in enumerate(self.items) if i not in drop]

    def insert(self, pos, block):
        """Insert a block copied from another fragment (with its mark, if it had one)."""
        block.gap = True
        items = [''] + ([block.mark, ''] if block.mark else []) + [block, '']
        if pos is None:
            self.items += items
        else:
            self.items[pos:pos] = items

    def render(self):
        out = list(self.header)
        if out:
            out.append('')
        for it in self.items:
            if isinstance(it, Block):
                if out and out[-1] != '' and (it.gap or out[-1].startswith('# mark:')):
                    out.append('')
                out += it.lines
            else:
                if it == '' and out and out[-1] == '':
                    continue
                out.append(it)
        text = '\n'.join(out).rstrip('\n') + '\n'
        return re.sub(r'\n{3,}', '\n\n', text)


def read(path):
    with open(path) as f:
        return f.read()


def lucb_files(root):
    out = []
    for dirpath, _, files in os.walk(root):
        for fn in files:
            if fn.endswith('.lucb'):
                out.append(os.path.relpath(os.path.join(dirpath, fn), root))
    return sorted(out)


def index(root):
    """name -> (relative path, Block) of every top-level declaration under a module directory."""
    out = {}
    frags = {}
    for rel in lucb_files(root):
        fr = Fragment(read(os.path.join(root, rel)))
        frags[rel] = fr
        for b in fr.blocks():
            out.setdefault(b.name, (rel, b))
    return out, frags


class Merge:
    def __init__(self, old, new, engine, log):
        self.old_root = os.path.join(old, 'luce-browser-engine')
        self.new_root = os.path.join(new, 'luce-browser-engine')
        self.engine = engine
        self.src = os.path.join(engine, 'src', 'web')
        self.log = log
        self.old, _ = index(os.path.join(self.old_root, 'src', 'web'))
        self.new, self.new_frags = index(os.path.join(self.new_root, 'src', 'web'))
        self.eng, self.eng_frags = index(self.src)
        self.touched = set()       # engine fragments changed
        self.created = []          # engine fragments created
        self.added = set()         # names added to the engine
        self.report = collections.defaultdict(list)

    # ---- types ---------------------------------------------------------------------------------
    def merge_types(self):
        for rel, fr in sorted(self.new_frags.items()):
            if rel.startswith('stubs/') or rel in ('module.lucb', 'support.lucb'):
                continue
            adds = []           # (anchor name or None, Block)
            anchor = None
            group = None        # the engine fragment of a struct this phase upgraded
            for b in fr.blocks():
                if b.kind == 'struct' and b.mark:
                    group = None
                old = self.old.get(b.name)
                same_as_p1 = old is not None and old[1].text() == b.text()
                if same_as_p1:
                    anchor = b.name
                    continue
                if b.name in self.eng and self.eng[b.name][0].startswith('stubs/'):
                    # a region declared it among the closure stubs: it moves to its types fragment,
                    # generated when the region's is a placeholder (or a part of it), else as written
                    erel, eb = self.eng[b.name]
                    if b.kind == 'func' and not eb.is_stub():
                        anchor = b.name
                        continue
                    keep_generated = b.kind == 'func' or eb.is_opaque_struct() or \
                        (b.kind == 'struct' and is_subset(eb, b)) or code_of(eb) == code_of(b) or \
                        (b.kind == 'enum' and eb.kind == 'enum' and not self.uses_own_cases(eb, b))
                    self.delete_block(erel, b.name)
                    chosen = b if keep_generated else eb
                    if not keep_generated:
                        chosen.mark = b.mark
                    adds.append((anchor, chosen))
                    self.report['moved from ' + erel + (' (generated)' if keep_generated else ' (as written)')].append(
                        f'{b.name} -> {rel}')
                    anchor = b.name
                    continue
                if b.name in self.eng:
                    erel, eb = self.eng[b.name]
                    if eb.text() == b.text():
                        pass
                    elif eb.is_opaque_struct() and not b.is_opaque_struct() and b.kind == 'struct':
                        self.replace_block(erel, b)
                        self.report['filled'].append(f'{b.name} ({erel})')
                        group = erel
                    elif b.kind == 'struct' and old is not None and is_subset(eb, b):
                        self.replace_block(erel, b)
                        self.report['upgraded (a region filled part of it)'].append(f'{b.name} ({erel})')
                        group = erel
                    elif group is not None and erel == group and b.kind != 'struct' and code_of(eb) != code_of(b):
                        self.replace_block(erel, b)
                        self.report['upgraded with its struct'].append(f'{b.name} ({erel})')
                    elif old is not None and not b.is_opaque_struct():
                        self.report['kept'].append(f'{b.name} ({erel}): the engine declares its own')
                    anchor = b.name
                    continue
                adds.append((anchor, b))
                anchor = b.name
            if adds:
                self.add_blocks(rel, fr, adds)

    def replace_block(self, erel, b):
        fr = self.eng_frags[erel]
        for i, it in enumerate(fr.items):
            if isinstance(it, Block) and it.name == b.name:
                nb = Block(b.kind, b.name, b.lines)
                fr.items[i] = nb
                self.eng[b.name] = (erel, nb)
                self.touched.add(erel)
                return
        raise KeyError(b.name)

    def uses_own_cases(self, eb, b):
        """Whether ported code names a case of a region's enum that the generated one spells differently."""
        cases = lambda blk: {m.group(1) for m in (re.match(r'^    (\w+)', ln) for ln in blk.lines) if m}
        own = cases(eb) - cases(b)
        if not own:
            return False
        pat = re.compile(r'\b' + re.escape(eb.name) + r'\.(' + '|'.join(map(re.escape, own)) + r')\b')
        for rel, fr in self.eng_frags.items():
            if rel.startswith('stubs/'):
                continue
            for blk in fr.blocks():
                if blk is not eb and pat.search(blk.text()):
                    return True
        return False

    def delete_block(self, erel, name):
        fr = self.eng_frags[erel]
        fr.items = [it for it in fr.items if not (isinstance(it, Block) and it.name == name)]
        del self.eng[name]
        self.touched.add(erel)

    def add_blocks(self, rel, new_fr, adds):
        if rel in self.eng_frags:
            fr = self.eng_frags[rel]
            for anchor, b in adds:
                pos = None
                if anchor is not None:
                    for i, it in enumerate(fr.items):
                        if isinstance(it, Block) and it.name == anchor:
                            pos = i + 1
                fr.insert(pos, b)
                self.eng[b.name] = (rel, b)
                self.added.add(b.name)
            self.touched.add(rel)
            return
        fr = Fragment('')
        fr.header = list(new_fr.header)
        for anchor, b in adds:
            fr.insert(None, b)
            self.eng[b.name] = (rel, b)
            self.added.add(b.name)
        self.eng_frags[rel] = fr
        self.touched.add(rel)
        self.created.append(rel)

    # ---- stubs ---------------------------------------------------------------------------------
    def merge_stubs(self):
        closure_rel = 'stubs/stub_closure.lucb'
        closure = self.eng_frags[closure_rel]
        moved = set()
        p2_ids = {r[0] for r in S.P2_REGIONS}
        by_region = collections.defaultdict(list)    # region rel -> [Block]
        headers = {}
        for rel, fr in sorted(self.new_frags.items()):
            base = os.path.basename(rel)
            if not rel.startswith('stubs/') or not any(base.startswith(f'stub_{r}_') for r in p2_ids):
                continue
            headers[rel] = fr.header
            for b in fr.blocks():
                if b.name in self.eng:
                    erel, eb = self.eng[b.name]
                    if erel == closure_rel and eb.is_stub():
                        by_region[rel].append(eb)
                        moved.add(b.name)
                        self.report['moved'].append(b.name)
                    else:
                        self.report['ported'].append(f'{b.name} ({erel})')
                    continue
                by_region[rel].append(b)
                self.added.add(b.name)
        # closure stubs of the regions' donor files that the generator names differently
        owners = self.owners = self.struct_files()
        for b in closure.blocks():
            if b.name in moved or not b.is_stub():
                continue
            f = self.donor_file_of(b, owners)
            r = S.region_of(f) if f else None
            if not r or r[0] not in p2_ids:
                continue
            rel = f'stubs/stub_{r[0]}_{r[1]}.lucb'
            # after the stubs of the same donor file (stem), else at the end
            stem = re.sub(r'\.(h|cpp)$', '', f)
            blocks = by_region[rel]
            pos = len(blocks)
            for i, x in enumerate(blocks):
                xf = x.ported_file() or ''
                if re.sub(r'\.(h|cpp)$', '', xf) == stem or xf and stem.endswith(re.sub(r'\.(h|cpp)$', '', xf)):
                    pos = i + 1
            blocks.insert(pos, b)
            moved.add(b.name)
            self.report['moved (named by hand)'].append(b.name)
        closure.items = [it for it in closure.items if not (isinstance(it, Block) and it.name in moved)]
        closure.drop_empty_sections()
        self.touched.add(closure_rel)
        for rel, blocks in sorted(by_region.items()):
            self.write_stub_fragments(rel, headers.get(rel) or self.region_header(rel), blocks)
        # new closure stubs
        newc = self.new_frags.get(closure_rel)
        if newc is not None:
            blocks = [b for b in newc.blocks() if b.name not in self.old and b.name not in self.eng]
            if blocks:
                hdr = header_lines('stub_p2_closure - stubs of virtual functions of classes outside the phase-2 files', [
                    'The virtual functions of the classes outside every region that the phase-2 declarations',
                    'pull in whole (DESIGN.md §4.4 "scope closure"), with generated signatures and a',
                    '`trap("unported: ...")` body. Generated by the skeleton generator from Ladybird at',
                    '47c82b38d0; the phase that owns each class ports it.'])
                self.write_stub_fragments('stubs/stub_p2_closure.lucb', hdr, blocks)
                for b in blocks:
                    self.added.add(b.name)

    def struct_files(self):
        """snake(Luce struct name) -> donor file, from the new namemap."""
        out = {}
        with open(os.path.join(self.new_root, 'docs', 'namemap.tsv')) as f:
            for line in f:
                parts = line.rstrip('\n').split('\t')
                if len(parts) >= 6 and parts[5] in ('struct', 'opaque') and ':' in parts[4]:
                    out[snake(parts[3])] = parts[4].split(':')[0]
        return out

    def donor_file_of(self, b, owners):
        """The donor file a closure stub belongs to: its `Ported from` path, a donor path its
        documentation names, or the struct its name starts with (`html_preload_entry_...`)."""
        f = b.ported_file()
        if f:
            return f
        paths = [m.group(1) for ln in b.lines if ln.startswith('##')
                 for m in re.finditer(r'(Libraries/\S+?\.(?:h|cpp))\b', ln)]
        for path in paths:
            if S.region_of(path):
                return path
        if paths:
            return paths[0]
        name = b.name
        for prefix in ('realm_create_', 'heap_allocate_', 'is_', 'as_if_', 'as_'):
            if name.startswith(prefix):
                name = name[len(prefix):]
                break
        best = None
        for sn, file in owners.items():
            if (name == sn or name.startswith(sn + '_')) and (best is None or len(sn) > len(best[0])):
                best = (sn, file)
        return best[1] if best else None

    def region_header(self, rel):
        name = os.path.basename(rel)[:-5]
        return header_lines(f'{name} - typed stubs of {name[5:]}', [
            'Closure stubs of this region\'s donor files, moved from stubs/stub_closure.lucb.'])

    def write_stub_fragments(self, rel, header, blocks):
        """One fragment, or _1, _2, ... split at donor-file boundaries near MAX_LINES lines."""
        groups = []
        limit = MAX_LINES - len(header) - 2
        if sum(len(b.lines) + 1 for b in blocks) <= limit + 40:
            limit += 40
        cur, size, cur_file = [], 0, None
        for b in blocks:
            f = b.ported_file()
            n = len(b.lines) + 1
            # split at a donor-file boundary once near the limit, anywhere at the limit
            if cur and (size + n > limit or size + n > limit - 80 and f != cur_file):
                groups.append(cur)
                cur, size = [], 0
            cur.append(b)
            size += n
            cur_file = f
        if cur:
            groups.append(cur)
        stem = rel[:-5]
        for gi, group in enumerate(groups, 1):
            path = rel if len(groups) == 1 else f'{stem}_{gi}.lucb'
            fr = Fragment('')
            hdr = list(header)
            if len(groups) > 1:
                base = os.path.basename(stem)
                hdr = [h.replace(f'#   {base} - ', f'#   {base}_{gi} - ', 1) for h in hdr]
            files = sorted({f for f in (self.donor_file_of(b, self.owners) for b in group) if f})
            if files:
                hdr = trim_donor_files(hdr, files, gi, len(groups))
            fr.header = hdr
            for b in group:
                b.mark = None
                fr.insert(None, b)
            self.eng_frags[path] = fr
            self.touched.add(path)
            self.created.append(path)

    # ---- ORDER and docs ------------------------------------------------------------------------
    def write(self, dry):
        for rel in sorted(self.touched):
            path = os.path.join(self.src, rel)
            text = self.eng_frags[rel].render()
            if dry:
                continue
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'w') as f:
                f.write(text)
        order_path = os.path.join(self.src, 'ORDER')
        order = read(order_path).split('\n')
        order = [x for x in order if x]
        for rel in sorted(set(self.created)):
            if rel in order:
                continue
            order = insert_order(order, rel)
        if not dry:
            with open(order_path, 'w') as f:
                f.write('\n'.join(order) + '\n')
        self.merge_docs(dry)

    def merge_docs(self, dry):
        docs = os.path.join(self.engine, 'docs')
        new_docs = os.path.join(self.new_root, 'docs')
        # namemap rows of what was added
        rows = []
        with open(os.path.join(new_docs, 'namemap.tsv')) as f:
            for line in f:
                if line.startswith('#'):
                    continue
                parts = line.rstrip('\n').split('\t')
                if len(parts) >= 4 and parts[3] in self.added:
                    rows.append(line.rstrip('\n'))
        have = set(read(os.path.join(docs, 'namemap.tsv')).split('\n'))
        rows = [r for r in rows if r not in have]
        if not dry and rows:
            with open(os.path.join(docs, 'namemap.tsv'), 'a') as f:
                f.write('\n'.join(rows) + '\n')
        self.report['namemap rows'] += rows
        # regions
        p2_ids = {r[0] for r in S.P2_REGIONS}
        regions = [ln for ln in read(os.path.join(new_docs, 'regions.tsv')).split('\n') if ln.split('\t')[0] in p2_ids]
        have = read(os.path.join(docs, 'regions.tsv'))
        add = [ln for ln in regions if ('\n' + ln.split('\t')[0] + '\t') not in '\n' + have]
        if not dry and add:
            with open(os.path.join(docs, 'regions.tsv'), 'a') as f:
                f.write('\n'.join(add) + '\n')
        # gc fields of the new structs
        rows = []
        with open(os.path.join(new_docs, 'gc_fields.tsv')) as f:
            for line in f:
                parts = line.rstrip('\n').split('\t')
                if len(parts) == 3 and not line.startswith('#') and parts[1] in self.added | set(self.report_filled()):
                    rows.append(line.rstrip('\n'))
        have = set(read(os.path.join(docs, 'gc_fields.tsv')).split('\n'))
        rows = [r for r in rows if r not in have]
        if not dry and rows:
            with open(os.path.join(docs, 'gc_fields.tsv'), 'a') as f:
                f.write('\n'.join(rows) + '\n')
        self.report['gc_fields rows'] += rows

    def report_filled(self):
        return [x.split(' ')[0] for x in self.report['filled']]


FIELD = re.compile(r'^    pub var (\w+): ([^#]+?)\s*(?:#.*)?$')
HEAD = re.compile(r'^pub struct \w+(?:: (.*))?:$')


def snake(name):
    return re.sub(r'(?<=[a-z0-9])(?=[A-Z])', '_', name).lower()


def code_of(block):
    return [ln for ln in block.lines if not ln.lstrip().startswith('#')]


def struct_shape(block):
    fields = {}
    conf = set()
    for ln in block.lines:
        m = FIELD.match(ln)
        if m:
            fields[m.group(1)] = m.group(2)
        m = HEAD.match(ln)
        if m and m.group(1):
            conf = {c.strip() for c in m.group(1).split(',')}
    return fields, conf


def is_subset(engine_block, generated_block):
    """Whether a region's partial struct is part of the generated one (same fields, same types)."""
    ef, ec = struct_shape(engine_block)
    gf, gc = struct_shape(generated_block)
    return bool(ef) and all(gf.get(k) == v for k, v in ef.items()) and ec <= gc and (ef, ec) != (gf, gc)


def header_lines(title, desc):
    out = [BAR, '#', f'#   {title}', '#', '#   DESCRIPTION:']
    out += ['#       ' + d if d else '#' for d in desc]
    out += ['#', BAR]
    return out


def trim_donor_files(hdr, files, gi, n):
    """Keep only this part's donor files in a split stub fragment's header."""
    out = []
    in_files = False
    for h in hdr:
        if h.strip() == '#       Donor files:':
            in_files = True
            out.append(h)
            out += [f'#           {f}' for f in files]
            continue
        if in_files:
            if h.startswith('#           ') or h.startswith('#           ...'):
                continue
            in_files = False
        out.append(h)
    return out


def insert_order(order, rel):
    """Insert a new fragment into ORDER: a types fragment among the types fragments in sorted
    position, a stub fragment after the last stub fragment, anything else at the end."""
    is_types = '/types_' in '/' + rel and not rel.startswith('generated/types_')
    if is_types:
        types = [i for i, x in enumerate(order) if '/types_' in '/' + x and not x.startswith('generated/types_')]
        after = [i for i in types if order[i] < rel]
        pos = (after[-1] + 1) if after else (types[0] if types else len(order))
        return order[:pos] + [rel] + order[pos:]
    if rel.startswith('stubs/'):
        stubs = [i for i, x in enumerate(order) if x.startswith('stubs/')]
        pos = stubs[-1] + 1 if stubs else len(order)
        return order[:pos] + [rel] + order[pos:]
    return order + [rel]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--old', required=True, help='generate.py --phase 1 output root')
    ap.add_argument('--new', required=True, help='generate.py --phase 2 output root')
    ap.add_argument('--engine', required=True, help='the luce-browser-engine checkout to merge into')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    m = Merge(args.old, args.new, args.engine, print)
    m.merge_types()
    m.merge_stubs()
    m.write(args.dry_run)
    for k, v in m.report.items():
        print(f'== {k}: {len(v)}')
        for x in v[:400]:
            print('   ', x)
    print('== created:', len(set(m.created)))
    for x in sorted(set(m.created)):
        print('   ', x)


if __name__ == '__main__':
    main()
