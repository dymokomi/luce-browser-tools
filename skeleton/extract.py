#!/usr/bin/env python3
"""Extract declarations from the reference Ladybird build into model.json.

Parses every translation unit of AK, LibGC, LibURL, LibTextCodec, LibUnicode, LibGfx and LibWeb
with the exact flags of the reference build's compile_commands.json (libclang, LLVM 21), walks
the cursors located in the donor tree, and records classes, enums, typedefs, functions, methods,
variables and template information, de-duplicated by USR across TUs.

    python3 extract.py [-j N] [--only SUBSTRING]      # writes ../cache/model.json
"""
import argparse
import json
import multiprocessing
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from clangenv import ci, CACHE, compile_commands, clang_args, rel  # noqa: E402

K = ci.CursorKind
import re
TPARM = re.compile(r'type-parameter-\d+-\d+')
T = ci.TypeKind

TU_LIBS = ('/AK/', '/LibGC/', '/LibURL/', '/LibTextCodec/', '/LibUnicode/', '/LibGfx/', '/LibWeb/')

BUILTIN = {
    T.VOID: 'void', T.BOOL: 'bool', T.CHAR_U: 'u8', T.UCHAR: 'u8', T.CHAR16: 'u16',
    T.CHAR32: 'u32', T.USHORT: 'u16', T.UINT: 'u32', T.ULONG: 'u64', T.ULONGLONG: 'u64',
    T.UINT128: 'u128', T.CHAR_S: 'char', T.SCHAR: 'i8', T.WCHAR: 'u32', T.SHORT: 'i16',
    T.INT: 'i32', T.LONG: 'i64', T.LONGLONG: 'i64', T.INT128: 'i128', T.FLOAT: 'f32',
    T.DOUBLE: 'f64', T.LONGDOUBLE: 'f64', T.NULLPTR: 'nullptr', T.HALF: 'f16',
    T.FLOAT16: 'f16',
}
if hasattr(T, 'CHAR8'):
    BUILTIN[T.CHAR8] = 'u8'
SIZE_TYPEDEFS = {'size_t': 'usize', 'ssize_t': 'isize', 'uintptr_t': 'usize', 'intptr_t': 'isize',
                 'ptrdiff_t': 'isize', 'FlatPtr': 'usize', '__size_t': 'usize'}

RECORD_KINDS = {K.CLASS_DECL: 'class', K.STRUCT_DECL: 'struct', K.UNION_DECL: 'union',
                K.CLASS_TEMPLATE: 'template', K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION: 'partial'}
FUNC_KINDS = (K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR, K.CONVERSION_FUNCTION,
              K.FUNCTION_TEMPLATE)

_lines_cache = {}


def source_line(path, line):
    lines = _lines_cache.get(path)
    if lines is None:
        try:
            with open(path, errors='replace') as f:
                lines = f.read().split('\n')
        except OSError:
            lines = []
        _lines_cache[path] = lines
    if 0 < line <= len(lines):
        return lines[line - 1]
    return ''


def qname(c):
    parts = []
    while c is not None and c.kind != K.TRANSLATION_UNIT:
        if c.kind in (K.NAMESPACE,) or c.kind in RECORD_KINDS or c.kind == K.ENUM_DECL:
            parts.append(c.spelling or '(anonymous)')
        elif c.kind == K.LINKAGE_SPEC:
            pass
        elif c.kind in FUNC_KINDS:
            parts.append(c.spelling)
        c = c.semantic_parent
    return '::'.join(reversed(parts))


def loc_of(c):
    f = c.location.file
    if f is None:
        return '', 0
    return rel(f.name), c.location.line


