# Core-first rule

**Before writing any function, class, container, string helper, locking
primitive, or platform call: check `Source/Core/Public/` first.** If Core
already provides it, use it — do not rewrite it, do not wrap a std type to
avoid Core.

That's your index reference for what Core offers and when to use each part:
`modules/Core.md`. Naming, typedefs and `noexcept` conventions come from
`rules/coding-style.md`; worked call-site recipes from
`.agents/skills/core-abstractions.md`.

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
Core types are still aliases over `std` (`TVector`, `TArray`, `TMap`, `TPair`,
`TQueue`, `TVariant`, `TFunction` in `Templates/`; `TSpan` in
`Memory/Span.hpp`; `TAtomic`/`FAtomicFlag` in `Core/CoreTypes.hpp`); that does
not change the rule — use the Lumen name. Hand-written replacements live in
the same headers (`TOptional`, `TExpected`, ownership, threads, stop tokens,
condition variables, locks, the HAL and the algorithms).

| Need | std | Lumen Core |
| --- | --- | --- |
| Boolean | `bool` | `Bool` (`Core/CoreTypes.hpp`) |
| Character | `char` / `wchar_t` / `unsigned char` | `AnsiChar` / `WideChar` / `Byte` |
| Fixed-width integer | `std::int8_t`…`std::uint64_t`, `short` / `int` / `long long` | `Int8`…`UInt64` |
| Floating point | `float` / `double` | `Float32` / `Float64` |
| Size / index | `size_t` / `ptrdiff_t` | `USize` / `ISize` |
| Pointer-sized integer | `uintptr_t` / `intptr_t` | `UPtr` / `IPtr` |
| Null pointer type | `std::nullptr_t` | `NullPtrType` |
| Dependent false / void helper | `std::false_type` / `std::void_t` | `AlwaysFalseV` / `TVoid` / `TVoidT` (`Core/CoreTypes.hpp`) |
| Dynamic array | `std::vector` | `TVector<T>` (`Templates/Vector.hpp`) |
| Fixed array | `std::array` | `TArray<T, N>` (`Templates/Array.hpp`) |
| Hash map | `std::unordered_map` | `TMap<K, V>` (`Templates/Map.hpp`) |
| Pair | `std::pair` | `TPair<A, B>` (`Templates/Pair.hpp`) |
| FIFO | `std::queue` | `TQueue<T>` (`Templates/Queue.hpp`) |
| Variant | `std::variant` | `TVariant<…>` (`Templates/Variant.hpp`) |
| Variant visitation | hand-written overload set for `std::visit` | `Meta::Overloaded` (`Meta/Overloaded.hpp`) |
| Maybe-value | `std::optional` | `TOptional<T>` (`Templates/Optional.hpp`) |
| Error value | `std::expected` | `TExpected<T, E>` + `TUnexpected<E>`, `Unexpect`, `MakeUnexpected` (`Templates/Expected.hpp`, `Templates/Unexpected.hpp`) |
| Monadic pipeline | ad-hoc `if` chains | `FMonadic`: `Transform` / `AndThen` / `OrElse` / `Inspect` / `ValueOr` / `ValueOrElse` (`Templates/Monadic.hpp`; inherited by `TOptional`, `TExpected` and non-array `TUniquePtr`) |
| Callable | `std::function` | `TFunction<Signature>` (`Templates/Function.hpp`) |
| String / view | `std::string` / `std::string_view` | `FString` / `FStringView` |
| Span | `std::span` | `TSpan<T>` (`Memory/Span.hpp`) |
| Unique ownership | `std::unique_ptr` / `std::make_unique` | `TUniquePtr<T>` / `MakeUnique<T>` |
| Shared ownership | `std::shared_ptr` / `std::make_shared` | `TSharedPtr<T>` / `TSharedRef<T>`; `MakeShared<T>()` returns a `TSharedRef<T>`, `MakeShareable(ptr)` adopts a raw pointer, `MakeSharedRef` builds from an existing controller, `StaticCastSharedPtr`/`StaticCastSharedRef` for casts (`Memory/SharedPtr.hpp`) |
| Bitmask enum operators | hand-rolled bitwise overloads | `LUMEN_ENUM_FLAGS` (`Core/EnumFlags.hpp`) |
| Reuse control base | `= delete` on every class | `FNonCopyable` / `FNonMovable` (`Core/NonCopyable.hpp`, `Core/NonMovable.hpp`) |
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
| Address-based wait/wake | `std::atomic::wait` / raw futex syscalls | `HAL::FPlatformFutex` (`HAL/PlatformFutex.hpp`) |
| Native thread backend | `pthread_*` / Win32 threads | `HAL::FPlatformThread` (`HAL/PlatformThread.hpp`) — prefer `FThread` |
| Timing / sleeping | `std::chrono`, `std::this_thread::sleep_for` | `HAL::FPlatformTime` (`HAL/PlatformTime.hpp`) |
| OS signal handling | `std::signal` | `HAL::FPlatformSignal` / `HAL::FSignal` (`HAL/PlatformSignal.hpp`) |
| Lock-free bounded SPMC queue | hand-rolled ring buffer | `Thread::TSPMCQueue<T>` (`Thread/SPMCQueue.hpp`) |
| Lock-free latest snapshot | mutex + copy | `Thread::TTripleBuffer<T>` (`Thread/TripleBuffer.hpp`) |
| Index / descriptor allocation | hand-rolled free list or bitmap | `FIndiceAllocator` / `FIndiceHeapAllocator` (`Memory/IndiceAllocator.hpp`) |
| Path / file / directory | `std::filesystem` | `Filesystem::FPath`, `FFile`, `FDirectory`, `IFileSystem`, `IFileHandle` (`Filesystem/*.hpp`) |
| Logging | `printf` / `std::cout` | `FLogger`, `FLogCategory`, `LUMEN_LOG*` (`Logging/*.hpp`) |
| Enum → name / reflection / JSON | ad-hoc `switch` / macros | `CReflected`, `Meta::EnumName`, `Meta::EnumQualifiedName` (`Meta/Reflection.hpp`), `Meta::SerializeToJSON` (`Meta/SerializeToJSON.hpp`) |
| Invoke | `std::invoke` | `Invoke`, `TInvokeResult` / `TInvokeResultT`, `TIsInvocable` / `TIsInvocableV` (`Templates/Invoke.hpp`) |
| Sort | `std::sort` / `std::stable_sort` | `Algo::Sort` / `Algo::StableSort` (`Algo/Sort.hpp`; introsort variants in `Algo/{IntroSort,QuickSort,HeapSort,InsertionSort}.hpp`) |
| Range access / traits | `std::ranges` | `Algo::Begin` / `End` / `IterMove`, `Concepts::CRange` / `CInputRange` / `CBorrowedRange` (`Algo/Range.hpp`) |
| Find | `std::find*` | `Algo::Find*` + `…OrNone` (`Algo/Find.hpp`) |
| For-each | `std::for_each` | `Algo::ForEach` / `Algo::ForEachUntil` (`Algo/ForEach.hpp`) |
| Any match | `std::any_of` | `Algo::AnyOf` (`Algo/Any.hpp`) |
| Range equality | `std::equal` | `Algo::Equal` (`Algo/Equal.hpp`) |
| Type hash | `std::hash` (where applicable) | `FHash::TypeHash<T>()` (`Templates/Hash.hpp`) |

