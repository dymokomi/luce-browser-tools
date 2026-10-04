"""C++ type → Luce type (DESIGN.md §2.6), over the serialized clang types of model.json.

`Translator.tr(ser, env, use)` answers a TypeRef tree. Named declarations are referenced by a
key (`('rec', usr)`, `('enum', usr)`, `('variant', key)`, ...); the planner decides where each
lives and what it is called, then `render(ref, module)` spells the type for one module.
"""
import json
import re

# ---- template tables ---------------------------------------------------------------------------
NONNULL_PTR = {'GC::Ref', 'AK::NonnullRefPtr', 'AK::NonnullOwnPtr', 'AK::NonnullRawPtr', 'GC::RawRef',
               'AK::ValueComparingNonnullRefPtr', 'AK::NonnullRefCountedPtr'}
NULLABLE_PTR = {'GC::Ptr', 'AK::RefPtr', 'AK::OwnPtr', 'AK::RawPtr', 'GC::RawPtr', 'GC::Root',
                'AK::ValueComparingRefPtr', 'GC::HeapRoot'}
WEAK = {'GC::Weak', 'AK::WeakPtr'}
FALLIBLE = {'AK::ErrorOr', 'Web::WebIDL::ExceptionOr', 'JS::ThrowCompletionOr'}
VECTORS = {'AK::Vector', 'GC::RootVector', 'GC::ConservativeVector', 'AK::SegmentedVector',
           'AK::COWVector', 'GC::HeapVector'}
UNWRAP_FIRST = {'AK::DistinctNumeric', 'AK::Atomic', 'AK::NeverDestroyed', 'AK::Detail::__IdentityType',
                'AK::MaybeOwned'}
DROP = {'AK::Badge'}

# Generic structs written by hand (overrides) in the foundation: C++ template -> (module, Luce name).
OVERRIDE_GENERICS = {
    'AK::Vector': ('ak', 'Vector'), 'AK::HashTable': ('ak', 'HashTable'), 'AK::HashMap': ('ak', 'HashMap'),
    'AK::Checked': ('ak', 'Checked'),
}

SPECIAL_TEMPLATES = (NONNULL_PTR | NULLABLE_PTR | WEAK | FALLIBLE | VECTORS | UNWRAP_FIRST | DROP |
                     set(OVERRIDE_GENERICS) | {'AK::Optional', 'AK::Variant', 'AK::Span', 'AK::FixedArray',
                                               'AK::Array', 'AK::Tuple', 'AK::Function', 'GC::Function',
                                               'AK::HashMap', 'GC::RootHashMap', 'AK::OrderedHashMap',
                                               'AK::HashTable', 'AK::OrderedHashTable', 'GC::HeapHashTable'})
# Records a `T const&` keeps by value (see Translator.is_small_value).
SMALL_VALUES = {'AK::String', 'AK::FlyString', 'AK::StringView', 'AK::Utf16String', 'AK::Utf16View',
                'AK::Utf16FlyString', 'AK::ByteString', 'AK::Utf8View', 'AK::Utf32View', 'AK::Duration',
                'AK::UnixDateTime', 'AK::MonotonicTime', 'AK::Empty', 'AK::Error', 'URL::URL', 'URL::Origin',
                'URL::Host', 'URL::Site', 'Web::DOM::QualifiedName', 'Web::CSSPixels', 'Web::DevicePixels',
                'Web::CSSPixelFraction', 'Gfx::Color', 'Web::DOM::AbstractElement', 'JS::Value',
                'Web::UniqueNodeID', 'AK::Badge'}
INT_NAMES = {'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64', 'usize', 'isize'}


