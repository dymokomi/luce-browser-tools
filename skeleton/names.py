"""Name mapping of DESIGN.md §4.2: C++ names to Luce names."""
import re

RESERVED = set('''alloc and as asm break catch const continue defer elif else enum errdefer
export extern false for free from func goto if import in interface
let local match mutating new none not or pub recover return self static struct test
true try type union var volatile while with class spawn'''.split())
CORE = set('''assert discard error trap hash print format sizeof alignof offsetof hex bin pad
bool i8 i16 i32 i64 isize u8 u16 u32 u64 usize f16 f32 f64 char str
unit never void fmt Error ErrorCode'''.split())
# Standard module names that bind where imported; avoid them as parameter names anyway.
STD_MODULES = {'memory', 'io', 'os', 'thread', 'sync', 'atomic', 'c', 'testing', 'runtime', 'luce'}

# C++ namespace -> (type prefix, function prefix) in the engine's `web` module.
ENGINE_PREFIX = {
    'Web': ('', ''), 'Web::DOM': ('Dom', 'dom_'), 'Web::HTML': ('Html', 'html_'),
    'Web::CSS': ('Css', 'css_'), 'Web::CSS::Parser': ('Css', 'css_'), 'Web::Layout': ('Layout', 'layout_'),
    'Web::Painting': ('Paint', 'paint_'), 'Web::SVG': ('Svg', 'svg_'), 'Web::Fetch': ('Fetch', 'fetch_'),
    'Web::Fetch::Infrastructure': ('Fetch', 'fetch_'), 'Web::Bindings': ('Bind', 'bind_'),
    'Web::WebIDL': ('Idl', 'idl_'), 'Web::MimeSniff': ('Mime', 'mime_'), 'Web::Platform': ('Platform', 'platform_'),
    'JS': ('Js', 'js_'),
}
ENGINE_UNPREFIXED = {'JS::Realm': 'Realm', 'JS::VM': 'Vm'}

WORD = re.compile(r'[A-Z]+(?=[A-Z][a-z])|[A-Z]?[a-z]+|[A-Z]+|\d+')


def words(identifier):
    out = []
    for part in identifier.split('_'):
        out.extend(WORD.findall(part))
    return [w for w in out if w]


def pascal(identifier):
    """PascalCase with acronyms as words: HTMLElement -> HtmlElement, UTF16 -> Utf16."""
    ws = words(identifier)
    if not ws:
        return identifier
    out = []
    for w in ws:
        out.append(w[0].upper() + w[1:].lower() if w.isupper() or w[0].isupper() else w[0].upper() + w[1:])
    return ''.join(out)


def snake(identifier):
    """snake_case: HtmlElement -> html_element, BackgroundColor -> background_color, INVALID -> invalid."""
    if '_' in identifier and identifier.lower() == identifier:
        return identifier.strip('_') or identifier
    ws = words(identifier)
    if not ws:
        return identifier.lower()
    # keep digits attached to the previous word: utf16, h1
    merged = []
    for w in ws:
        if w.isdigit() and merged:
            merged[-1] += w
        else:
            merged.append(w)
    return '_'.join(w.lower() for w in merged)


def safe(name):
    """A declaration name that is neither reserved nor core."""
    if name in RESERVED or name in CORE or name in STD_MODULES or name == '_':
        return name + '_'
    if name and name[0].isdigit():
        return 'n' + name
    return name


def field_name(name):
    if name in RESERVED:
        return name + '_'
    return name


def split_qname(q):
    return [p for p in q.split('::') if p]


def namespace_and_name(q):
    parts = split_qname(q)
    return '::'.join(parts[:-1]), parts[-1] if parts else q


OPERATOR_NAMES = {
    '+': 'add', '-': 'sub', '*': 'mul', '/': 'div', '%': 'rem', '==': 'eq', '!=': 'ne', '<': 'lt', '<=': 'le',
    '>': 'gt', '>=': 'ge', '<=>': 'cmp', '[]': 'at', '()': 'call', '+=': 'add_assign', '-=': 'sub_assign',
    '*=': 'mul_assign', '/=': 'div_assign', '%=': 'rem_assign', '<<': 'shl', '>>': 'shr', '&': 'bit_and',
    '|': 'bit_or', '^': 'bit_xor', '~': 'bit_not', '!': 'not_', '&&': 'and_', '||': 'or_', '++': 'inc',
    '--': 'dec', '->': 'arrow', '<<=': 'shl_assign', '>>=': 'shr_assign', '&=': 'and_assign', '|=': 'or_assign',
    '^=': 'xor_assign', ',': 'comma', '""': 'literal', '=': 'assign',
}


def method_word(name, nparams_nonthis=None):
    """The Luce word for a C++ member name (operators become named methods, §2.8)."""
    if name.startswith('operator'):
        op = name[len('operator'):].strip()
        if op in ('-',) and nparams_nonthis == 0:
            return 'neg'
        if op == '*' and nparams_nonthis == 0:
            return 'deref'
        if op == '&' and nparams_nonthis == 0:
            return 'address_of'
        if op in OPERATOR_NAMES:
            return OPERATOR_NAMES[op]
        if op.startswith('""'):
            return 'literal' + snake(op[2:].strip())
        op = re.sub(r'type-parameter-\d+-\d+', 'T', op)
        # conversion operator: operator Foo
        return 'to_' + snake(re.sub(r'[^A-Za-z0-9_]+', '_', op).strip('_'))
    return name