Platform, compiler and build-level definitions come from
`Core/CoreDefinitions.hpp`: gate platform code with `LUMEN_PLATFORM_*`,
`LUMEN_ARCH_*` and `LUMEN_COMPILER_*` instead of raw `_WIN32`, `__linux__`,
`__clang__`, etc.; use `LUMEN_FORCE_INLINE`/`LUMEN_NO_INLINE`,
`LUMEN_UNUSED` and `LUMEN_EXIT_SUCCESS`/`LUMEN_EXIT_FAILURE` instead of
compiler-specific attributes or literal exit codes. Use `LUMEN_ASSERT` for
runtime preconditions: it traps on an `[[unlikely]]` branch in Debug and is
removed under `NDEBUG`; use `static_assert` for compile-time invariants.

Facilities with no Core equivalent (`std::tuple`, `std::index_sequence`,
`std::bit_cast`, non-atomic `std::bitset`) remain allowed until Core grows one.
Range access and range concepts use `Algo/Range.hpp`, not `std::ranges`.

## Concepts: use Core's, not std's

`Concepts/*.hpp` defines the project concepts. Use them in template
constraints instead of spelling out `std::constructible_from`,
`std::convertible_to`, `std::same_as`, `std::invocable`, etc.

| Header | Concept |
| --- | --- |
| `Concepts/ConstructibleFrom.hpp` | `CConstructibleFrom` |
| `Concepts/ConvertibleTo.hpp` | `CConvertibleTo` |
| `Concepts/IsBool.hpp` | `CIsBool` |
| `Concepts/IsIntegral.hpp` | `CIsIntegral` |
| `Concepts/IsInvocable.hpp` | `CIsInvocable` (in `Lumen::Concepts`) |
| `Concepts/IsLockable.hpp` | `CIsLockable` |
| `Concepts/IsStringLike.hpp` | `CIsStringLike` |
| `Concepts/IsVoid.hpp` | `CIsVoid` |
| `Concepts/SameAs.hpp` | `CSameAs`, `CIsSame` |
| `Algo/Range.hpp` | `CRange`, `CInputRange`, `CRandomAccessRange`, `CCommonRange`, `CBorrowedRange` |
| `Concepts/Sortable.hpp` | `CSortableRange` |
| `Concepts/TriviallyCopyable.hpp` | `CTriviallyCopyable` |
| `Templates/Invoke.hpp` | `CIsInvocable` (in `Lumen`, companion to `Invoke`) |
| `Templates/Monadic.hpp` | `CMonadic`, `CTransformable`, `CBindable`, `CRecoverable` |
| `Meta/Reflection.hpp` | `CReflected` |
| `Templates/MonadicDetails.hpp` | `CHasValueMember`, `CBoolTestable`, `CHasError`, `CHasFailHook`, `CHasEmplaceHook` — hooks for custom monadic containers, not call sites |
| `Tests/UnitTestsAssert.hpp` | `CFormattableOrEnum` — test framework only |

