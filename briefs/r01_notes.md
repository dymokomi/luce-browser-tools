# r01 (ak_core) final report — source for docs/regions/r01.md

Final commit d8a7e64 on port/r01. Stub fragments `ak/stub_r01_ak_core.lucb` and `gc/stub_r01_ak_core.lucb` deleted. 275 ak tests pass; test.sh fails on any -W output (because `check -W` exits 0 on warnings).

## What other regions need to know
- Imports: module-wide imports live in `ak/module.lucb` (io, memory, Allocator, luce, os, math, math32, `atomic as atomics`, thread, testing). A fragment may not re-import any of these.
- Allocation hook (DESIGN §3.3/§4.1.1): `ak.atomic_allocator` is a `local var` of type `Allocator?` that gc installs; `kmalloc_atomic` and `alloc_atomic` use it. kmalloc/kfree/krealloc allocate from the current allocator with a size header. `vector_create_atomic[T]()` makes a Vector whose buffer is atomic storage.
- Arithmetic templates suffixed per type (generics can't do arithmetic): `ceil_div_usize`, `saturating_add_i32`, `exp2_usize`, `sqrt_f64`, `clamp_to_i32_f64`, … Checked: `checked_add_usize`, `add_i64`, generic `checked_value(&c)`, `is_within_range_u32`. NumericLimits are constants (`i32_max`, `f64_epsilon`). `min`/`max`/`clamp` generic over Comparable; `min_f64` etc. use `<` for NaN safety.
- Callbacks: predicates/callbacks are `FunctionN` values; `function1_call`, `functionN_is_set`, `functionN_clear` added. Generic callbacks must be static methods of generic structs.
- Vector: `vector_span(&v)` is what `for` walks; `vector_at` -> `T*`, `vector_get` -> `T*?`; struct gained `m_atomic`, conforms to `IndexedContainer[T]`; `vector_clone` = copy ctor, `vector_move_assign` = move assignment; moving overloads (`vector_extend`, `vector_prepend_vector`) take `Vector[T]*` and empty the source.
- HashTable/HashMap: HashTable has `m_mask`, `m_head`/`m_tail`, `m_ordered`; every bucket has links and a stored hash. OrderedHashTable wraps a HashTable; OrderedHashMap used through `ordered_hash_map_*`/`ordered_hash_table_*`. Iterators are `HashTableIterator[T]` (for-walkable). `hash_map_get_k` -> `V*?`, `hash_map_take_k` -> `V?`, `hash_map_clone` fallible. Hash-compatible lookups take `HashCompatibleTraits[T, K]`. BucketState `as u8`.
- Traits: integer traits filled; PtrTraits uses ptr_hash; traits.lucb adds U8/U16/I8/I16/Usize/Isize/Bool/Char/F32/F64 and IdentityHashTraits. FlyString/String/Utf16String traits left to r02. DefaultTraits still traps.
- Errors: string-literal ErrorOr failure is `error(ak_error, "…")`; errno/syscall errors `ak_error_throw(ak_error_from_errno(code))` (stores in `ak_pending_error`, fails with ak_errno_error); `ak_error_from_failure(failure)` recovers the AkError; errno constants `enomem`, `einval`, `enoent`, `eintr`, `ebadf`, and functions `eagain()`, `enotconn()`, `emsgsize()`.
- Assertions: messages are `str`; `custom_assertion_handler` replaces the donor's weak symbol.
- Hand-fixed generated declarations in many `types_*.lucb` (noted in headers): Types all headers, iterator types with element type as only parameter, ByteBuffer inline storage restored, stream vtables `usize!`, FixedPoint `[TUnderlying, TPrecision]`, `types_external` Timespec/Timeval, `types_big_int_base` IntegerWrapper.
- r04: `weakable_make_weak_ptr` (in gc, new `gc/weakable.lucb`) stores an `ak.WeakLink*` in `Weak.m_impl`.
- Placeholder `void*?` stub signatures given real types, keeping namemap names.
- Intrusive containers take a `member: usize` offset.
- Time's `to_string`, Random's uuid and Stream's formatted writes call r02/r03 stubs (trap until merged).

## Deviations
- Inline capacity of Vector etc., node caches and fast-last-access dropped.
- Ref counts kept faithfully, but nothing is ever freed.
- Variant's parameter-pack functions trap.
- Per-architecture branches: AArch64/libm branch taken, using luce-std math and `rint`.
- Donor bug kept: `Bitmap::find_best_fit` never terminates on a bitmap shorter than 64 bits; test avoids it.

## Compiler issues (workarounds marked `# workaround:`)
generic_function_value, float_bits_through_pointer, generic_sizeof_branch (all in luce-js/docs/COMPILER-REQUESTS.md); `check -W` exits 0 on warnings.
