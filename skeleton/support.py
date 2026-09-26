"""Hand-written generic declarations the generator places in `ak` and `gc` (the overrides file of
DESIGN.md §8.1): the container shapes of §2.6, AK's Traits as an interface, Function<> pairs, and
the ClassInfo record shared by every polymorphic hierarchy (§2.3)."""
from emit import header_box
from clangenv import PIN

OVR_AK = ['ClassInfo', 'Traits', 'PtrTraits', 'DefaultTraits', 'Vector', 'HashTableBucket', 'HashTable',
          'OrderedHashTable', 'HashMapEntry', 'HashMapEntryTraits', 'HashMap', 'OrderedHashMap', 'Checked'] + \
    [f'Function{n}' for n in range(9)]
OVR_GC = ['Weak'] + [f'Function{n}' for n in range(9)]


def function_struct(n, module):
    tps = [f'A{i}' for i in range(1, n + 1)] + ['R']
    args = ', '.join(['void*?'] + tps[:-1])
    lines = []
    if module == 'ak':
        lines.append(f'## AK::Function<R({", ".join(tps[:-1])})>: a function pointer beside its capture context '
                     f'(DESIGN.md §2.7).')
        lines.append(f'pub struct Function{n}[{", ".join(tps)}]:')
        lines.append(f'    pub var call: (func({args}) -> R)?')
        lines.append('    pub var context: void*?')
    else:
        lines.append(f'## GC::Function<R({", ".join(tps[:-1])})>: a cell holding an escaping callback and its '
                     f'captures (scanned conservatively, as LibGC does).')
        lines.append(f'pub struct Function{n}[{", ".join(tps)}]:')
        lines.append('    pub var cell_base: Cell')
        lines.append(f'    pub var function: ak.Function{n}[{", ".join(tps)}]')
    lines.append('')
    return lines


def ak_support():
    desc = ['Hand-written generic shapes the generated declarations use (DESIGN.md §2.3, §2.6, §2.7):',
            'ClassInfo, AK\'s Traits as an interface, the containers (inline capacity dropped), Checked<T>',
            f'and Function<>. Part of the skeleton generator\'s output (Ladybird {PIN}).']
    L = header_box('support - ClassInfo, Traits, containers and Function<> shapes', desc)
    L += '''## The error code of AK::Error (DESIGN.md §2.10): `ErrorOr<T>` is `T!` and fails with it.
pub let ak_error: ErrorCode = ErrorCode.package(1)

## The error code of AK::Error::from_errno; the errno is kept in a per-thread slot (DESIGN.md §2.10).
pub let ak_errno_error: ErrorCode = ErrorCode.package(2)

## Class information of one class of a polymorphic hierarchy: name, pre-order class-id range,
## size and vtable (DESIGN.md §2.3).
pub struct ClassInfo:
    pub let name: str
    pub let first_id: u16
    pub let last_id: u16
    pub let size: usize
    pub let vtable: const void*
    ## The engine's MixinOffsets of this class (DESIGN.md §2.5), or none.
    pub let mixins: const void*? = none

## AK::Traits<K> (AK/Traits.h): AK's hash and equality for a key type, as an interface a one-byte
## traits struct implements (DESIGN.md §2.6).
pub interface Traits[K]:
    func hash_of(key: const K*) -> u32
    func equals(a: const K*, b: const K*) -> bool

## AK::Traits<T*>: pointers hash by address with AK's ptr_hash.
pub struct PtrTraits[T]: Traits[T*]:
    var empty_: u8
    func hash_of(key: const (T*)*) -> u32: trap("unported: AK::Traits<T*>::hash")
    func equals(a: const (T*)*, b: const (T*)*) -> bool: return *a == *b

## AK::Traits<T> for key types the generator could not give their own traits struct.
pub struct DefaultTraits[T]: Traits[T]:
    var empty_: u8
    func hash_of(key: const T*) -> u32: trap("unported: AK::Traits<T>::hash")
    func equals(a: const T*, b: const T*) -> bool: trap("unported: AK::Traits<T>::equals")

## AK::Vector<T, N> (AK/Vector.h): size, capacity and the outline buffer; inline capacity dropped.
pub struct Vector[T]:
    pub var m_size: usize
    pub var m_capacity: usize
    pub var m_outline_buffer: T*?

## AK::HashTableBucket<T> (AK/HashTable.h).
pub struct HashTableBucket[T]:
    pub var state: u8
    pub var storage: T

## AK::HashTable<T, TT> (AK/HashTable.h).
pub struct HashTable[T, TT: Traits[T]]:
    pub var m_buckets: HashTableBucket[T]*?
    pub var m_size: usize
    pub var m_capacity: usize
    pub var m_deleted_count: usize

## AK::OrderedHashTable<T, TT> (AK/HashTable.h, IsOrdered = true).
pub struct OrderedHashTable[T, TT: Traits[T]]:
    pub var m_buckets: HashTableBucket[T]*?
    pub var m_size: usize
    pub var m_capacity: usize
    pub var m_deleted_count: usize
    pub var m_collection_head: HashTableBucket[T]*?
    pub var m_collection_tail: HashTableBucket[T]*?

## AK::HashMap<K, V>::Entry (AK/HashMap.h).
pub struct HashMapEntry[K, V]:
    pub var key: K
    pub var value: V

## AK::HashMap<K, V>::EntryTraits (AK/HashMap.h).
pub struct HashMapEntryTraits[K, V, KT: Traits[K]]: Traits[HashMapEntry[K, V]]:
    var empty_: u8
    func hash_of(key: const HashMapEntry[K, V]*) -> u32: trap("unported: AK::HashMap::EntryTraits::hash")
    func equals(a: const HashMapEntry[K, V]*, b: const HashMapEntry[K, V]*) -> bool: trap("unported: AK::HashMap::EntryTraits::equals")

## AK::HashMap<K, V, KT> (AK/HashMap.h).
pub struct HashMap[K, V, KT: Traits[K]]:
    pub var m_table: HashTable[HashMapEntry[K, V], HashMapEntryTraits[K, V, KT]]

## AK::OrderedHashMap<K, V, KT> (AK/HashMap.h, IsOrdered = true).
pub struct OrderedHashMap[K, V, KT: Traits[K]]:
    pub var m_table: OrderedHashTable[HashMapEntry[K, V], HashMapEntryTraits[K, V, KT]]

## AK::Checked<T> (AK/Checked.h).
pub struct Checked[T]:
    pub var m_value: T
    pub var m_overflow: bool
'''.split('\n')
    for n in range(9):
        L += function_struct(n, 'ak')
    return L


