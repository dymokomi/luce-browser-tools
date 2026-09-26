"""Names of every planned declaration and function (DESIGN.md §4.2), unique per module."""
import collections
import hashlib
import re

import names as N
import scope as S
from typemap import render, walk_refs

LOWER_ROOT_NS = {
    'ak': ['AK', 'AK::Detail'], 'gc': ['GC'], 'web_unicode': ['Unicode', 'Unicode::Detail'],
    'text_codec': ['TextCodec'], 'web_url': ['URL'], 'web_infra': ['Web::Infra', 'Web'],
    'css_syntax': ['Web::CSS::Parser', 'Web::CSS', 'Web'], 'css_data': ['Web::CSS', 'Web'],
    'html_syntax': ['Web::HTML', 'Web'], 'gfx': ['Gfx', 'Web', 'Gfx::Detail'], 'web_fonts': ['Gfx'],
    'raster': ['Gfx'], 'display_list': ['Web::Painting', 'Web', 'Gfx'],
}


def type_prefix_engine(ns):
    """(type prefix, function prefix) for a namespace in the engine's web module."""
    if not ns:
        return '', ''
    parts = [p for p in ns.split('::') if p and p != '(anonymous)' and p != 'Detail']
    best = None
    for i in range(len(parts), 0, -1):
        cand = '::'.join(parts[:i])
        if cand in N.ENGINE_PREFIX:
            best = (cand, N.ENGINE_PREFIX[cand], parts[i:])
            break
    if best is None:
        if parts and parts[0] == 'Web' and len(parts) > 1:
            tp = N.pascal(parts[1])
            rest = parts[2:]
            fp = N.snake(parts[1]) + '_'
        elif parts:
            tp = N.pascal(parts[0])
            rest = parts[1:]
            fp = N.snake(parts[0]) + '_'
        else:
            return '', ''
    else:
        cand, (tp, fp), rest = best
        if cand == 'Web' and rest:
            tp, fp, rest = N.pascal(rest[0]), N.snake(rest[0]) + '_', rest[1:]
    # nested namespaces below the prefixed one are part of the function prefix only
    for r in rest:
        if r in ('Parser', 'Infrastructure'):
            continue
        fp += N.snake(r) + '_'
    return tp, fp


def lower_ns_rest(module, ns):
    parts = [p for p in ns.split('::') if p and p != '(anonymous)']
    roots = sorted(LOWER_ROOT_NS.get(module, []), key=lambda s: -len(s))
    for root in roots:
        rp = root.split('::')
        if parts[:len(rp)] == rp:
            return [p for p in parts[len(rp):] if p != 'Detail']
    return [p for p in parts if p not in ('Detail', 'AK', 'Web')]


def drop_leading(prefix, name):
    ws = N.words(name)
    if prefix and len(ws) > 1 and N.pascal(ws[0]) == prefix:
        return ''.join(N.pascal(w) for w in ws[1:]) if False else N.pascal(name)[len(prefix):]
    return N.pascal(name)