class Ref:
    """A Luce type. kind: b (builtin), decl, ptr, opt, span, arr, tuple, func, tparam, unmapped, fallible."""
    __slots__ = ('kind', 'name', 'key', 'args', 'inner', 'const', 'nullable', 'n', 'items', 'ret', 'note')

    def __init__(self, kind, **kw):
        self.kind = kind
        self.name = kw.get('name')
        self.key = kw.get('key')
        self.args = kw.get('args') or []
        self.inner = kw.get('inner')
        self.const = kw.get('const', False)
        self.nullable = kw.get('nullable', False)
        self.n = kw.get('n', 0)
        self.items = kw.get('items') or []
        self.ret = kw.get('ret')
        self.note = kw.get('note', '')

    def __repr__(self):
        return f'Ref({self.kind},{self.name or self.key},{self.args or ""}{self.inner or ""})'


def B(name):
    return Ref('b', name=name)


UNIT = B('unit')


def unmapped(spelling):
    return Ref('unmapped', note=spelling)


def spelling_of(ser):
    k = ser.get('k')
    if k == 'b':
        return ser['n']
    if k in ('rec', 'e'):
        s = ser.get('q', '?')
        if ser.get('args'):
            s += '<' + ', '.join(spelling_of(a) if a.get('k') != 'v' else str(a.get('v', 'N'))
                                 for a in ser['args']) + '>'
        return s
    if k == 'p':
        return spelling_of(ser['t']) + '*'
    if k == 'lr':
        return spelling_of(ser['t']) + '&'
    if k == 'rr':
        return spelling_of(ser['t']) + '&&'
    if k == 'a':
        return spelling_of(ser['t']) + f'[{ser.get("n", "")}]'
    if k == 'f':
        return spelling_of(ser['r']) + '(' + ', '.join(spelling_of(p) for p in ser['ps']) + ')'
    if k in ('tp',):
        return ser.get('n', 'T')
    return ser.get('s', '?')


def strip_const(ser):
    if ser.get('c'):
        ser = dict(ser)
        ser.pop('c')
    return ser