def gc_support():
    desc = ['Hand-written generic shapes the generated declarations use: GC::Weak<T> and GC::Function<>',
            f'(DESIGN.md §2.6). Part of the skeleton generator\'s output (Ladybird {PIN}).']
    L = header_box('support - Weak<T> and GC::Function<> shapes', desc)
    L += ['## GC::Weak<T> (LibGC/Weak.h): a weak reference through a WeakImpl.',
          'pub struct Weak[T]:', '    pub var m_impl: void*?', '']
    for n in range(9):
        L += function_struct(n, 'gc')
    return L


def web_support(mixins=()):
    desc = ['The allocation seam every generated realm_create_<class> helper calls (DESIGN.md §2.3, §3.2):',
            'allocate a zeroed cell of a class from the realm\'s heap under DeferGC and set its vtable and',
            'class id; then run the virtual initialize(realm). Region r14 ports these with the heap.',
            f'Part of the skeleton generator\'s output (Ladybird {PIN}).']
    L = header_box('support - cell allocation for the generated creation helpers', desc)
    L += ['## A pending WebIDL exception (`WebIDL::ExceptionOr<T>` is `T!`): the exception itself is stored in',
          '## the VM\'s pending-exception slot before failing (DESIGN.md §2.10).',
          'pub let webidl_exception: ErrorCode = ErrorCode.package(1)', '',
          '## A pending JS completion (`JS::ThrowCompletionOr<T>` is `T!`), stored in the same slot (§2.10).',
          'pub let js_exception: ErrorCode = ErrorCode.package(2)', '',
          ] + mixin_lines(mixins) + [
          '## Allocate a zeroed cell of class `info` in `realm`\'s heap (DeferGC held), vtable and class id set.',
          'pub func web_allocate_cell(realm: Realm*, info: const ak.ClassInfo*) -> gc.Cell*:',
          '    trap("unported: web_allocate_cell (JS::Realm::create / GC::Heap::allocate)")', '',
          '## Run the virtual Cell::initialize(realm) of a freshly constructed cell.',
          'pub func web_initialize_cell(cell: gc.Cell*, realm: Realm*):',
          '    trap("unported: web_initialize_cell (JS::Cell::initialize)")', '']
    return L


def mixin_lines(mixins):
    """MixinOffsets and one as_if_<mixin> per stateful mixin (DESIGN.md §2.5).

    mixins: [(field key, Luce type name, C++ name)]"""
    if not mixins:
        return []
    L = ['## Offsets of the stateful mixins (secondary bases) inside a cell class; a class that does not',
         '## mix one in leaves it none (DESIGN.md §2.5).', 'pub struct MixinOffsets:']
    for key, tname, q in mixins:
        L.append(f'    pub let {key}: usize?')
    L.append('')
    for key, tname, q in mixins:
        L += [f'## as_if<{q}>: the mixin inside a cell, found through its class\'s MixinOffsets.',
              f'pub func as_if_{key}[P: gc.IsCell](p: P*) -> {tname}*?:',
              '    let info = p.cell().vtable.class_info',
              '    let mixins = (const MixinOffsets*)(info.mixins else return none)',
              f'    let offset = mixins.{key} else return none',
              f'    return ({tname}*)((u8*)p + offset)', '']
    return L