class Extractor:
    def __init__(self):
        self.records = {}
        self.enums = {}
        self.typedefs = {}
        self.functions = {}
        self.defs = {}
        self.vars = {}
        self.memo = {}

    # ---- types -----------------------------------------------------------------------------
    def ser(self, t, depth=0):
        key = (t.kind.value, t.spelling, t.get_canonical().spelling)
        m = self.memo.get(key)
        if m is not None:
            return m
        r = self._ser(t, depth)
        self.memo[key] = r
        return r

    def _ser(self, t, depth):
        if depth > 40:
            return {'k': 'x', 's': t.spelling}
        k = t.kind
        const = t.is_const_qualified()
        r = None
        if k == T.ELABORATED:
            r = dict(self.ser(t.get_named_type(), depth + 1))
        elif k == T.TYPEDEF:
            d = t.get_declaration()
            if d.spelling in SIZE_TYPEDEFS:
                r = {'k': 'b', 'n': SIZE_TYPEDEFS[d.spelling]}
            else:
                u = d.underlying_typedef_type
                if u.kind == T.INVALID:
                    r = dict(self.ser(t.get_canonical(), depth + 1))
                else:
                    r = dict(self.ser(u, depth + 1))
                    if r.get('k') in ('rec', 'dep') and 'alias' not in r:
                        r['alias'] = (qname(d.semantic_parent) + '::' if d.semantic_parent is not None and
                                      d.semantic_parent.kind != K.TRANSLATION_UNIT else '') + d.spelling
        elif k in BUILTIN:
            r = {'k': 'b', 'n': BUILTIN[k]}
        elif k == T.POINTER:
            r = {'k': 'p', 't': self.ser(t.get_pointee(), depth + 1)}
        elif k == T.LVALUEREFERENCE:
            r = {'k': 'lr', 't': self.ser(t.get_pointee(), depth + 1)}
        elif k == T.RVALUEREFERENCE:
            r = {'k': 'rr', 't': self.ser(t.get_pointee(), depth + 1)}
        elif k == T.RECORD:
            r = self.ser_record(t, depth)
        elif k == T.ENUM:
            d = t.get_declaration()
            r = {'k': 'e', 'q': qname(d), 'usr': d.get_usr()}
        elif k == T.CONSTANTARRAY:
            r = {'k': 'a', 't': self.ser(t.element_type, depth + 1), 'n': t.element_count}
        elif k in (T.INCOMPLETEARRAY, T.VARIABLEARRAY, T.DEPENDENTSIZEDARRAY):
            r = {'k': 'a', 't': self.ser(t.element_type, depth + 1), 'n': 0}
        elif k == T.FUNCTIONPROTO or k == T.FUNCTIONNOPROTO:
            ps = [self.ser(a, depth + 1) for a in t.argument_types()] if k == T.FUNCTIONPROTO else []
            r = {'k': 'f', 'r': self.ser(t.get_result(), depth + 1), 'ps': ps}
        elif k in (T.AUTO, T.UNEXPOSED, T.DEPENDENT, T.ATTRIBUTED):
            can = t.get_canonical()
            if TPARM.fullmatch(can.spelling.replace('const ', '').strip()):
                r = {'k': 'tp', 'n': t.spelling.replace('const ', '').replace('&', '').strip(),
                     'idx': can.spelling.replace('const ', '').strip()}
            elif can.kind not in (T.UNEXPOSED, T.DEPENDENT, T.AUTO) and can.kind != k:
                r = dict(self.ser(can, depth + 1))
            else:
                r = self.ser_dependent(t, depth)
        elif k == T.MEMBERPOINTER:
            r = {'k': 'x', 's': t.spelling}
        else:
            can = t.get_canonical()
            if can.kind != k and can.kind != T.INVALID:
                r = dict(self.ser(can, depth + 1))
            else:
                r = {'k': 'x', 's': t.spelling, 'kind': k.name}
        if const:
            r = dict(r)
            r['c'] = 1
        return r

    def ser_dependent(self, t, depth):
        r = {'k': 'dep', 's': t.spelling}
        n = t.get_num_template_arguments()
        d = t.get_declaration()
        if d.kind != K.NO_DECL_FOUND:
            r['q'] = qname(d)
            r['usr'] = d.get_usr()
        if n > 0:
            args = []
            for i in range(n):
                a = t.get_template_argument_type(i)
                if a.kind == T.INVALID:
                    args.append({'k': 'v'})
                else:
                    args.append(self.ser(a, depth + 1))
            r['args'] = args
        return r

    def ser_record(self, t, depth):
        d = t.get_declaration()
        r = {'k': 'rec', 'q': qname(d), 'usr': d.get_usr()}
        tmpl = d.specialized_template
        if tmpl is not None and tmpl.kind != K.NO_DECL_FOUND and tmpl != d:
            r['tq'] = qname(tmpl)
            r['tusr'] = tmpl.get_usr()
            args = []
            n = t.get_num_template_arguments()
            if n < 0:
                n = 0
            for i in range(n):
                a = t.get_template_argument_type(i)
                if a.kind == T.INVALID:
                    args.append({'k': 'v'})
                else:
                    args.append(self.ser(a, depth + 1))
            # Non-type arguments: values from the specialization cursor.
            cn = d.get_num_template_arguments()
            if cn == len(args):
                for i in range(cn):
                    kind = d.get_template_argument_kind(i)
                    if kind.name == 'INTEGRAL' and args[i].get('k') == 'v':
                        args[i] = {'k': 'v', 'v': d.get_template_argument_value(i)}
            r['args'] = args
        f, _ = loc_of(d)
        r['file'] = f
        return r

    # ---- declarations ----------------------------------------------------------------------
    def walk(self, cursor, in_record=None):
        for c in cursor.get_children():
            k = c.kind
            if k in (K.NAMESPACE, K.LINKAGE_SPEC, K.UNEXPOSED_DECL):
                f, _ = loc_of(c)
                if f and not f.startswith('Build/vcpkg'):
                    self.walk(c)
                continue
            f, line = loc_of(c)
            if not f or f.startswith('Build/vcpkg'):
                continue
            try:
                if k in RECORD_KINDS:
                    self.record(c, f, line)
                elif k == K.ENUM_DECL:
                    self.enum(c, f, line)
                elif k in (K.TYPEDEF_DECL, K.TYPE_ALIAS_DECL):
                    self.typedef(c, f, line)
                elif k in FUNC_KINDS:
                    self.function(c, f, line)
                elif k == K.VAR_DECL:
                    self.var(c, f, line)
            except Exception as e:  # keep going; record the failure
                sys.stderr.write(f'extract: {f}:{line}: {c.spelling}: {e!r}\n')

    def record(self, c, f, line):
        usr = c.get_usr()
        if not c.is_definition():
            if usr not in self.records:
                self.records.setdefault('fwd:' + usr, {'usr': usr, 'q': qname(c), 'fwd': 1, 'file': f,
                                                        'line': line})
            return
        if usr in self.records:
            # still walk out-of-line things? members are only in the definition
            return
        rec = {
            'usr': usr, 'q': qname(c), 'kind': RECORD_KINDS[c.kind], 'file': f, 'line': line,
            'anon': 1 if c.is_anonymous() else 0,
            'bases': [], 'fields': [], 'methods': [], 'statics': [], 'nested': [],
            'tparams': [], 'macros': [], 'abstract': 1 if c.is_abstract_record() else 0,
            'end': c.extent.end.line,
        }
        parent = c.semantic_parent
        if parent is not None and parent.kind in RECORD_KINDS:
            rec['parent'] = parent.get_usr()
        tmpl = c.specialized_template
        if tmpl is not None and tmpl.kind != K.NO_DECL_FOUND:
            rec['spec_of'] = tmpl.get_usr()
            rec['spec_q'] = qname(tmpl)
            try:
                rec['spec_args'] = self.ser_record(c.type, 0).get('args', [])
            except Exception:
                rec['spec_args'] = []
        if c.kind not in (K.CLASS_TEMPLATE, K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
            try:
                sz = c.type.get_size()
                if sz > 0:
                    rec['size'] = sz
            except Exception:
                pass
        self.records[usr] = rec
        access = 'private' if c.kind == K.CLASS_DECL or c.kind == K.CLASS_TEMPLATE else 'public'
        for ch in c.get_children():
            try:
                access = self.record_child(rec, ch, f, access)
            except Exception as e:
                sys.stderr.write(f'extract: {f}: member {ch.spelling} of {rec["q"]}: {e!r}\n')

    def record_child(self, rec, ch, f, access):
        if True:
            k = ch.kind
            if k == K.CXX_ACCESS_SPEC_DECL:
                return ch.access_specifier.name.lower()
            fch, lch = loc_of(ch)
            if fch and fch == f and lch:
                src = source_line(os.path.join(DONOR_ROOT(fch)), lch)
                mac = macro_on_line(src)
                if mac and mac not in rec['macros']:
                    rec['macros'].append(mac)
            if k == K.CXX_BASE_SPECIFIER:
                rec['bases'].append({'t': self.ser(ch.type), 'access': ch.access_specifier.name.lower(),
                                     'virtual': 1 if ch.is_virtual_base() else 0,
                                     'usr': ch.referenced.get_usr() if ch.referenced else ''})
            elif k == K.FIELD_DECL:
                fd = {'n': ch.spelling, 't': self.ser(ch.type), 'access': access, 'line': lch}
                if ch.is_bitfield():
                    fd['bits'] = ch.get_bitfield_width()
                init = default_init(ch)
                if init:
                    fd['init'] = init
                if ch.is_mutable_field():
                    fd['mutable'] = 1
                rec['fields'].append(fd)
            elif k == K.VAR_DECL:
                rec['statics'].append(ch.get_usr())
                self.var(ch, fch or f, lch)
            elif k in FUNC_KINDS:
                rec['methods'].append(ch.get_usr())
                self.function(ch, fch or f, lch, access=access, macro=macro_on_line(
                    source_line(DONOR_ROOT(fch), lch)) if fch else '')
            elif k in RECORD_KINDS:
                if ch.is_definition():
                    rec['nested'].append(ch.get_usr())
                    self.record(ch, fch or f, lch)
                    sub = self.records.get(ch.get_usr())
                    if sub is not None:
                        sub['access'] = access
                        # an anonymous struct/union member: find the implicit field that holds it
            elif k == K.ENUM_DECL:
                rec['nested'].append(ch.get_usr())
                self.enum(ch, fch or f, lch)
            elif k in (K.TYPEDEF_DECL, K.TYPE_ALIAS_DECL):
                self.typedef(ch, fch or f, lch)
            elif k == K.TEMPLATE_TYPE_PARAMETER:
                rec['tparams'].append({'k': 'type', 'n': ch.spelling})
            elif k == K.TEMPLATE_NON_TYPE_PARAMETER:
                rec['tparams'].append({'k': 'value', 'n': ch.spelling, 't': ch.type.spelling})
            elif k == K.TEMPLATE_TEMPLATE_PARAMETER:
                rec['tparams'].append({'k': 'template', 'n': ch.spelling})
        return access

    def enum(self, c, f, line):
        usr = c.get_usr()
        if usr in self.enums or not c.is_definition():
            return
        e = {'usr': usr, 'q': qname(c), 'file': f, 'line': line, 'scoped': 1 if c.is_scoped_enum() else 0,
             'under': self.ser(c.enum_type), 'cases': [], 'anon': 1 if c.is_anonymous() else 0}
        parent = c.semantic_parent
        if parent is not None and parent.kind in RECORD_KINDS:
            e['parent'] = parent.get_usr()
        for ch in c.get_children():
            if ch.kind == K.ENUM_CONSTANT_DECL:
                e['cases'].append([ch.spelling, ch.enum_value])
        self.enums[usr] = e

    def typedef(self, c, f, line):
        usr = c.get_usr()
        if usr in self.typedefs:
            return
        try:
            u = self.ser(c.underlying_typedef_type)
        except Exception:
            return
        td = {'usr': usr, 'q': qname(c.semantic_parent) + '::' + c.spelling if c.semantic_parent and
              c.semantic_parent.kind != K.TRANSLATION_UNIT else c.spelling, 'file': f, 'line': line, 't': u}
        self.typedefs[usr] = td

    def function(self, c, f, line, access=None, macro=''):
        usr = c.get_usr()
        if c.is_definition():
            if usr not in self.defs:
                d = {'usr': usr, 'file': f, 'line': line, 'pnames': [], 'end': c.extent.end.line}
                for ch in c.get_children():
                    if ch.kind == K.PARM_DECL:
                        d['pnames'].append(ch.spelling)
                    elif ch.kind == K.COMPOUND_STMT:
                        toks = []
                        for i, tok in enumerate(ch.get_tokens()):
                            if i > 48:
                                toks = None
                                break
                            toks.append(tok.spelling)
                        if toks is not None:
                            d['body'] = toks
                        d['body_line'] = ch.extent.start.line
                self.defs[usr] = d
        if usr in self.functions:
            fn = self.functions[usr]
            if access and not fn.get('access'):
                fn['access'] = access
            return
        fn = {'usr': usr, 'q': qname(c), 'n': c.spelling, 'kind': c.kind.name, 'file': f, 'line': line,
              'params': [], 'tparams': []}
        if macro:
            fn['macro'] = macro
        if access:
            fn['access'] = access
        parent = c.semantic_parent
        if parent is not None and parent.kind in RECORD_KINDS:
            fn['parent'] = parent.get_usr()
        if c.kind != K.FUNCTION_TEMPLATE or True:
            try:
                rt = c.result_type
                fn['ret'] = self.ser(rt)
            except Exception:
                fn['ret'] = {'k': 'x', 's': '?'}
        flags = []
        if c.kind in (K.CXX_METHOD, K.CONVERSION_FUNCTION, K.FUNCTION_TEMPLATE, K.CONSTRUCTOR,
                      K.DESTRUCTOR):
            try:
                if c.is_virtual_method():
                    flags.append('virtual')
                if c.is_pure_virtual_method():
                    flags.append('pure')
                if c.is_static_method():
                    flags.append('static')
                if c.is_const_method():
                    flags.append('const')
                if c.is_default_method():
                    flags.append('default')
                if c.is_deleted_method():
                    flags.append('deleted')
                if c.kind == K.CONSTRUCTOR:
                    if c.is_copy_constructor():
                        flags.append('copy')
                    if c.is_move_constructor():
                        flags.append('move')
                if c.is_copy_assignment_operator_method() or c.is_move_assignment_operator_method():
                    flags.append('assign')
            except Exception:
                pass
        if c.storage_class == ci.StorageClass.STATIC:
            flags.append('internal' if 'parent' not in fn else 'static')
        if c.linkage == ci.LinkageKind.INTERNAL:
            if 'internal' not in flags:
                flags.append('internal')
        for ch in c.get_children():
            if ch.kind == K.PARM_DECL:
                p = {'n': ch.spelling, 't': self.ser(ch.type)}
                dflt = default_init(ch)
                if dflt:
                    p['d'] = dflt
                fn['params'].append(p)
            elif ch.kind == K.CXX_OVERRIDE_ATTR:
                flags.append('override')
            elif ch.kind == K.CXX_FINAL_ATTR:
                flags.append('final')
            elif ch.kind == K.TEMPLATE_TYPE_PARAMETER:
                fn['tparams'].append({'k': 'type', 'n': ch.spelling})
            elif ch.kind == K.TEMPLATE_NON_TYPE_PARAMETER:
                fn['tparams'].append({'k': 'value', 'n': ch.spelling})
            elif ch.kind == K.TEMPLATE_TEMPLATE_PARAMETER:
                fn['tparams'].append({'k': 'template', 'n': ch.spelling})
        try:
            if c.type.kind == T.FUNCTIONPROTO and c.type.is_function_variadic():
                flags.append('variadic')
        except Exception:
            pass
        fn['flags'] = flags
        self.functions[usr] = fn

    def var(self, c, f, line):
        usr = c.get_usr()
        if usr in self.vars:
            return
        v = {'usr': usr, 'q': qname(c.semantic_parent) + '::' + c.spelling if c.semantic_parent and
             c.semantic_parent.kind != K.TRANSLATION_UNIT else c.spelling,
             'n': c.spelling, 'file': f, 'line': line, 't': self.ser(c.type)}
        parent = c.semantic_parent
        if parent is not None and parent.kind in RECORD_KINDS:
            v['parent'] = parent.get_usr()
        if c.linkage == ci.LinkageKind.INTERNAL:
            v['internal'] = 1
        init = default_init(c)
        if init:
            v['init'] = init
        toks = [t.spelling for t in c.get_tokens()][:6]
        if 'constexpr' in toks:
            v['constexpr'] = 1
        self.vars[usr] = v


def DONOR_ROOT(relpath):
    from clangenv import DONOR, BUILD
    if relpath.startswith('Build/'):
        return BUILD + '/' + relpath[6:]
    return DONOR + '/' + relpath


MACROS = ('GC_CELL', 'WEB_PLATFORM_OBJECT', 'WEB_NON_IDL_PLATFORM_OBJECT', 'JS_OBJECT', 'GC_DECLARE_ALLOCATOR',
          'JS_CELL', 'JS_ENVIRONMENT', 'JS_PROTOTYPE_OBJECT', 'AK_MAKE_NONCOPYABLE', 'AK_MAKE_NONMOVABLE',
          'AK_MAKE_DEFAULT_MOVABLE', 'AK_MAKE_DEFAULT_COPYABLE', 'LAYOUT_NODE', 'GC_DEFINE_ALLOCATOR')


def macro_on_line(src):
    s = src.strip()
    for m in MACROS:
        if s.startswith(m + '('):
            return m
    return ''


def default_init(c):
    """Tokens after '=' (or inside a brace initializer) in a field/param/var declaration."""
    try:
        toks = [t.spelling for t in c.get_tokens()]
    except Exception:
        return ''
    if len(toks) > 200:
        return ''
    depth = 0
    for i, t in enumerate(toks):
        if t in ('(', '<', '['):
            depth += 1
        elif t in (')', '>', ']'):
            depth -= 1
        elif t == '=' and depth <= 0:
            return ' '.join(toks[i + 1:])
        elif t == '{' and depth <= 0 and i > 0 and toks[i - 1] == c.spelling:
            return ' '.join(toks[i:])
    return ''


def run_chunk(args):
    files, index = args
    ex = Extractor()
    idx = ci.Index.create()
    cc = {e['file']: e for e in compile_commands()}
    n = 0
    for path in files:
        e = cc[path]
        t0 = time.time()
        try:
            tu = idx.parse(path, args=clang_args(e))
        except Exception as err:
            sys.stderr.write(f'parse failed {path}: {err}\n')
            continue
        errors = [d for d in tu.diagnostics if d.severity >= 3]
        for d in errors[:3]:
            sys.stderr.write(f'diag {rel(path)}: {d}\n')
        ex.walk(tu.cursor)
        n += 1
        del tu
        sys.stderr.write(f'[{index}] {rel(path)} {time.time() - t0:.1f}s\n')
    out = CACHE + f'/chunk_{index:03d}.json'
    with open(out, 'w') as f:
        json.dump({'records': ex.records, 'enums': ex.enums, 'typedefs': ex.typedefs,
                   'functions': ex.functions, 'defs': ex.defs, 'vars': ex.vars}, f)
    return out


def merge(paths):
    model = {'records': {}, 'enums': {}, 'typedefs': {}, 'functions': {}, 'defs': {}, 'vars': {}}
    for p in paths:
        with open(p) as f:
            part = json.load(f)
        for key in model:
            dst = model[key]
            for usr, v in part[key].items():
                if key == 'records' and usr.startswith('fwd:'):
                    if usr[4:] in dst or usr in dst:
                        continue
                if key == 'records' and not usr.startswith('fwd:'):
                    dst.pop('fwd:' + usr, None)
                if key == 'functions' and usr in dst:
                    old = dst[usr]
                    # prefer the declaration that knows more (override attrs appear in class bodies)
                    if len(v.get('flags', [])) > len(old.get('flags', [])):
                        dst[usr] = v
                    continue
                if usr not in dst:
                    dst[usr] = v
    return model


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-j', type=int, default=max(1, os.cpu_count() - 2))
    ap.add_argument('--only', default='')
    args = ap.parse_args()
    os.makedirs(CACHE, exist_ok=True)
    for old in os.listdir(CACHE):
        if old.startswith('chunk_'):
            os.remove(os.path.join(CACHE, old))
    files = [e['file'] for e in compile_commands()
             if any(lib in e['file'] for lib in TU_LIBS) and e['file'].endswith('.cpp')
             and args.only in e['file'] and '/Build/' not in e['file'].replace('/Build/release/Libraries/LibWeb/', '/')
             and not e['file'].endswith(('Skia.cpp',))]
    # generated LibWeb sources (bindings) are not parsed as TUs; their headers are seen through includes
    files = [f for f in files if '/Build/release/' not in f]
    files.sort()
    n = args.j * 4
    chunks = [(files[i::n], i) for i in range(n)]
    chunks = [c for c in chunks if c[0]]
    t0 = time.time()
    with multiprocessing.Pool(args.j) as pool:
        outs = pool.map(run_chunk, chunks, chunksize=1)
    model = merge(outs)
    with open(CACHE + '/model.json', 'w') as f:
        json.dump(model, f)
    print(f'{len(files)} TUs in {time.time() - t0:.0f}s: ' +
          ', '.join(f'{k} {len(v)}' for k, v in model.items()))


if __name__ == '__main__':
    main()