class Translator:
    def __init__(self, model):
        self.m = model

    # env: dict name->Ref (template params). use: field|param|ret|arg
    def tr(self, ser, env=None, use='field'):
        env = env or {}
        k = ser.get('k')
        if k == 'b':
            n = ser['n']
            if n == 'void':
                return UNIT
            if n == 'char':
                return B('u8')
            if n == 'nullptr':
                return Ref('ptr', inner=B('void'), nullable=True)
            if n in ('u128', 'i128'):
                return Ref('arr', inner=B('u64'), n=2, note=n)
            return B(n)
        if k == 'e':
            if ser.get('usr', '').startswith('c:@') and ser.get('q'):
                return Ref('decl', key=('enum', ser['usr']))
            return B('i32')
        if k == 'p':
            inner_ser = ser['t']
            if inner_ser.get('k') == 'f':
                f = self.tr(inner_ser, env, 'arg')
                f.nullable = True
                return f
            inner = self.tr(strip_const(inner_ser), env, 'arg')
            if inner.kind == 'b' and inner.name == 'unit':
                return Ref('ptr', inner=B('void'), const=bool(inner_ser.get('c')), nullable=True)
            if inner.kind == 'unmapped':
                return inner
            const = bool(inner_ser.get('c')) and not self.is_reference_type(inner)
            return Ref('ptr', inner=inner, const=const, nullable=True)
        if k in ('lr', 'rr'):
            inner_ser = ser['t']
            if inner_ser.get('k') == 'f':
                return self.tr(inner_ser, env, 'arg')
            inner = self.tr(strip_const(inner_ser), env, 'arg')
            if inner.kind == 'unmapped':
                return inner
            const = bool(inner_ser.get('c'))
            if inner.kind == 'b' and inner.name == 'unit':
                return Ref('ptr', inner=B('void'), const=const)
            if self.is_reference_type(inner):
                return Ref('ptr', inner=inner)
            if k == 'rr' or (const and use in ('param', 'ret') and self.is_small_value(inner)):
                return inner
            return Ref('ptr', inner=inner, const=const)
        if k == 'a':
            inner = self.tr(strip_const(ser['t']), env, 'arg')
            n = ser.get('n', 0)
            if n <= 0:
                return Ref('ptr', inner=inner, nullable=True, note='flexible array')
            return Ref('arr', inner=inner, n=n)
        if k == 'f':
            ps = [self.tr(p, env, 'param') for p in ser.get('ps', [])]
            r = self.tr(ser['r'], env, 'ret')
            return Ref('func', items=ps, ret=r)
        if k == 'tp':
            name = ser.get('n', 'T')
            if name in env:
                return env[name]
            return unmapped(name)
        if k == 'rec':
            return self.tr_record(ser, env, use)
        if k == 'dep':
            return self.tr_dependent(ser, env, use)
        return unmapped(ser.get('s', '?'))

    def is_small_value(self, ref):
        """Whether a `T const&` parameter or result stays `T` by value (the regions' convention: the
        immutable strings, URLs, origins, qualified names, pixel units, colors and geometry are passed
        by value; every other struct and container `T const&` is `const T*`)."""
        k = ref.kind
        if k in ('b', 'ptr', 'func', 'opt', 'span', 'tparam', 'unmapped', 'drop', 'tuple', 'fallible'):
            return True
        if k == 'arr':
            return False
        if k != 'decl':
            return True
        kind = ref.key[0]
        if kind in ('enum', 'variant', 'traits', 'inst'):
            return True
        if kind == 'ovr':
            return ref.key[2] not in ('Vector', 'HashTable', 'OrderedHashTable', 'HashMap', 'OrderedHashMap',
                                      'Checked')
        if kind == 'rec':
            r = self.m.records.get(ref.key[1]) or self.m.fwd.get(ref.key[1]) or {}
            return r.get('q') in SMALL_VALUES
        return False

    def is_reference_type(self, ref):
        """Cells and polymorphic objects are always handled by pointer."""
        if ref.kind == 'decl' and ref.key[0] in ('rec', 'inst', 'tmpl'):
            return self.m.is_poly_key(ref.key)
        return False

    def args_of(self, ser, env):
        return ser.get('args') or []

    def tr_template(self, tq, targs, ser, env, use):
        """A specialization of template `tq` with serialized args `targs`; None when not special."""
        def a(i, u='arg'):
            if i < len(targs) and targs[i].get('k') != 'v':
                return self.tr(strip_const(targs[i]), env, u)
            return unmapped(f'{tq} arg {i}')

        def aconst(i):
            return i < len(targs) and bool(targs[i].get('c'))

        if tq in DROP:
            return Ref('drop', note=spelling_of(targs[0]) if targs else '')
        if tq in NONNULL_PTR or tq in NULLABLE_PTR:
            inner = a(0)
            if inner.kind == 'unmapped':
                return inner
            nullable = tq in NULLABLE_PTR
            if inner.kind == 'ptr' or inner.kind == 'func':
                return inner
            return Ref('ptr', inner=inner, nullable=nullable,
                       const=aconst(0) and not self.is_reference_type(inner))
        if tq == 'AK::Optional':
            inner = a(0)
            if inner.kind == 'unmapped':
                return inner
            if inner.kind in ('ptr', 'func'):
                inner.nullable = True
                return inner
            if inner.kind == 'opt':
                return inner
            return Ref('opt', inner=inner)
        if tq in FALLIBLE:
            inner = a(0, 'ret')
            if use == 'ret':
                return Ref('fallible', inner=inner)
            return inner
        if tq == 'AK::Span':
            inner = a(0)
            return Ref('span', inner=inner, const=aconst(0))
        if tq in ('AK::FixedArray', 'std::initializer_list'):
            inner = a(0)
            return Ref('span', inner=inner, const=tq == 'std::initializer_list' or aconst(0))
        if tq == 'AK::Array':
            n = targs[1].get('v') if len(targs) > 1 else None
            inner = a(0)
            if isinstance(n, int) and n > 0:
                return Ref('arr', inner=inner, n=n)
            if isinstance(n, int) and n == 0:
                return Ref('span', inner=inner, note='Array<T, 0>')
            return Ref('span', inner=inner, note='Array<T, N>')
        if tq in UNWRAP_FIRST:
            return a(0, use)
        if tq == 'AK::Variant':
            items = [a(i) for i in range(len(targs))]
            if any(i.kind == 'unmapped' for i in items):
                bad = [i for i in items if i.kind == 'unmapped'][0]
                return bad
            if any(self.has_tparam(i) for i in items):
                return unmapped('Variant<' + ', '.join(spelling_of(t) for t in targs) + '> (generic)')
            key = ('variant', tuple(self.key_of(i) for i in items))
            self.m.note_variant(key, items, self.m.aliases.get(ser.get('usr', ''), []))
            return Ref('decl', key=key)
        if tq in ('AK::Tuple', 'std::pair', 'AK::Tuple'):
            items = [a(i) for i in range(len(targs))]
            if len(items) == 1:
                return items[0]
            if not items:
                return B('u8')
            return Ref('tuple', items=items)
        if tq == 'AK::Function' or tq == 'GC::Function':
            if not targs or targs[0].get('k') != 'f':
                return unmapped(tq + '<?>')
            f = targs[0]
            ps = [self.tr(p, env, 'arg') for p in f.get('ps', [])]
            r = self.tr(f['r'], env, 'arg')
            if r.kind == 'fallible':
                r = r.inner
            if len(ps) > 8 or any(p.kind in ('unmapped', 'drop') for p in ps) or r.kind == 'unmapped':
                return unmapped(spelling_of(ser))
            name = f'Function{len(ps)}'
            module = 'ak' if tq == 'AK::Function' else 'gc'
            ref = Ref('decl', key=('ovr', module, name), args=ps + [r])
            if tq == 'GC::Function':
                return ref  # a cell: always used through a pointer by the caller's GC::Ref
            return ref
        if tq in WEAK:
            inner = a(0)
            return Ref('decl', key=('ovr', 'gc', 'Weak'), args=[inner])
        if tq in VECTORS:
            return Ref('decl', key=('ovr', 'ak', 'Vector'), args=[a(0)])
        if tq in ('AK::HashMap', 'GC::RootHashMap', 'AK::OrderedHashMap'):
            kk = a(0)
            v = a(1)
            ordered = tq == 'AK::OrderedHashMap' or (len(targs) > 4 and targs[4].get('v'))
            kt = self.traits_ref(targs[2] if len(targs) > 2 else None, kk, env)
            name = 'OrderedHashMap' if ordered else 'HashMap'
            return Ref('decl', key=('ovr', 'ak', name), args=[kk, v, kt])
        if tq in ('AK::HashTable', 'AK::OrderedHashTable', 'GC::HeapHashTable'):
            kk = a(0)
            ordered = tq == 'AK::OrderedHashTable' or (len(targs) > 2 and targs[2].get('v'))
            kt = self.traits_ref(targs[1] if len(targs) > 1 else None, kk, env)
            return Ref('decl', key=('ovr', 'ak', 'OrderedHashTable' if ordered else 'HashTable'), args=[kk, kt])
        if tq == 'AK::Checked':
            return Ref('decl', key=('ovr', 'ak', 'Checked'), args=[a(0)])
        if tq.startswith('std::'):
            return unmapped(spelling_of(ser))
        return None

    def has_tparam(self, ref):
        if ref is None:
            return False
        if ref.kind == 'tparam':
            return True
        return any(self.has_tparam(x) for x in (ref.args + ref.items + [ref.inner, ref.ret]) if x is not None)

    def traits_ref(self, tser, kref, env):
        if self.has_tparam(kref) or has_unmapped(kref):
            return Ref('decl', key=('ovr', 'ak', 'DefaultTraits'), args=[kref])
        if tser is not None and tser.get('k') == 'rec' and tser.get('tq') != 'AK::Traits':
            usr = tser.get('usr')
            if usr in self.m.records:
                self.m.note_traits_record(usr, kref)
                return Ref('decl', key=('rec', usr))
        if kref.kind == 'ptr':
            return Ref('decl', key=('ovr', 'ak', 'PtrTraits'), args=[kref.inner])
        if self.has_tparam(kref) or kref.kind in ('unmapped', 'tuple', 'span', 'arr', 'opt', 'func'):
            return Ref('decl', key=('ovr', 'ak', 'DefaultTraits'), args=[kref])
        key = ('traits', self.key_of(kref))
        self.m.note_traits(key, kref)
        return Ref('decl', key=key)

    def key_of(self, ref):
        """A hashable structural key of a Ref (for variants and traits)."""
        if ref.kind == 'b':
            return ('b', ref.name)
        if ref.kind == 'decl':
            return ('d', ref.key, tuple(self.key_of(a) for a in ref.args))
        if ref.kind in ('ptr', 'opt', 'span', 'arr', 'fallible'):
            return (ref.kind, self.key_of(ref.inner), ref.const, ref.nullable, ref.n)
        if ref.kind == 'tuple':
            return ('t', tuple(self.key_of(i) for i in ref.items))
        if ref.kind == 'func':
            return ('f', tuple(self.key_of(i) for i in ref.items), self.key_of(ref.ret), ref.nullable)
        if ref.kind == 'tparam':
            return ('tp', ref.name)
        return (ref.kind, ref.note)

    def tr_record(self, ser, env, use):
        tq = ser.get('tq')
        if tq:
            r = self.tr_template(tq, ser.get('args') or [], ser, env, use)
            if r is not None:
                return r
            return self.m.template_instance_ref(ser, env, self)
        q = ser.get('q', '')
        if q.startswith('std::') or q.startswith('(anonymous)') or not q:
            return unmapped(spelling_of(ser))
        usr = ser.get('usr', '')
        if '@F@' in usr or '@FI@' in usr or '@Sa' in usr and '(lambda' in q:
            return unmapped(spelling_of(ser))
        self.m.note_q(usr, q, ser.get('file', ''))
        return Ref('decl', key=('rec', usr))

    def tr_dependent(self, ser, env, use):
        q = ser.get('q', '')
        usr = ser.get('usr', '')
        tmpl = self.m.template_by_usr(usr) or self.m.template_by_qname(q)
        s0 = re.sub(r'^(const\s+)?(typename\s+)?', '', ser.get('s', '')).split('<')[0].split('::')[-1].strip()
        if tmpl is not None and s0 and s0 != tmpl['q'].split('::')[-1]:
            tmpl = None
        if tmpl is not None:
            tq = tmpl['q']
            fake = {'k': 'rec', 'q': tq, 'tq': tq, 'tusr': tmpl['usr'], 'args': ser.get('args') or []}
            r = self.tr_template(tq, fake['args'], fake, env, use)
            if r is not None:
                return r
            return self.m.template_instance_ref(fake, env, self)
        # an injected class name inside the template itself (Rect<T> in Rect's methods)
        s = ser.get('s', '')
        m = re.match(r'^(?:const\s+)?([A-Za-z_:][A-Za-z0-9_:]*)<', s)
        if m:
            name = m.group(1)
            tmpl = self.m.template_by_short(name, q)
            if tmpl is not None:
                fake = {'k': 'rec', 'q': tmpl['q'], 'tq': tmpl['q'], 'tusr': tmpl['usr'],
                        'args': ser.get('args') or [{'k': 'tp', 'n': p['n']} for p in tmpl['tparams']
                                                    if p['k'] == 'type']}
                r = self.tr_template(tmpl['q'], fake['args'], fake, env, use)
                if r is not None:
                    return r
                return self.m.template_instance_ref(fake, env, self)
        bare = s.replace('const ', '').replace('&', '').strip()
        if bare in env:
            return env[bare]
        return unmapped(s)


