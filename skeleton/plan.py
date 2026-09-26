"""Planning: scope closure, class hierarchies, placement into packages/modules, and naming.

The planner reads model.json, decides which declarations the phase needs (§4.4 "scope closure"),
places every declaration in the module of §4.1 that owns its file (bumped upward when it needs a
higher module, §4.1.3), names it (§4.2), and prepares everything the emitter writes.
"""
import collections
import json
import re

import names as N
import scope as S
from typemap import SPECIAL_TEMPLATES, Translator, Ref, B, UNIT, render, walk_refs, has_unmapped, unmapped_notes, spelling_of

CORE_LIBS = {'AK', 'LibGC', 'LibWeb', 'LibGfx', 'LibURL', 'LibTextCodec', 'LibUnicode', 'LibJS'}
DROP_BASES = {'AK::RefCounted', 'AK::AtomicRefCounted', 'AK::RefCountedBase', 'AK::Weakable', 'AK::Noncopyable',
              'AK::Detail::RefCountedBase', 'AK::Badge'}
CONCRETE_TEMPLATES = {'Gfx::Point', 'Gfx::Size', 'Gfx::Rect', 'Gfx::Line', 'Gfx::Quad', 'Gfx::Vector2',
                      'Gfx::Vector3', 'Gfx::Vector4', 'Gfx::VectorN', 'Gfx::Matrix', 'Gfx::Matrix4x4',
                      'Gfx::Matrix3x3', 'Gfx::BoundingBox', 'Gfx::Triangle', 'Gfx::Line'}
SKIP_TEMPLATES = {'AK::Formatter', 'AK::Traits', 'AK::GenericTraits', 'AK::DefaultTraits', 'IPC::Encoder',
                  'IPC::Decoder', 'AK::Detail::IntegralConstant'}
MACRO_METHODS = {'class_name', 'interface_name', 'implements_interface', 'fast_is'}
MAX_MODULE_PROBE = 4
# Methods of the hand-written container shapes: C++ template -> (Luce struct, param map, generics).
OVR_METHODS = {
    'AK::Vector': ('Vector', {'T': 'T'}, ['T']),
    'AK::HashTable': ('HashTable', {'T': 'T', 'TraitsForT': 'TT'}, ['T', 'TT: Traits[T]']),
    'AK::HashMap': ('HashMap', {'K': 'K', 'V': 'V', 'KeyTraits': 'KT'}, ['K', 'V', 'KT: Traits[K]']),
    'AK::Checked': ('Checked', {'T': 'T'}, ['T']),
}


def lib_of(path):
    if path.startswith('AK/'):
        return 'AK'
    parts = path.split('/')
    if parts[0] == 'Libraries' and len(parts) > 1:
        return parts[1]
    if parts[0] == 'Build' and len(parts) > 2 and parts[1] == 'Libraries':
        return parts[2]
    return parts[0]


class Decl:
    """One Luce declaration to emit (struct, enum, variant, traits struct, generic struct, instance)."""

    def __init__(self, key, kind):
        self.key = key
        self.kind = kind          # record, opaque, enum, variant, traits, tmpl, inst, ovr
        self.level = 'declare'
        self.module = None
        self.origin = None        # module its file assigns
        self.name = None
        self.file = ''
        self.line = 0
        self.q = ''
        self.stem = ''
        self.region = None
        self.needs = set()        # modules it must see
        self.info = {}


class Func:
    """One Luce function to emit."""

    def __init__(self, fn, owner_key=None, env=None, env_key=None, generic=None):
        self.fn = fn
        self.owner_key = owner_key
        self.env = env or {}
        self.env_key = env_key
        self.generic = generic or []   # type parameter names
        self.module = None
        self.origin = None
        self.name = None
        self.word = None
        self.region = None
        self.needs = set()
        self.params = []          # (name, Ref)
        self.ret = UNIT
        self.this = None          # Ref of this
        self.kind = 'stub'        # stub, impl, dispatcher, construct, create
        self.slot = None
        self.skip = ''
        self.notes = []


