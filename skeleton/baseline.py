"""The ported package a skeleton is generated against (a *baseline*): its names and class ids.

Once regions have ported a package, the generator never rewrites its files; it plans the whole
closure again and writes a scratch tree, and merge.py brings over what is new. For the new
declarations to agree with the ported ones, the generator reads from the baseline:

- `docs/namemap.tsv`: the Luce name of every C++ declaration already named. A new plan names a
  declaration or function the same way when the namemap knows it (regions chose some names by
  hand), and never gives a new one a name the namemap gives to another C++ declaration;
- every top-level declaration in `src/` (a name the port already uses);
- the class ids of the polymorphic classes (`<class>_class: ak.ClassInfo = ak.ClassInfo(...,
  first_id = A, last_id = B, ...)`). Ids are numbered in pre-order once; renumbering would rewrite
  every ported types file. A new class shares the id of its nearest numbered ancestor and is
  recognized by its ClassInfo (the convention the regions settled on: r14's Realm, r28's
  Fetch::Response, r55's DOMRect).
"""
import collections
import os
import re

DECL = re.compile(r'^(?:pub\s+)?(?:extern\s+)?(func|struct|enum|union|interface|let|var|type)\s+([A-Za-z_][A-Za-z0-9_]*)')
CLASS_INFO = re.compile(r'^pub let ([a-z0-9_]+_class): ak\.ClassInfo = ak\.ClassInfo\(name = "[^"]*", '
                        r'first_id = (\d+), last_id = (\d+)')
PORTED = re.compile(r'^## Ported from \S+ ([A-Za-z_][\w:<>,~ ]*?[\w>])(?: \(|\.|,| \[|$)')
FUNC = re.compile(r'^(?:pub\s+)?func\s+([A-Za-z_][A-Za-z0-9_]*)')
TYPE_KINDS = {'struct', 'opaque', 'enum', 'generic struct', 'opaque generic', 'instantiation', 'variant'}
FUNC_KINDS = {'stub', 'impl', 'construct', 'function', 'declared only (not ported)'}


class Baseline:
    def __init__(self, root, lower_root=None):
        self.root = root
        self.rows = collections.defaultdict(list)     # C++ name -> [(Luce name, loc, kind)]
        self.owner = {}                               # Luce name -> C++ name (first namemap row)
        self.names = {}                               # Luce name -> (relative file, kind)
        self.class_ids = {}                           # '<snake class>_class' -> (first, last)
        self.opaque = set()                           # structs still declared opaque
        path = os.path.join(root, 'docs', 'namemap.tsv')
        with open(path) as f:
            for line in f:
                if line.startswith('#'):
                    continue
                parts = line.rstrip('\n').split('\t')
                if len(parts) < 6:
                    continue
                q, _, module, name, loc, kind = parts[:6]
                self.rows[q].append((name, loc, kind))
                self.owner.setdefault(name, q)
        src = os.path.join(root, 'src')
        for dirpath, _, files in os.walk(src):
            for fn in files:
                if not fn.endswith('.lucb'):
                    continue
                full = os.path.join(dirpath, fn)
                rel = os.path.relpath(full, src)
                with open(full) as fh:
                    lines = fh.read().split('\n')
                self.read_ported_lines(lines)
                for i, line in enumerate(lines):
                    m = DECL.match(line)
                    if m:
                        self.names.setdefault(m.group(2), (rel, m.group(1)))
                        if m.group(1) == 'struct' and i + 1 < len(lines) and \
                                lines[i + 1].strip() == 'pub var opaque_: u8' and \
                                (i + 2 >= len(lines) or not lines[i + 2].startswith('    ')):
                            self.opaque.add(m.group(2))
                    m = CLASS_INFO.match(line)
                    if m:
                        self.class_ids[m.group(1)] = (int(m.group(2)), int(m.group(3)))
        self.read_lower(lower_root or os.path.dirname(os.path.abspath(root)))

    def read_lower(self, lower_root):
        """Class ids of the lower packages' classes (`gc.cell_class`: GC::Cell is a root the engine's
        classes descend from), from checkouts of luce-browser-<package>/src/<module>/ under
        `lower_root` (the bootstrap pins; by default the engine's sibling checkouts)."""
        for package in ('foundation', 'css', 'html', 'render'):
            src = os.path.join(lower_root, f'luce-browser-{package}', 'src')
            if not os.path.isdir(src):
                continue
            for module in sorted(os.listdir(src)):
                mdir = os.path.join(src, module)
                for dirpath, _, files in os.walk(mdir):
                    for fn in files:
                        if not fn.endswith('.lucb'):
                            continue
                        with open(os.path.join(dirpath, fn)) as fh:
                            for line in fh:
                                m = CLASS_INFO.match(line)
                                if m:
                                    self.class_ids[f'{module}.{m.group(1)}'] = (int(m.group(2)), int(m.group(3)))

    def read_ported_lines(self, lines):
        """`## Ported from <file>[:<line>] <C++ name>...` above a function names it too (regions added
        functions, closure stubs among them, without namemap rows)."""
        for i, line in enumerate(lines):
            m = PORTED.match(line)
            if not m:
                continue
            q = m.group(1).rstrip('.,;:')
            for nxt in lines[i + 1:i + 8]:
                fm = FUNC.match(nxt)
                if fm:
                    name = fm.group(1)
                    if not any(n == name for n, _, _ in self.rows.get(q, ())):
                        self.rows[q].append((name, '', 'function'))
                    self.owner.setdefault(name, q)
                    break
                if not nxt.startswith('#'):
                    break

    def has(self, name):
        return name in self.names

    def type_name(self, q):
        """The baseline's name of a C++ type, when the namemap names it exactly once (the first
        skeleton recorded some foreign types under `Web::`: `Web::HTTP::HeaderList`)."""
        for cand in (q, 'Web::' + q):
            names = {n for n, loc, kind in self.rows.get(cand, ()) if kind in TYPE_KINDS}
            if len(names) == 1:
                return names.pop()
        return None

    def func_names(self, q):
        return [(n, loc, kind) for n, loc, kind in self.rows.get(q, ()) if kind in FUNC_KINDS]

    def taken_by_other(self, name, q):
        """Whether the baseline uses `name` for a C++ declaration other than `q`."""
        owner = self.owner.get(name)
        return owner is not None and owner != q and owner != 'Web::' + q
