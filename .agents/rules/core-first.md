# Core-first rule

**Before writing any function, class, container, string helper, locking
primitive, or platform call: check `Source/Core/Public/` first.** If Core
already provides it, use it — do not rewrite it, do not wrap a std type to
avoid Core.

That's your index reference for what Core offers and when to use each part:
`modules/Core.md`.

## How to apply

1. Classify what you need: container? string? error/optional? sync primitive?
   file access? reflection? logging?
2. Look it up in `modules/Core.md` (or grep `Source/Core/Public/**`).
3. Found → use the Core type (`TVector`, `FStringView`, `FThread`,
   `FConditionVariable`, `HAL::FPlatformMutex`…) instead of `std::vector`,
   `std::string_view`, `std::thread`, `std::mutex`, … in your code.
4. Not found → use `std` **only for facilities Core has not implemented yet**
   (e.g. `std::tuple`, `std::index_sequence`, `std::bit_cast`). Do not
   hand-roll a replacement and do not wrap a std type to avoid Core. Then
   decide where the new utility belongs:
   - Genuinely generic and reusable by all modules → add it to **Core**
     (Public + Private + tests), then depend on it.
   - Only meaningful to one module → keep it in that module, but follow the
     same names/patterns as Core so it can graduate later.
5. If a std construct exists but Core's version adds guarantees (header-only,
   no allocation on hot path, i-cache-friendliness, lock-free semantics),
   Core's version wins: prefer `TVector`/`FStringView`/`FThread` over std in
   all public signatures and hot paths.

## std → Core map

Use the right-hand name whenever the left-hand facility is needed. Several
`Templates/` types are still aliases over `std`; that does not change the
rule — use the Lumen name.

| Need | std | Lumen Core |
| --- | --- | --- |
| Boolean | `bool` | `Bool` (`Core/CoreTypes.hpp`) |
| Character | `char` / `wchar_t` / `unsigned char` | `AnsiChar` / `WideChar` / `Byte` |
| Fixed-width integer | `std::int8_t`…`std::uint64_t`, `short` / `int` / `long long` | `Int8`…`UInt64` |
| Floating point | `float` / `double` | `Float32` / `Float64` |
| Size / index | `size_t` / `ptrdiff_t` | `USize` / `ISize` |
| Pointer-sized integer | `uintptr_t` / `intptr_t` | `UPtr` / `IPtr` |
| Null pointer type | `std::nullptr_t` | `NullPtrType` |
| Dependent false / void helper | `std::false_type` / `std::void_t` | `AlwaysFalseV` / `TVoidT` |
| Dynamic array | `std::vector` | `TVector<T>` (`Templates/Vector.hpp`) |
| Fixed array | `std::array` | `TArray<T, N>` (`Templates/Array.hpp`) |
| Hash map | `std::unordered_map` | `TMap<K, V>` (`Templates/Map.hpp`) |
| Pair | `std::pair` | `TPair<A, B>` (`Templates/Pair.hpp`) |
| FIFO | `std::queue` | `TQueue<T>` (`Templates/Queue.hpp`) |
| Variant | `std::variant` | `TVariant<…>` (`Templates/Variant.hpp`) |
| Maybe-value | `std::optional` | `TOptional<T>` (`Templates/Optional.hpp`) |
| Error value | `std::expected` | `TExpected<T, E>` + `TUnexpected<E>` |
| Callable | `std::function` | `TFunction<Signature>` (`Templates/Function.hpp`) |
| String / view | `std::string` / `std::string_view` | `FString` / `FStringView` |
| Span | `std::span` | `TSpan<T>` (`Memory/Span.hpp`) |
| Unique ownership | `std::unique_ptr` / `std::make_unique` | `TUniquePtr<T>` / `MakeUnique<T>` |
| Shared ownership | `std::shared_ptr` / `std::make_shared` | `TSharedPtr<T>` / `TSharedRef<T>`; `MakeShared<T>()` returns a `TSharedRef<T>`, `MakeShareable(ptr)` adopts a raw pointer, `MakeSharedRef` builds from an existing controller, `StaticCastSharedPtr`/`StaticCastSharedRef` for casts (`Memory/SharedPtr.hpp`) |
| Thread | `std::thread`, `std::jthread` | `FThread` (`Thread/Thread.hpp`) |
| Cooperative stop | `std::stop_token` / `std::stop_source` / `std::stop_callback` | `FStopToken` / `FStopSource` / `FStopCallback` |
| Mutex | `std::mutex` | `HAL::FPlatformMutex` (`HAL/PlatformMutex.hpp`) |
| Shared mutex | `std::shared_mutex` | `HAL::FPlatformSharedMutex` |
| Recursive mutex | `std::recursive_mutex` | `HAL::FPlatformRecursiveSharedMutex` |
| Spin lock | — (hand-rolled) | `HAL::FPlatformSpinLockMutex` |
| Condition variable | `std::condition_variable` | `FConditionVariable` (`Templates/ConditionVariable.hpp`) |
| RAII lock | `std::lock_guard` / `std::unique_lock` / `std::shared_lock` | `HAL::TLockGuard` / `HAL::TUniqueLock` / `HAL::TSharedLock` |
| Atomic | `std::atomic<T>` / `std::atomic_flag` | `TAtomic<T>` / `FAtomicFlag` (`Core/CoreTypes.hpp`) |
| Atomic bitset | `std::bitset` + manual locks | `FAtomicBitset` (`Templates/AtomicBitset.hpp`) |
| Invoke | `std::invoke` | `Invoke` (`Templates/Invoke.hpp`) |
| Sort | `std::sort` / `std::stable_sort` | `Algo::Sort` / `Algo::StableSort` |
| Find | `std::find*` | `Algo::Find*` (`Algo/Find.hpp`) |
| For-each | `std::for_each` | `Algo::ForEach` / `Algo::ForEachUntil` (`Algo/ForEach.hpp`) |
| Type hash | `std::hash` (where applicable) | `FHash::TypeHash<T>()` |