Concept names follow `rules/coding-style.md`: PascalCase with a `C` prefix.

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
  changing the container), `ValueOr` (unwrap with a fallback) and
  `ValueOrElse` (lazy fallback). Do not
  unwrap with ad-hoc `if (opt) {...}` chains across a public API; express the
  whole flow monadically.

For signatures, the propagation macros, worked recipes and pitfalls, follow
`.agents/skills/core-abstractions.md`.

Why this is enforced:

- **Paradigm** — error handling and absence are explicit in signatures;
  `Transform`/`AndThen`/`OrElse`/`Inspect`/`ValueOr`/`ValueOrElse` compose
  failure handling once, instead of repeating checks at every call site.
- **Safety** — nothing can escape a `noexcept` frame or an actor boundary;
  the compiler forces callers to consume the `TOptional`/expected value, so
  ignored error states become review findings rather than runtime surprises.
- **Performance** — no exception machinery, no unwinding paths; monadic
  chains keep hot paths allocation-free, inline-friendly, and
  branch-predictable.

Banned in engine code (non-test): `throw`, `std::expected`, `std::optional`
(use Core's), and std APIs defined to throw (`std::stoi`, growth-`throwing`
std containers in public paths, …). The single allowed throw site is the test
framework (`LUMEN_CHECK*` throws `FUnitTestAssertionFailure`, caught and
reported by the test runner — see `rules/testing.md`). `try`/`catch` are
allowed only at boundaries that translate a throwing std call (e.g.
`std::visit`) into a structured error (RHI binding and transition validation)
or report console output failure (`FLogger`); they are never control flow.

## Non-negotiables behind the rule

- Public APIs accept/return Lumen types (`Int32`, `USize`, `FStringView`,
  `TVector<T>`), not raw `int`/`size_t`/C++ std types.
- No per-message/object hidden allocation in actor/concurrency code — Core
  provides `Thread::TSPMCQueue<T>` (`Thread/SPMCQueue.hpp`) and
  `Thread::TTripleBuffer<T>` (`Thread/TripleBuffer.hpp`) for that (see
  `modules/Core.md`).
- Do not copy-paste a utility into your module "temporarily"; make it either
  a Core addition (with tests) or a properly-namespaced private helper.