def render(ref, module, names, top=True):
    """Spell a Ref in module `module`. names(key) -> (module, name) of a declaration."""
    k = ref.kind
    if k == 'b':
        return ref.name
    if k == 'decl':
        mod, name = names(ref.key)
        s = name if mod == module else f'{mod}.{name}'
        if ref.args:
            s += '[' + ', '.join(render(a, module, names, False) for a in ref.args) + ']'
        return s
    if k == 'ptr':
        inner = render(ref.inner, module, names, False)
        if ref.inner.kind in ('func', 'opt', 'span') or (ref.inner.kind == 'ptr' and ref.inner.nullable):
            # a function, optional or span pointed to reads as one only in parentheses;
            # an array does not need them (u8[4]*)
            inner = f'({inner})'
        s = ('const ' if ref.const else '') + inner + '*'
        if ref.const and (ref.inner.kind == 'ptr'):
            s = f'const ({inner})*'
        if ref.nullable:
            s += '?'
        return s
    if k == 'opt':
        inner = render(ref.inner, module, names, False)
        if ref.inner.kind in ('func', 'span', 'arr') or '?' in inner:
            inner = f'({inner})'
        return inner + '?'
    if k == 'span':
        inner = render(ref.inner, module, names, False)
        if ref.inner.kind in ('func', 'opt') or inner.endswith('?') or ref.inner.kind in ('span', 'arr'):
            inner = f'({inner})'
        return ('const ' if ref.const else '') + inner + '[]'
    if k == 'arr':
        inner = render(ref.inner, module, names, False)
        if ref.inner.kind in ('func', 'opt') or inner.endswith('?'):
            inner = f'({inner})'
        return f'{inner}[{ref.n}]'
    if k == 'tuple':
        return '(' + ', '.join(render(i, module, names, False) for i in ref.items) + ')'
    if k == 'func':
        ps = ', '.join(render(p, module, names, False) for p in ref.items)
        r = ref.ret
        rs = ' -> ' + render(r, module, names, False)
        if r.kind == 'fallible' and r.inner.kind == 'b' and r.inner.name == 'unit':
            rs = ' -> unit!'
        s = f'func({ps}){rs}'
        if ref.nullable:
            s = f'({s})?'
        elif not top:
            s = f'({s})'
        return s
    if k == 'tparam':
        return ref.name
    if k == 'fallible':
        inner = ref.inner
        if inner.kind == 'b' and inner.name == 'unit':
            return '!'
        s = render(inner, module, names, False)
        if inner.kind in ('func', 'opt') and not s.startswith('('):
            s = f'({s})'
        return s + '!'
    if k == 'unmapped':
        return 'void*?'
    if k == 'drop':
        return 'u8'
    raise ValueError(k)