Platform, compiler and build-level definitions come from
`Core/CoreDefinitions.hpp`: gate platform code with `LUMEN_PLATFORM_*`,
`LUMEN_ARCH_*` and `LUMEN_COMPILER_*` instead of raw `_WIN32`, `__linux__`,
`__clang__`, etc.; use `LUMEN_FORCE_INLINE`/`LUMEN_NO_INLINE`,
`LUMEN_UNUSED` and `LUMEN_EXIT_SUCCESS`/`LUMEN_EXIT_FAILURE` instead of
compiler-specific attributes or literal exit codes.

Non-atomic `std::bitset`, `std::tuple`, `std::ranges` plumbing and other
facilities with no Core equivalent remain allowed until Core grows one.

## TOptional / TExpected / Monadic are mandatory

**Lumen is `noexcept` by definition — no exceptions are tolerated anywhere
in engine code.** Absence and failure therefore cannot be signalled by
throwing; they are part of the type system:

- **Absence** → `TOptional<T>` (`Templates/Optional.hpp`).
- **Failure** → `TExpected<T, E>` (`Templates/Expected.hpp`,
  pre-built errors carried by `TUnexpected` from `Templates/Unexpected.hpp`).
- **Pipeline** → combine results with the **Monadic** operations of
  `Templates/Monadic.hpp`: `Transform` (T->U), `AndThen` (bind into another
  valid value), `OrElse` (recover from an error), `Inspect` (peek without
  changing the container) and `ValueOr` (unwrap with a fallback). Do not
  unwrap with ad-hoc `if (opt) {...}` chains across a public API; express the
  whole flow monadically.

Why this is enforced:

- **Paradigm** — error handling and absence are explicit in signatures;
  `Transform`/`AndThen`/`OrElse`/`Inspect`/`ValueOr` compose failure handling
  once, instead of repeating checks at every call site.
- **Safety** — nothing can escape a `noexcept` frame or an actor boundary;
  the compiler forces callers to consume the `TOptional`/expected value, so
  ignored error states become review findings rather than runtime surprises.
- **Performance** — no exception machinery, no unwinding paths; monadic
  chains keep hot paths allocation-free, inline-friendly, and
  branch-predictable.

Banned in engine code (non-test): `throw`, `catch`, `try`, `std::expected`,
`std::optional` (use Core's), and std APIs defined to throw (`std::stoi`,
growth-`throwing` std containers in public paths, …). The single allowed
throw site is the test framework (`LUMEN_CHECK*` throws
`FUnitTestAssertionFailure`, caught and reported by the test runner —
see `rules/testing.md`).

## Non-negotiables behind the rule

- Public APIs accept/return Lumen types (`Int32`, `USize`, `FStringView`,
  `TVector<T>`), not raw `int`/`size_t`/C++ std types.
- No per-message/object hidden allocation in actor/concurrency code — Core
  provides lock-free queues and triple buffers for that (see `modules/Core.md`).
- Do not copy-paste a utility into your module "temporarily"; make it either
  a Core addition (with tests) or a properly-namespaced private helper.