class Namer:
    def __init__(self, planner):
        self.p = planner
        self.type_names = {}      # key -> name
        self.by_module = collections.defaultdict(dict)   # module -> name -> key

    # ---- types ---------------------------------------------------------------------------------
    def names(self, key):
        d = self.p.decls[key]
        return d.module, d.name

    def record_base_name(self, d):
        q = d.q
        r = self.p.records.get(d.key[1]) if d.key[0] == 'rec' else None
        parent = r.get('parent') if r else None
        if d.kind == 'enum':
            e = self.p.enums.get(d.key[1])
            parent = e.get('parent') if e else None
        leaf = q.split('::')[-1] if q else 'Anon'
        anon = '(unnamed' in q or '(anonymous' in q or (r and r.get('anon'))
        if d.kind == 'enum' and (not leaf or '(unnamed' in leaf or '(anonymous' in leaf):
            anon = True
        if parent and ('rec', parent) in self.p.decls and q not in S.MOVES:
            pd = self.p.decls[('rec', parent)]
            if pd.name is None:
                self.name_decl(pd)
            if anon:
                kind = r['kind'] if r else 'enum'
                return pd.name + ('Union' if kind == 'union' else 'Struct' if kind != 'enum' else 'Enum')
            return pd.name + N.pascal(leaf)
        if anon:
            return 'Anonymous'
        ns = '::'.join(q.split('::')[:-1])
        if '<' in leaf:
            leaf = leaf.split('<')[0]
        if d.module == 'web' or d.info.get('foreign'):
            if q in N.ENGINE_UNPREFIXED:
                return N.ENGINE_UNPREFIXED[q]
            tp, _ = type_prefix_engine(ns)
            return tp + drop_leading(tp, leaf) if tp else N.pascal(leaf)
        return N.pascal(leaf)

    def name_decl(self, d):
        if d.name is not None:
            return d.name
        k = d.kind
        if k == 'ovr':
            d.name = d.key[2]
        elif k in ('record', 'opaque', 'enum'):
            d.name = self.record_base_name(d)
            if d.key[0] == 'rec':
                r = self.p.records.get(d.key[1])
                if r and r.get('spec_of'):
                    args = r.get('spec_args') or []
                    d.name += 'Of' + ''.join(N.pascal(self.short_ser(a)) for a in args)[:40]
        elif k in ('tmpl', 'opaque_tmpl'):
            base = self.record_base_name(d)
            d.name = base
        elif k == 'inst':
            info = self.p.inst_info[d.key]
            if info['alias']:
                leaf = info['alias'].split('::')[-1]
                d.name = N.pascal(leaf)
            else:
                d.name = N.pascal(d.q.split('::')[-1]) + 'Of' + ''.join(
                    N.pascal(self.short_ref(a)) for a in info['args'])
        elif k == 'variant':
            info = self.p.variant_info[d.key]
            aliases = sorted(info['aliases'], key=lambda a: (len(a), a))
            if aliases:
                leaf = aliases[0].split('::')[-1]
                owner = self.record_by_q(aliases[0].rsplit('::', 1)[0])
                if owner is not None:
                    d.name = self.name_decl(owner) + N.pascal(leaf)
                else:
                    d.name = N.pascal(leaf) if d.module != 'web' else self.web_alias(aliases[0])
            else:
                parts = [N.pascal(self.short_ref(i)) for i in info['items']]
                name = 'Or'.join(parts)
                if len(name) > 60:
                    name = 'Or'.join(N.pascal(self.short_param(i, 200)) for i in info['items'])
                if len(name) > 100:
                    name = 'Variant' + hashlib.sha1(name.encode()).hexdigest()[:8]
                d.name = name
        elif k == 'traits':
            kref = self.p.traits_info[d.key]
            d.name = N.pascal(self.short_ref(kref)) + 'Traits'
        return d.name

    def record_by_q(self, q):
        if not hasattr(self, '_rec_by_q'):
            self._rec_by_q = {}
            for d in self.p.decls.values():
                if d.key[0] == 'rec' and d.q:
                    self._rec_by_q.setdefault(d.q, d)
        return self._rec_by_q.get(q)

    def web_alias(self, q):
        ns = '::'.join(q.split('::')[:-1])
        tp, _ = type_prefix_engine(ns)
        return tp + drop_leading(tp, q.split('::')[-1])

    def short_ser(self, ser):
        k = ser.get('k')
        if k == 'b':
            return ser['n']
        if k in ('rec', 'e'):
            return (ser.get('q') or 'x').split('::')[-1]
        if k in ('p', 'lr', 'rr', 'a'):
            return self.short_ser(ser['t'])
        if k == 'v':
            return str(ser.get('v', 'n'))
        return 'x'

    def short_ref(self, ref):
        k = ref.kind
        if k == 'b':
            return ref.name
        if k == 'decl':
            d = self.p.decls[ref.key]
            name = self.name_decl(d)
            if ref.args:
                name += ''.join(N.pascal(self.short_ref(a)) for a in ref.args)
            return name
        if k == 'ptr':
            return self.short_ref(ref.inner)
        if k == 'opt':
            return 'Optional' + N.pascal(self.short_ref(ref.inner))
        if k == 'span':
            return 'Span' + N.pascal(self.short_ref(ref.inner))
        if k == 'arr':
            return 'Array' + N.pascal(self.short_ref(ref.inner))
        if k == 'tuple':
            return 'Tuple'
        if k == 'func':
            return 'Callback'
        if k == 'tparam':
            return ref.name
        if k == 'unmapped':
            return 'Opaque'
        if k == 'fallible':
            return self.short_ref(ref.inner)
        return 'X'

    def assign_types(self):
        decls = sorted(self.p.decls.values(), key=lambda d: (len(d.q or ''), d.q or '', str(d.key)))
        for d in decls:
            self.name_decl(d)
            if d.name in N.CORE or d.name in N.RESERVED:
                d.name = N.pascal(d.module) + d.name
        # uniqueness per module
        groups = collections.defaultdict(list)
        for d in decls:
            groups[(d.module, d.name)].append(d)
        taken = collections.defaultdict(set)
        for (module, name), ds in groups.items():
            taken[module].add(name)
        for (module, name), ds in sorted(groups.items()):
            if len(ds) < 2:
                continue
            ds.sort(key=lambda d: (d.kind == 'opaque', len(d.q or ''), d.q or '', str(d.key)))
            for i, d in enumerate(ds[1:], 1):
                parts = [p for p in (d.q or '').split('::')[:-1] if p not in ('Web', 'AK', 'Detail', '(anonymous)')]
                cand = None
                for j in range(len(parts) - 1, -1, -1):
                    c = N.pascal(parts[j]) + name
                    if c not in taken[module]:
                        cand = c
                        break
                if cand is None:
                    n = 2
                    while f'{name}{n}' in taken[module]:
                        n += 1
                    cand = f'{name}{n}'
                d.name = cand
                taken[module].add(cand)
        self.taken_types = taken

    # ---- functions -----------------------------------------------------------------------------
    def fsnake(self, name):
        return N.snake(name)

    def free_prefix(self, module, ns):
        if module == 'web':
            _, fp = type_prefix_engine(ns)
            return fp
        rest = lower_ns_rest(module, ns)
        return ''.join(N.snake(r) + '_' for r in rest)

    def owner_snake(self, F):
        if F.owner_key[0] == 'ovr':
            return N.snake(F.owner_key[2])
        d = self.p.decls[F.owner_key]
        return N.snake(d.name)

    def assign_funcs(self, generated):
        """generated: module -> set of names already used by generated declarations."""
        p = self.p
        # method words per owner, disambiguated among the owner's overloads
        by_owner = collections.defaultdict(list)
        for F in p.funcs:
            fn = F.fn
            nparams = len(fn['params'])
            if F.kind == 'construct':
                F.word = 'construct'
            else:
                F.word = N.snake(N.method_word(fn['n'], nparams))
            spec = spec_suffix(fn['usr'])
            if spec:
                F.word += '_' + spec
            by_owner[(F.owner_key, F.env_key) if F.owner_key else ('free', F.module, self.free_prefix(F.module, ns_of(fn['q'])) + F.word)].append(F)
        for key, fs in by_owner.items():
            words = collections.defaultdict(list)
            for F in fs:
                words[F.word].append(F)
            for w, group in words.items():
                if len(group) > 1:
                    self.disambiguate(group)
        # override impls take the word of their slot
        slot_word = {}
        for F in p.funcs:
            if F.kind == 'impl' and F.owner_key and F.owner_key[0] == 'rec':
                vi = p.vinfo.get(F.owner_key[1])
                if vi is None:
                    continue
                if any(s['usr'] == F.fn['usr'] for s in vi['slots']):
                    slot_word[F.fn['usr']] = F.word
        for F in p.funcs:
            if F.kind == 'impl' and F.owner_key and F.owner_key[0] == 'rec':
                vi = p.vinfo.get(F.owner_key[1])
                if vi is None:
                    continue
                for fn, (iu, ifn) in vi['overrides']:
                    if fn['usr'] == F.fn['usr']:
                        F.slot = (iu, ifn['usr'])
                        if ifn['usr'] in slot_word:
                            F.word = slot_word[ifn['usr']]
        self.slot_word = slot_word
        # full names
        used = collections.defaultdict(set)
        for m, s in generated.items():
            used[m] |= s
        for F in sorted(p.funcs, key=lambda F: (F.fn['file'], F.fn['line'], F.fn['q'])):
            if F.owner_key:
                base = self.owner_snake(F) + '_' + F.word
                if F.kind == 'impl':
                    base += '_impl'
            else:
                base = self.free_prefix(F.module, ns_of(F.fn['q'])) + F.word
            base = N.safe(base)
            name = base
            n = 2
            while name in used[F.module] or name in self.taken_types.get(F.module, ()):
                name = f'{base}_{n}'
                n += 1
            used[F.module].add(name)
            F.name = name
        self.used_funcs = used

    def short_param(self, ref, limit=30):
        if ref.kind == 'drop':
            return N.snake(re.split(r'::|<', (ref.note or 'badge').rstrip('>'))[-1] or 'badge')[:30]
        if ref.kind == 'decl':
            s = self.name_decl(self.p.decls[ref.key])
        elif ref.kind in ('ptr', 'fallible'):
            return self.short_param(ref.inner, limit)
        elif ref.kind in ('opt', 'span', 'arr'):
            s = {'opt': 'Optional', 'span': 'Span', 'arr': 'Array'}[ref.kind] + N.pascal(self.short_param(ref.inner, limit))
        else:
            s = self.short_ref(ref)
        s = N.snake(s)
        return s[:limit]

    def disambiguate(self, group):
        """Suffix overloads with the fewest distinguishing parameter types (DESIGN.md §4.2)."""
        shorts = [[self.short_param(r) for _, r in getattr(F, 'orig_params', F.params)] for F in group]
        maxlen = max(len(s) for s in shorts)
        positions = [i for i in range(maxlen)
                     if len(set(s[i] if i < len(s) else None for s in shorts)) > 1]
        chosen = [[] for _ in group]
        for pos in positions:
            keys = ['_'.join(c) for c in chosen]
            counts = collections.Counter(keys)
            if all(counts[k] == 1 for k in keys):
                break
            for gi, s in enumerate(shorts):
                if counts[keys[gi]] > 1 and pos < len(s):
                    chosen[gi].append(s[pos])
        suffixes = ['_'.join(c) for c in chosen]
        seen = collections.Counter(suffixes)
        for F, suf in zip(group, suffixes):
            if seen[suf] > 1 and 'const' in F.fn.get('flags', []):
                suf = (suf + '_const') if suf else 'const'
            F.word = F.word + ('_' + suf if suf else '')


def spec_suffix(usr):
    """The template arguments of an explicit function template specialization, from its USR."""
    m = re.search(r'<#(.*?)>(?:#|$)', usr)
    if not m:
        return ''
    out = []
    for arg in m.group(1).split('#'):
        names = re.findall(r'@(?:S|E|C|U)@(\w+)', arg)
        if names:
            out.append(N.snake(names[-1]))
        elif arg:
            out.append({'I': 'i32', 'i': 'u32', 'b': 'bool', 'd': 'f64', 'f': 'f32', 'c': 'u8', 'C': 'u8',
                        'K': 'u64', 'k': 'i64', 'l': 'i64', 'L': 'u64', 's': 'i16', 'S': 'u16'}.get(arg, 'x'))
    return '_'.join(out)


def ns_of(q):
    parts = q.split('::')
    return '::'.join(parts[:-1])