def walk_refs(ref, byvalue=True, out=None):
    """Yield (key, byvalue) for each declaration a Ref names."""
    if out is None:
        out = []
    if ref is None:
        return out
    k = ref.kind
    if k == 'decl':
        out.append((ref.key, byvalue))
        key = ref.key
        if key[0] == 'ovr':
            # containers hold their elements by pointer; traits are by value (one byte)
            for a in ref.args:
                walk_refs(a, a.kind == 'decl' and a.key[0] in ('traits',) or
                          (a.kind == 'decl' and a.key[0] == 'rec' and key[2] in ('HashMap', 'OrderedHashMap',
                                                                                'HashTable', 'OrderedHashTable')
                           and a is ref.args[-1]), out)
        else:
            for a in ref.args:
                walk_refs(a, False, out)
    elif k in ('ptr', 'span'):
        walk_refs(ref.inner, False, out)
    elif k in ('opt', 'arr', 'fallible'):
        walk_refs(ref.inner, byvalue, out)
    elif k == 'tuple':
        for i in ref.items:
            walk_refs(i, byvalue, out)
    elif k == 'func':
        for i in ref.items:
            walk_refs(i, False, out)
        walk_refs(ref.ret, False, out)
    return out


def has_unmapped(ref):
    if ref is None:
        return False
    if ref.kind == 'unmapped':
        return True
    return any(has_unmapped(x) for x in ref.args + ref.items + [ref.inner, ref.ret] if x is not None)


def unmapped_notes(ref, out=None):
    if out is None:
        out = []
    if ref is None:
        return out
    if ref.kind == 'unmapped':
        out.append(ref.note)
    for x in ref.args + ref.items + [ref.inner, ref.ret]:
        if x is not None:
            unmapped_notes(x, out)
    return out


def ser_key(ser):
    return json.dumps(ser, sort_keys=True)