class Planner:
    def __init__(self, model_path, log):
        with open(model_path) as f:
            m = json.load(f)
        self.log = log
        self.records = {k: v for k, v in m['records'].items() if not k.startswith('fwd:')}
        self.fwd = {k[4:]: v for k, v in m['records'].items() if k.startswith('fwd:')}
        self.functions = m['functions']
        self.defs = m['defs']
        self.enums = m['enums']
        self.vars = m['vars']
        self.typedefs = m['typedefs']
        self.tmpl_by_usr = {u: r for u, r in self.records.items() if r['kind'] in ('template',)}
        self.tmpl_by_q = {}
        for r in self.tmpl_by_usr.values():
            self.tmpl_by_q.setdefault(r['q'], r)
        self.aliases = collections.defaultdict(list)
        for td in self.typedefs.values():
            t = td['t']
            if t.get('k') == 'rec' and t.get('usr') and '<' not in td['q'] and '(' not in td['q']:
                self.aliases[t['usr']].append(td['q'])
        self.tr = Translator(self)
        self.decls = {}
        self.funcs = []
        self.variant_info = {}
        self.traits_info = {}
        self.traits_records = {}
        self.inst_info = {}
        self.poly_cache = {}
        self.cell_cache = {}
        self.bases_cache = {}
        self.skipped = []
        self.class_ids = {}
        self.crtp = {}

    # ---- queries used by the translator ------------------------------------------------------
    def template_by_usr(self, usr):
        return self.tmpl_by_usr.get(usr)

    def template_by_qname(self, q):
        return self.tmpl_by_q.get(q)

    def template_by_short(self, name, context_q):
        cands = [r for q, r in self.tmpl_by_q.items() if q.split('::')[-1] == name.split('::')[-1]]
        if len(cands) == 1:
            return cands[0]
        for r in cands:
            if context_q and r['q'].startswith(context_q.rsplit('::', 1)[0]):
                return r
        return cands[0] if cands else None

    def note_q(self, usr, q, file):
        if usr not in self.records and usr not in self.fwd:
            self.fwd[usr] = {'q': q, 'file': file, 'line': 0}

    def note_variant(self, key, items, alias):
        info = self.variant_info.get(key)
        if info is None:
            self.variant_info[key] = {'items': items, 'aliases': set()}
            info = self.variant_info[key]
        for a in alias or []:
            info['aliases'].add(a)

    def note_traits(self, key, kref):
        self.traits_info.setdefault(key, kref)

    def note_traits_record(self, usr, kref):
        self.traits_records.setdefault(usr, kref)

    def template_instance_ref(self, ser, env, tr):
        tq = ser.get('tq')
        tusr = ser.get('tusr')
        tmpl = self.tmpl_by_usr.get(tusr) or self.tmpl_by_q.get(tq)
        targs = ser.get('args') or []
        if tq in SKIP_TEMPLATES:
            return Ref('unmapped', note=spelling_of(ser))
        if tmpl is not None and tq in CONCRETE_TEMPLATES:
            args = [tr.tr(a, env, 'arg') for a in targs if a.get('k') != 'v']
            if any(has_unmapped(a) or tr.has_tparam(a) for a in args):
                # an instantiation inside a generic context: keep it opaque-generic
                return Ref('unmapped', note=spelling_of(ser))
            key = ('inst', tmpl['usr'], tuple(tr.key_of(a) for a in args))
            if key not in self.inst_info:
                alias = None
                if ser.get('usr') and self.aliases.get(ser['usr']):
                    alias = sorted(self.aliases[ser['usr']], key=len)[0]
                self.inst_info[key] = {'tmpl': tmpl, 'args': args, 'alias': alias, 'targs': targs}
            elif ser.get('usr') and self.aliases.get(ser['usr']) and not self.inst_info[key]['alias']:
                self.inst_info[key]['alias'] = sorted(self.aliases[ser['usr']], key=len)[0]
            return Ref('decl', key=key)
        if tmpl is not None:
            nparams = [p for p in tmpl['tparams'] if p['k'] == 'type']
            args = []
            ti = 0
            for i, p in enumerate(tmpl['tparams']):
                if p['k'] != 'type':
                    continue
                if i < len(targs):
                    a = targs[i]
                    args.append(tr.tr(a, env, 'arg') if a.get('k') != 'v' else B('u8'))
                else:
                    args.append(B('u8'))
                ti += 1
            if len(args) != len(nparams):
                return Ref('unmapped', note=spelling_of(ser))
            for a in args:
                if a.kind == 'drop':
                    return Ref('unmapped', note=spelling_of(ser))
            return Ref('decl', key=('tmpl', tmpl['usr']), args=args)
        usr = ser.get('usr')
        if usr in self.records:
            return Ref('decl', key=('rec', usr))
        return Ref('unmapped', note=spelling_of(ser))

    def is_poly_key(self, key):
        if key[0] == 'rec':
            return self.is_poly(key[1])
        if key[0] == 'tmpl':
            r = self.tmpl_by_usr.get(key[1])
            return r is not None and self.is_cell_rec(r)
        return False

    # ---- hierarchy -----------------------------------------------------------------------------
    def rec(self, usr):
        return self.records.get(usr)

    def mode(self, r):
        lib = lib_of(r.get('file', ''))
        if lib not in CORE_LIBS:
            return 'opaque'
        if lib == 'LibJS' and r['q'] not in ('JS::Cell', 'JS::Value'):
            return 'opaque'
        return 'full'

    def eff_bases(self, usr):
        """[(role, ser, Ref|usr)] role: primary|mixin|embed; bases rewritten per §3.6/§2.5."""
        if usr in self.bases_cache:
            return self.bases_cache[usr]
        self.bases_cache[usr] = []
        r = self.records.get(usr) or self.tmpl_by_usr.get(usr)
        out = []
        if r is None:
            return out
        for t in self.base_sers(usr, r):
            if t.get('k') == 'rec':
                tq = t.get('tq')
                if tq:
                    if tq in DROP_BASES:
                        continue
                    tm = self.tmpl_by_usr.get(t.get('tusr'))
                    if tm is None or not tm['fields']:
                        continue
                    out.append(('embed', t, None))
                    continue
                q = t.get('q', '')
                if q in DROP_BASES:
                    continue
                bu = t.get('usr')
                br = self.records.get(bu)
                if br is None:
                    continue
                blib = lib_of(br.get('file', ''))
                if blib == 'LibJS' and q != 'JS::Cell':
                    if self.is_cell(bu):
                        jc = [u for u, x in self.records.items() if x['q'] == 'JS::Cell' and x['kind'] != 'template']
                        if jc:
                            out.append(('base', None, jc[0]))
                    continue
                if blib not in CORE_LIBS:
                    continue
                if not br['fields'] and not self.has_virtuals(bu) and not self.eff_bases(bu):
                    continue
                out.append(('base', t, bu))
            elif t.get('k') == 'dep':
                continue
        res = []
        primary_done = False
        for role, t, bu in out:
            if role == 'base' and not primary_done:
                res.append(('primary', t, bu))
                primary_done = True
            elif role == 'base':
                res.append(('mixin', t, bu))
            else:
                res.append(('embed', t, None))
        self.bases_cache[usr] = res
        return res

    def base_sers(self, usr, r):
        """The base types of a record, looking through field-less CRTP helper templates
        (StyleValueWithDefaultOperators<T> : StyleValue) to the non-dependent bases they add."""
        out = []
        for b in r.get('bases', []):
            t = b['t']
            tq = t.get('tq') if t.get('k') == 'rec' else None
            if tq and tq not in DROP_BASES:
                tm = self.tmpl_by_usr.get(t.get('tusr'))
                if tm is not None and not tm['fields']:
                    self.crtp.setdefault(usr, []).append((tm, t))
                    for tb in tm.get('bases', []):
                        if tb['t'].get('k') == 'rec':
                            out.append(tb['t'])
                    continue
            out.append(t)
        return out

    def primary(self, usr):
        for role, t, bu in self.eff_bases(usr):
            if role == 'primary':
                return bu
        return None

    def has_virtuals(self, usr):
        r = self.records.get(usr)
        if r is None:
            return False
        for mu in r['methods']:
            fn = self.functions.get(mu)
            if fn and 'virtual' in fn.get('flags', []):
                return True
        return False

    def is_poly(self, usr):
        if usr in self.poly_cache:
            return self.poly_cache[usr]
        self.poly_cache[usr] = False
        r = self.records.get(usr)
        if r is None or self.mode(r) == 'opaque' and lib_of(r.get('file', '')) != 'LibJS':
            res = False
        else:
            res = self.has_virtuals(usr)
            p = self.primary(usr)
            if p and self.is_poly(p):
                res = True
        if r is not None and lib_of(r.get('file', '')) == 'LibJS' and self.is_cell(usr):
            res = True
        self.poly_cache[usr] = res
        return res

    def is_cell(self, usr):
        if usr in self.cell_cache:
            return self.cell_cache[usr]
        self.cell_cache[usr] = False
        r = self.records.get(usr)
        res = False
        if r is not None:
            if r['q'] == 'GC::Cell':
                res = True
            else:
                for t in self.base_sers(usr, r):
                    bu = t.get('usr')
                    if bu and self.is_cell(bu):
                        res = True
        self.cell_cache[usr] = res
        return res

    def is_cell_rec(self, r):
        for b in r.get('bases', []):
            bu = b['t'].get('usr')
            if bu and self.is_cell(bu):
                return True
        return False

    def root_of(self, usr):
        while True:
            p = self.primary(usr)
            if p is None or not self.is_poly(p):
                return usr
            usr = p

    # ---- closure -------------------------------------------------------------------------------
    def decl(self, key):
        d = self.decls.get(key)
        if d is not None:
            return d
        kind = key[0]
        if kind == 'rec':
            r = self.records.get(key[1])
            if r is None:
                d = Decl(key, 'opaque')
                fw = self.fwd.get(key[1])
                d.q = fw['q'] if fw else key[1]
                d.file = fw['file'] if fw else ''
                d.line = fw['line'] if fw else 0
            else:
                d = Decl(key, 'record' if self.mode(r) == 'full' else 'opaque')
                d.q = r['q']
                d.file = r['file']
                d.line = r['line']
                if r.get('spec_of') and not r['fields']:
                    d.kind = 'opaque'
        elif kind == 'enum':
            e = self.enums.get(key[1])
            d = Decl(key, 'enum' if e else 'opaque')
            if e:
                d.q, d.file, d.line = e['q'], e['file'], e['line']
            else:
                d.q = key[1]
        elif kind == 'variant':
            d = Decl(key, 'variant')
        elif kind == 'traits':
            d = Decl(key, 'traits')
        elif kind == 'tmpl':
            r = self.tmpl_by_usr[key[1]]
            d = Decl(key, 'tmpl')
            d.q, d.file, d.line = r['q'], r['file'], r['line']
        elif kind == 'inst':
            r = self.tmpl_by_usr[key[1]]
            d = Decl(key, 'inst')
            d.q, d.file, d.line = r['q'], r['file'], r['line']
        elif kind == 'ovr':
            d = Decl(key, 'ovr')
            d.module = key[1]
            d.origin = key[1]
            d.name = key[2]
        else:
            raise KeyError(key)
        self.decls[key] = d
        return d

    def need(self, key, level):
        d = self.decl(key)
        if level == 'full' and d.level != 'full':
            d.level = 'full'
            self.queue.append(key)
        elif key not in self.seen_declare:
            self.seen_declare.add(key)
            if d.kind in ('variant', 'traits', 'ovr'):
                d.level = 'full'
                self.queue.append(key)

    def need_ref(self, ref, byvalue=True):
        if ref is None:
            return set()
        mods = set()
        for key, bv in walk_refs(ref, byvalue):
            self.need(key, 'full' if bv else 'declare')
            mods.add(key)
        return mods

    def in_scope_file(self, f):
        return S.region_of(f) is not None

    def fn_region_file(self, fn):
        d = self.defs.get(fn['usr'])
        if d and d.get('file'):
            if S.region_of(d['file']):
                return d['file']
        return fn.get('file', '')

    def skip_reason(self, fn):
        flags = fn.get('flags', [])
        n = fn['n']
        kind = fn['kind']
        if kind == 'DESTRUCTOR':
            return 'destructor'
        if 'deleted' in flags:
            return 'deleted'
        if 'default' in flags:
            return 'defaulted'
        if 'copy' in flags or 'move' in flags:
            return 'copy/move constructor'
        if 'assign' in flags or n == 'operator=':
            return 'assignment operator'
        if n in ('operator new', 'operator delete', 'operator new[]', 'operator delete[]'):
            return 'allocation operator'
        if 'variadic' in flags:
            return 'C variadic'
        if fn.get('macro') and n in MACRO_METHODS:
            return 'macro-generated'
        if n in MACRO_METHODS and fn.get('macro'):
            return 'macro-generated'
        if n.startswith('<deduction guide') or n.startswith('~'):
            return 'deduction guide'
        if n == 'fast_is':
            return 'fast_is<T> (subsumed by the class-id helpers, §2.4)'
        if fn['q'].startswith('IPC::'):
            return 'IPC encoder/decoder (not ported)'
        if fn['usr'] in self.primaries_with_specs and fn['usr'] not in self.defs:
            return 'function template declared only for its explicit specializations'
        if any(tp['k'] != 'type' for tp in fn.get('tparams', [])):
            return 'non-type template parameter'
        for p in fn['params']:
            if '...' in json.dumps(p['t']) or p['t'].get('k') == 'x':
                return 'parameter pack'
        if fn.get('parent'):
            pr = self.records.get(fn['parent']) or self.tmpl_by_usr.get(fn['parent'])
            if pr and pr.get('spec_of'):
                return 'explicit specialization member'
        return ''

    def plan(self):
        # function templates whose explicit specializations carry the bodies
        self.primaries_with_specs = set()
        spec_names = set()
        for fn in self.functions.values():
            if '<#' in fn['usr']:
                spec_names.add((fn.get('parent'), fn['q']))
        for fn in self.functions.values():
            if fn.get('tparams') and '<#' not in fn['usr'] and (fn.get('parent'), fn['q']) in spec_names:
                self.primaries_with_specs.add(fn['usr'])
        self.queue = collections.deque()
        self.seen_declare = set()
        # ---- roots: everything located in a phase file -----------------------------------
        for usr, r in self.records.items():
            if not S.region_of(r['file']):
                continue
            if r['kind'] == 'template':
                if r['q'] in CONCRETE_TEMPLATES or r['q'] in SKIP_TEMPLATES:
                    continue
                if r['q'] in SPECIAL_TEMPLATES:
                    continue
                if not r['fields'] and not any(
                        self.functions.get(mu) and 'static' not in self.functions[mu].get('flags', [])
                        for mu in r['methods']):
                    continue
                self.need(('tmpl', usr), 'full')
            elif r['kind'] == 'partial' or r.get('spec_of') or r.get('parent'):
                continue
            elif self.mode(r) == 'full':
                self.need(('rec', usr), 'full')
        for usr, e in self.enums.items():
            if S.region_of(e['file']) and not e.get('parent') and e['q']:
                self.need(('enum', usr), 'full')
        self.drain()
        # ---- functions of phase files ----------------------------------------------------
        self.func_tasks = []
        seen = set()
        for usr, fn in self.functions.items():
            f = self.fn_region_file(fn)
            if not S.region_of(f):
                continue
            parent = fn.get('parent')
            if parent:
                pr = self.records.get(parent)
                if pr is None:
                    continue
                if pr['kind'] == 'template':
                    if pr['q'] in OVR_METHODS:
                        self.func_tasks.append(('ovr_method', fn))
                        continue
                    if pr['q'] in CONCRETE_TEMPLATES:
                        continue  # per instantiation below
                    if ('tmpl', parent) not in self.decls or self.decls[('tmpl', parent)].level != 'full':
                        continue
                    self.func_tasks.append(('generic_method', fn))
                    continue
                if pr['kind'] == 'partial':
                    continue
                d = self.decls.get(('rec', parent))
                if d is None or d.kind != 'record':
                    continue
                self.func_tasks.append(('method', fn))
            else:
                self.func_tasks.append(('free', fn))
            seen.add(usr)
        self.build_funcs()
        self.drain()
        # instantiations discovered while translating signatures get their methods, repeatedly
        done_inst = set()
        while True:
            new = [k for k, d in self.decls.items() if d.kind == 'inst' and d.level == 'full' and k not in done_inst]
            if not new:
                break
            for key in new:
                done_inst.add(key)
                self.inst_methods(key)
            self.drain()
        # virtual impls of every full polymorphic record (in scope or pulled in)
        self.virtual_plan()
        self.drain()
        self.vars_plan()
        self.drain()

    def drain(self):
        while self.queue:
            key = self.queue.popleft()
            d = self.decls[key]
            try:
                self.expand(d)
            except RecursionError:
                self.log(f'recursion expanding {key}')

    def expand(self, d):
        kind = d.kind
        if kind == 'record':
            r = self.records[d.key[1]]
            for role, t, bu in self.eff_bases(d.key[1]):
                if role in ('primary', 'mixin'):
                    self.need(('rec', bu), 'full')
                else:
                    ref = self.tr.tr(t, {}, 'field')
                    self.need_ref(ref, True)
            d.info['fields'] = []
            for fd in r['fields']:
                ref = self.tr.tr(fd['t'], {}, 'field')
                if ref.kind == 'fallible':
                    ref = ref.inner
                self.need_ref(ref, True)
                d.info['fields'].append((fd, ref))
            for nu in r.get('nested', []):
                nr = self.records.get(nu)
                if nr is not None and (nr.get('anon') or '(unnamed' in nr['q'] or '(anonymous' in nr['q']):
                    self.need(('rec', nu), 'full')
            if self.is_poly(d.key[1]):
                root = self.root_of(d.key[1])
                if root != d.key[1]:
                    self.need(('rec', root), 'full')
            if d.key[1] in self.traits_records:
                self.need_ref(self.traits_records[d.key[1]], False)
        elif kind == 'enum':
            pass
        elif kind == 'variant':
            info = self.variant_info[d.key]
            for it in info['items']:
                self.need_ref(it, True)
        elif kind == 'traits':
            self.need_ref(self.traits_info[d.key], False)
        elif kind == 'tmpl':
            r = self.tmpl_by_usr[d.key[1]]
            env = {p['n']: Ref('tparam', name=tpn(p['n'])) for p in r['tparams'] if p['k'] == 'type'}
            d.info['env'] = env
            d.info['fields'] = []
            for fd in r['fields']:
                ref = self.tr.tr(fd['t'], env, 'field')
                if ref.kind == 'fallible':
                    ref = ref.inner
                self.need_ref(ref, True)
                d.info['fields'].append((fd, ref))
            for role, t, bu in self.eff_bases(d.key[1]):
                if role in ('primary', 'mixin'):
                    self.need(('rec', bu), 'full')
        elif kind == 'inst':
            info = self.inst_info[d.key]
            tmpl = info['tmpl']
            env = {}
            ti = 0
            for p in tmpl['tparams']:
                if p['k'] == 'type':
                    env[p['n']] = info['args'][ti] if ti < len(info['args']) else B('u8')
                    ti += 1
            d.info['env'] = env
            d.info['fields'] = []
            for fd in tmpl['fields']:
                ref = self.tr.tr(fd['t'], env, 'field')
                self.need_ref(ref, True)
                d.info['fields'].append((fd, ref))
            for a in info['args']:
                self.need_ref(a, False)

    # ---- functions -----------------------------------------------------------------------------
    def build_funcs(self):
        for kind, fn in self.func_tasks:
            reason = self.skip_reason(fn)
            if reason:
                self.skipped.append((fn['q'], fn['file'], fn['line'], reason))
                continue
            if kind == 'method':
                self.add_method(fn, ('rec', fn['parent']), {})
            elif kind == 'generic_method':
                self.add_generic_method(fn)
            elif kind == 'ovr_method':
                pr = self.records[fn['parent']]
                name, env_map, generic = OVR_METHODS[pr['q']]
                env = {c: Ref('tparam', name=l) for c, l in env_map.items()}
                F = self.add_method(fn, ('ovr', 'ak', name), env, generic=list(generic))
                F.owner_args = [l for l in env_map.values()]
            else:
                self.add_free(fn)

    def sig(self, fn, env):
        params = []
        for p in fn['params']:
            ref = self.tr.tr(p['t'], env, 'param')
            if ref.kind == 'fallible':
                ref = ref.inner
            params.append((p.get('n', ''), ref))
        ret = self.tr.tr(fn.get('ret', {'k': 'b', 'n': 'void'}), env, 'ret')
        return params, ret

    def fn_env(self, fn, base_env):
        env = dict(base_env)
        gen = []
        for tp in fn.get('tparams', []):
            if tp['k'] == 'type' and tp['n']:
                env[tp['n']] = Ref('tparam', name=tpn(tp['n']))
                gen.append(tpn(tp['n']))
        return env, gen

    def finish_func(self, F, params, ret):
        drops = [i for i, (_, r) in enumerate(params) if r.kind == 'drop']
        F.orig_params = list(params)
        F.kept = [i for i in range(len(params)) if i not in drops]
        F.defaults = [(F.fn['params'][i].get('d') if i < len(F.fn['params']) else None)
                      for i in range(len(params)) if i not in drops]
        params = [p for i, p in enumerate(params) if i not in drops]
        if ret.kind == 'drop':
            ret = UNIT
        F.params = params
        F.ret = ret
        for _, r in params:
            self.need_ref(r, False)
            if has_unmapped(r):
                F.notes.extend(unmapped_notes(r))
        self.need_ref(ret, False)
        if has_unmapped(ret):
            F.notes.extend(unmapped_notes(ret))
        self.funcs.append(F)
        return F

    def add_free(self, fn):
        env, gen = self.fn_env(fn, {})
        params, ret = self.sig(fn, env)
        F = Func(fn, generic=gen)
        return self.finish_func(F, params, ret)

    def add_method(self, fn, owner_key, env, env_key=None, generic=None, kind=None):
        env2, gen = self.fn_env(fn, env)
        params, ret = self.sig(fn, env2)
        F = Func(fn, owner_key=owner_key, env=env2, env_key=env_key, generic=(generic or []) + gen)
        if kind:
            F.kind = kind
        if fn['kind'] == 'CONSTRUCTOR':
            F.kind = 'construct'
        return self.finish_func(F, params, ret)

    def add_generic_method(self, fn):
        pr = self.tmpl_by_usr[fn['parent']]
        env = {p['n']: Ref('tparam', name=tpn(p['n'])) for p in pr['tparams'] if p['k'] == 'type'}
        gen = [tpn(p['n']) for p in pr['tparams'] if p['k'] == 'type']
        return self.add_method(fn, ('tmpl', fn['parent']), env, generic=gen)

    def inst_methods(self, key):
        info = self.inst_info[key]
        tmpl = info['tmpl']
        if not S.region_of(tmpl['file']):
            return
        d = self.decls[key]
        env = d.info.get('env')
        if env is None:
            self.expand(d)
            env = d.info['env']
        for mu in tmpl['methods']:
            fn = self.functions.get(mu)
            if fn is None:
                continue
            reason = self.skip_reason(fn)
            if reason:
                continue
            self.add_method(fn, key, env, env_key=key)

    def virtual_plan(self):
        """Dispatchers, _impl stubs and vtable layouts for every full polymorphic record."""
        self.vinfo = {}
        self.crtp_env = {}
        method_funcs = collections.defaultdict(dict)
        for F in self.funcs:
            if F.owner_key and F.owner_key[0] == 'rec':
                method_funcs[F.owner_key[1]][F.fn['usr']] = F
        polys = [d for d in self.decls.values() if d.kind == 'record' and d.level == 'full' and self.is_poly(d.key[1])]
        # order: bases first
        def depth(usr):
            n = 0
            while True:
                p = self.primary(usr)
                if p is None:
                    return n
                usr = p
                n += 1
        polys.sort(key=lambda d: depth(d.key[1]))
        for d in polys:
            usr = d.key[1]
            r = self.records[usr]
            own = {}
            for mu in r['methods']:
                fn = self.functions.get(mu)
                if fn is None or 'virtual' not in fn.get('flags', []):
                    continue
                if fn['kind'] == 'DESTRUCTOR':
                    continue
                if fn.get('macro') and fn['n'] in MACRO_METHODS:
                    continue
                if any(tp['k'] != 'type' for tp in fn.get('tparams', [])) or fn.get('tparams'):
                    continue
                own[mu] = fn
            # virtual overrides written once in a CRTP helper template (StyleValueWithDefaultOperators<T>)
            for tm, tser in self.crtp.get(usr, []):
                for mu in tm['methods']:
                    fn = self.functions.get(mu)
                    if fn is None or 'virtual' not in fn.get('flags', []) or fn['kind'] == 'DESTRUCTOR':
                        continue
                    if fn.get('tparams') or any(o['n'] == fn['n'] for o in own.values()):
                        continue
                    own[mu] = fn
                    self.crtp_env[(usr, mu)] = self.crtp_env_of(tm, tser)
            slots = []       # introduced here: list of fn
            overrides = []   # (fn, intro slot)
            for mu, fn in own.items():
                intro = self.find_intro(usr, fn)
                if intro is None:
                    slots.append(fn)
                else:
                    overrides.append((fn, intro))
            self.vinfo[usr] = {'slots': slots, 'overrides': overrides}
            # make sure there is a Func for each own virtual (impl) even out of scope
            for fn in list(own.values()):
                F = method_funcs[usr].get(fn['usr'])
                if F is None:
                    if lib_of(r['file']) == 'LibJS' and r['q'] != 'JS::Cell':
                        continue
                    F = self.add_method(fn, ('rec', usr), self.crtp_env.get((usr, fn['usr']), {}))
                    if (usr, fn['usr']) in self.crtp_env:
                        F.crtp = True
                    F.closure = True
                    method_funcs[usr][fn['usr']] = F
                F.kind = 'impl'
            # pure virtual introduced without body still gets an _impl (traps "pure virtual")

    def crtp_env_of(self, tm, tser):
        env = {}
        args = tser.get('args') or []
        i = 0
        for tp in tm['tparams']:
            if tp['k'] == 'type' and i < len(args):
                env[tp['n']] = self.tr.tr(args[i], {}, 'arg')
            i += 1
        return env

    def sig_key(self, fn):
        return json.dumps([strip_names(p['t']) for p in fn['params']], sort_keys=True) + ('c' if 'const' in fn.get('flags', []) else '')

    def find_intro(self, usr, fn):
        """The virtual slot this method overrides: (owner usr, fn) of the introducing declaration."""
        key = self.sig_key(fn)
        cands = []
        for role, t, bu in self.eff_bases(usr):
            if role not in ('primary', 'mixin') or bu is None:
                continue
            found = self.lookup_slot(bu, fn['n'], key)
            if found:
                return found
            cands.extend(self.lookup_slot_by_name(bu, fn['n']))
        if len(cands) == 1:
            return cands[0]
        return None

    def lookup_slot(self, usr, name, key):
        vi = self.vinfo.get(usr)
        if vi is not None:
            for fn in vi['slots']:
                if fn['n'] == name and self.sig_key(fn) == key:
                    return (usr, fn)
        for role, t, bu in self.eff_bases(usr):
            if role in ('primary', 'mixin') and bu is not None:
                f = self.lookup_slot(bu, name, key)
                if f:
                    return f
        return None

    def lookup_slot_by_name(self, usr, name):
        out = []
        vi = self.vinfo.get(usr)
        if vi is not None:
            out.extend((usr, fn) for fn in vi['slots'] if fn['n'] == name)
        for role, t, bu in self.eff_bases(usr):
            if role in ('primary', 'mixin') and bu is not None:
                out.extend(self.lookup_slot_by_name(bu, name))
        return out

    def vars_plan(self):
        self.var_tasks = []
        for usr, v in self.vars.items():
            if not S.region_of(v['file']):
                continue
            parent = v.get('parent')
            if parent:
                d = self.decls.get(('rec', parent))
                if d is None or d.kind != 'record':
                    continue
            if v['n'] in ('', None):
                continue
            ref = self.tr.tr(v['t'], {}, 'field')
            if ref.kind == 'fallible':
                ref = ref.inner
            self.need_ref(ref, True)
            self.var_tasks.append((v, ref))

    # ---- placement ---------------------------------------------------------------------------
    def origin_module(self, d):
        if d.q in S.MOVES:
            return S.MOVES[d.q]
        if d.kind == 'ovr':
            return d.key[1]
        if d.kind == 'variant' or d.kind == 'traits':
            return 'ak'
        f = d.file
        if d.kind == 'opaque' and lib_of(f) not in CORE_LIBS:
            d.info['foreign'] = True
            return 'ak'
        if lib_of(f) == 'LibJS':
            return 'web'
        m = S.default_module(f) if f else 'web'
        return m

    def ref_modules(self, ref, byvalue_only=False):
        mods = set()
        for key, bv in walk_refs(ref, True):
            if byvalue_only and not bv:
                continue
            mods.add(key)
        return mods

    def place(self):
        """Assign modules to every declaration and function (fixpoint of upward bumps)."""
        for d in self.decls.values():
            d.origin = self.origin_module(d)
            d.module = d.origin
            reg = S.region_of(d.file) if d.file else None
            d.region = reg[0] if reg else None
        # dependencies of declarations (keys)
        deps = {}
        for d in self.decls.values():
            ks = set()
            if d.kind in ('record', 'tmpl', 'inst'):
                for fd, ref in d.info.get('fields', []):
                    for key, bv in walk_refs(ref, True):
                        if bv:
                            ks.add(key)
                if d.kind == 'record':
                    for role, t, bu in self.eff_bases(d.key[1]):
                        if role in ('primary', 'mixin'):
                            ks.add(('rec', bu))
                        else:
                            ref = self.tr.tr(t, {}, 'field')
                            for key, bv in walk_refs(ref, True):
                                ks.add(key)
                    vi = self.vinfo.get(d.key[1])
                    if vi:
                        for fn in vi['slots']:
                            params, ret = self.sig(fn, {})
                            for _, pr in params:
                                for key, bv in walk_refs(pr, True):
                                    ks.add(key)
                            for key, bv in walk_refs(ret, True):
                                ks.add(key)
                    parent = self.records[d.key[1]].get('parent')
                    if parent and ('rec', parent) in self.decls:
                        pass
                if d.kind == 'inst':
                    for a in self.inst_info[d.key]['args']:
                        for key, bv in walk_refs(a, True):
                            ks.add(key)
            elif d.kind == 'variant':
                for it in self.variant_info[d.key]['items']:
                    for key, bv in walk_refs(it, True):
                        ks.add(key)
            elif d.kind == 'traits':
                for key, bv in walk_refs(self.traits_info[d.key], True):
                    ks.add(key)
            ks.discard(d.key)
            deps[d.key] = ks
        self.deps = deps
        changed = True
        rounds = 0
        while changed and rounds < 50:
            changed = False
            rounds += 1
            for d in self.decls.values():
                if d.kind == 'ovr':
                    continue
                need = {self.decls[k].module for k in deps[d.key] if k in self.decls}
                # a nested record lives with its parent
                if d.kind in ('record', 'opaque') and d.key[0] == 'rec':
                    r = self.records.get(d.key[1])
                    if r and r.get('parent') and ('rec', r['parent']) in self.decls and d.q not in S.MOVES:
                        pm = self.decls[('rec', r['parent'])].module
                        if pm != d.module and S.MODULES.index(pm) > S.MODULES.index(d.module):
                            d.module = pm
                            changed = True
                        need.add(pm)
                if d.kind == 'enum':
                    e = self.enums.get(d.key[1])
                    if e and e.get('parent') and ('rec', e['parent']) in self.decls and d.q not in S.MOVES:
                        pm = self.decls[('rec', e['parent'])].module
                        if S.MODULES.index(pm) > S.MODULES.index(d.module):
                            d.module = pm
                            changed = True
                m = self.bump(d.origin if d.kind not in ('variant', 'traits') else 'ak', need | {d.module})
                if m != d.module:
                    d.module = m
                    changed = True
            # parent records must not sit below nested ones (they reference them by value)
        # functions
        for F in self.funcs:
            F.origin = self.func_origin(F)
            need = set()
            for _, r in F.params:
                need |= {self.decls[k].module for k, bv in walk_refs(r, True) if k in self.decls}
            need |= {self.decls[k].module for k, bv in walk_refs(F.ret, True) if k in self.decls}
            if F.owner_key:
                need.add(self.decls[F.owner_key].module)
            F.module = self.bump(F.origin, need | {F.origin})
            reg = S.region_of(self.fn_region_file(F.fn))
            if getattr(F, 'crtp', False):
                reg = S.region_of(self.decls[F.owner_key].file)
            F.region = reg if reg else None

    def relocate_foreign(self):
        """Move each opaque declaration of a foreign library (LibCore, IPC, ...) to the highest
        module every user sees, so the lower packages never own more of them than they need."""
        users = collections.defaultdict(set)
        for d in self.decls.values():
            for k in self.deps.get(d.key, ()):
                users[k].add(d.module)
            if d.kind in ('record', 'tmpl', 'inst'):
                for fd, ref in d.info.get('fields', []):
                    for k, bv in walk_refs(ref, True):
                        users[k].add(d.module)
        for F in self.funcs:
            for _, r in F.params + [('', F.ret)]:
                for k, bv in walk_refs(r, True):
                    users[k].add(F.module)
        for v, ref in self.var_tasks:
            for k, bv in walk_refs(ref, True):
                users[k].add(v['_module'])
        for d in self.decls.values():
            if not d.info.get('foreign'):
                continue
            us = users.get(d.key) or {'web'}
            best = 'ak'
            for m in S.MODULES:
                if all(self.visible(u, m) for u in us):
                    best = m
            d.module = best

    def func_origin(self, F):
        if F.owner_key:
            return self.decls[F.owner_key].module
        f = self.fn_region_file(F.fn)
        return S.default_module(f)

    def bump(self, origin, needed):
        """The first module (from origin upward) that is or sees origin and sees every needed module."""
        for m in S.MODULES[S.MODULES.index(origin):]:
            if m != origin and origin not in S.SEES[m]:
                continue
            if m in S.FOREIGN_MODULES and m != origin:
                continue
            ok = True
            for n in needed:
                if n != m and n not in S.SEES[m]:
                    ok = False
                    break
            if ok:
                return m
        return 'web'

    def visible(self, module, target):
        return module == target or target in S.SEES[module]


def tpn(name):
    """Luce name of a C++ template type parameter (never the name of a type in scope)."""
    if not name:
        return 'T'
    return name if len(name) == 1 else 'T' + name


def strip_names(ser):
    if isinstance(ser, dict):
        return {k: strip_names(v) for k, v in ser.items() if k not in ('file', 'alias', 'line', 'tusr', 'usr', 'idx')}
    if isinstance(ser, list):
        return [strip_names(x) for x in ser]
    return ser
