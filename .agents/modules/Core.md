# Module: Core

- **Type:** Shared library (`Source/Core/meson.build`), no dependencies.
- **Status:** built; everything else depends on it (auto-linked into every
  benchmark target; test targets list `Core` in `Depends` explicitly).
- **Role:** engine foundation — containers, strings, memory, algorithms,
  threading, synchronization, filesystem, HAL, reflection, logging, the
  in-house test and benchmark frameworks.

## Public headers map (use these before writing anything new)

Some headers are currently thin aliases over `std` (`TVector`, `TArray`,
`TMap`, `TPair`, `TQueue`, `TVariant`, `TFunction`) or over `std::atomic`
(`TAtomic`, `FAtomicFlag`); `TSpan` is also an alias. Always use the Lumen
name anyway: aliases may be replaced by real implementations later, and the
name keeps call sites consistent. Hand-written replacements exist for memory
ownership (unique/shared), optional/expected, threads, stop tokens, condition
variables, atomic bitsets, locks, the HAL and the algorithms.

### Core basics, containers, strings, ownership

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Templates/Vector.hpp` | `TVector<T>` | resizable array (`std::vector` equivalent) |
| `Templates/Array.hpp` | `TArray<T, Size>` | fixed-size array (`std::array` equivalent) |
| `Templates/Map.hpp` | `TMap<K, V>` | hash-map lookup (`std::unordered_map` equivalent) |
| `Templates/Pair.hpp` | `TPair<A, B>` | two-tuple key/value |
| `Templates/Queue.hpp` | `TQueue<T>` | FIFO (`std::queue` equivalent) |
| `Templates/Variant.hpp` | `TVariant<…>` | type-safe union |
| `Templates/Optional.hpp` | `TOptional<T>` | maybe-value (hand-written) |
| `Templates/Expected.hpp` (+ `Unexpected.hpp`, `Monadic.hpp`, `MonadicDetails.hpp`) | `TExpected<T, E>`, `TUnexpected<E>`, `MakeUnexpected`, `Unexpect`, `FMonadic`, `Transform`/`AndThen`/`OrElse`/`Inspect`/`ValueOr`/`ValueOrElse`; concepts `CMonadic`, `CTransformable`, `CBindable`, `CRecoverable` (`TOptional` and non-array `TUniquePtr` also inherit `FMonadic`) | error propagation without exceptions |
| `Templates/Function.hpp` | `TFunction<Signature>` | type-erased callable (`std::function` equivalent) |
| `String/String.hpp`, `String/StringView.hpp`, `String/StringUtils.hpp` | `FString`, `FStringView`; `IsSpace`, `ToLower`/`ToLowerInline`, `EqualsIgnoreCase`, `StripParam`/`StripDashes`/`AsOption` | all text handling (currently aliases; TODO real types) |
| `Memory/UniquePtr.hpp`, `Memory/SharedPtr.hpp`, `Memory/Span.hpp` | `TUniquePtr`/`MakeUnique`, `TSharedPtr`/`TSharedRef`, `MakeShared` (returns `TSharedRef`)/`MakeSharedRef`/`MakeShareable`, `StaticCastSharedPtr`/`StaticCastSharedRef`, `TSpan` | memory ownership (no raw `new`) |
| `Memory/IndiceAllocator.hpp` | `FIndiceAllocator` (free-list, first-fit), `FIndiceHeapAllocator` (bitmap), `FIndiceAllocation`, `FIndiceAllocatorRange` | thread-safe index/descriptor allocation, variable-size or fixed-size |
| `Core/CoreTypes.hpp` | `Int8`…`UInt64`, `Float32/64`, `AnsiChar`, `WideChar`, `Byte`, `Bool`, `USize`, `ISize`, `UPtr/IPtr`, `NullPtrType`, `AlwaysFalseV`, `TVoid`/`TVoidT`, `TAtomic<T>`, `FAtomicFlag` | Lumen typedefs, compile-time helpers and atomics |
| `Core/CoreDefinitions.hpp` | `LUMEN_PLATFORM_{UNIX,WINDOWS,LINUX,MACOS}`, `LUMEN_ARCH_*`, `LUMEN_COMPILER_*`, `LUMEN_PATH_SEPARATOR`, `LUMEN_NEWLINE`, `LUMEN_SHARED_LIB_EXTENSION`, `LUMEN_EXIT_SUCCESS`/`LUMEN_EXIT_FAILURE`, `LUMEN_FORCE_INLINE`/`LUMEN_NO_INLINE`, `LUMEN_ASSERT`, `LUMEN_UNUSED`, `LUMEN_DISABLE_UBSAN` | platform/compiler gating, path/exit constants and inline attributes |

### Algorithms and utilities

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Algo/Sort.hpp` (+ `IntroSort.hpp`, `MergeSort.hpp`, `QuickSort.hpp`, `HeapSort.hpp`, `InsertionSort.hpp`) | `Algo::Sort`, `Algo::StableSort` (declared in `Sort.hpp`, merge-sort implementation in `MergeSort.hpp`), `Algo::IntroSort`, `Algo::QuickSort`, `Algo::HeapSort`, `Algo::InsertionSort` | sorting without `std::sort`; prefer `Sort` (introsort, in-place, O(n log n) worst case), `StableSort` only when stability is required |
| `Algo/Find.hpp` | `Algo::Find`, `FindIf`, `FindIfNot`, `FindIfEqual` (+ `…OrNone` returning `TOptional`) | searching ranges without `std::find` |
| `Algo/ForEach.hpp` | `Algo::ForEach`, predicate-filtered `ForEach`, `Algo::ForEachUntil` (early-exit variants) | iterating ranges |
| `Algo/Any.hpp` | `Algo::AnyOf` | any element matches a predicate, without `std::any_of` |
| `Algo/Equal.hpp` | `Algo::Equal` (with optional binary predicate) | comparing two ranges, without `std::equal` |
| `Algo/Range.hpp` (`RangeTraits.hpp` compatibility header) | `Algo::Begin`/`End`/`IterMove`; `Concepts::CRange`, `CInputRange`, `CRandomAccessRange`, `CCommonRange`, `CBorrowedRange`; range type traits (`Algo::Internal`) | range access and introspection without `std::ranges`; `EnableBorrowedRangeV` marks non-owning views |
| `Templates/Hash.hpp` | `FHash::TypeHash<T>()` | compile-time stable type hashing |
| `Templates/Invoke.hpp` | `Invoke`, `TInvokeResult`/`TInvokeResultT`, `TIsInvocable`/`TIsInvocableV`, `CIsInvocable` (in `Lumen`) | invoking callables/member pointers without `std::invoke` |
| `Templates/AtomicBitset.hpp` | `FAtomicBitset` (`Resize`/`ClearAll`/`GetNumBits`, `Set`/`Clear`/`Test`/`ForEachSetBitAndClear`) | concurrent dirty tracking / flag sets |
| `Core/EnumFlags.hpp`, `Core/NonCopyable.hpp`, `Core/NonMovable.hpp` | `LUMEN_ENUM_FLAGS`, `FNonCopyable`, `FNonMovable` | bitmask enums, reuse control |
| `Concepts/*.hpp` | `CConstructibleFrom`, `CConvertibleTo`, `CIsBool`, `CIsIntegral`, `CIsInvocable`, `CIsLockable`, `CIsStringLike`, `CIsVoid`, `CSameAs`/`CIsSame`, `CRange`/`CInputRange`/`CRandomAccessRange`/`CCommonRange`/`CBorrowedRange`, `CSortableRange`, `CTriviallyCopyable` | constraining templates; use these instead of `std::` concepts (full list in `rules/core-first.md`) |

### Threading and synchronization

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Thread/Thread.hpp` | `FThread` — move-only thread with cooperative stop + automatic join; accepts a callable taking `FStopToken` as first argument; `GetStopToken`/`RequestStop`, `Join`/`Detach`, `GetCurrentId`, `HardwareConcurrency`, `Yield` | any thread creation (never `std::thread`/`std::jthread`) |
| `Templates/StopToken.hpp` | `FStopToken`, `FStopSource`, `FStopCallback` | cooperative cancellation; register cleanup callbacks |
| `Templates/ConditionVariable.hpp` | `FConditionVariable::Wait`/`WaitFor`/`WaitUntil` (with optional predicate), `NotifyOne`/`NotifyAll`; futex-backed, works with any `CIsLockable` | waiting/signalling on shared state |
| `HAL/PlatformMutex.hpp` (+ `Unix/`, `Windows/` implementations) | `HAL::FPlatformMutex`, `FPlatformSharedMutex`, `FPlatformRecursiveSharedMutex`, `FPlatformSpinLockMutex` | mutex/rwlock/spinlock (never `std::mutex`, `std::shared_mutex`, `std::recursive_mutex`) |
| `HAL/LockGuard.hpp` | `HAL::TLockGuard`, `TUniqueLock`, `TSharedLock`, `TSharedUniqueLock`, `FDeferLockT`/`FTryToLockT`/`FAdoptLockT` | RAII locking (never `std::lock_guard`/`std::unique_lock`) |
| `HAL/PlatformAtomic.hpp` | `HAL::CpuRelax()` | spin-wait relax hint |
| `HAL/PlatformFutex.hpp` (+ `Linux/`, `MacOS/`, `Windows/`) | `HAL::FPlatformFutex::{Wait, WaitFor, WakeOne, WakeAll}` | address-based wait/wake under condition variables |
| `HAL/PlatformThread.hpp` (+ `Unix/`, `Windows/`) | `HAL::FPlatformThread` with `FId`, `FNativeHandle` | low-level native thread backend (prefer `FThread`) |
| `HAL/PlatformTime.hpp` | `HAL::FPlatformTime::{Seconds, NowNanoseconds, Sleep}` | timing and sleeping |
| `Thread/SPMCQueue.hpp` | `Lumen::Thread::TSPMCQueue<T>` | lock-free bounded single-producer/multi-consumer queue (e.g. free lists) |
| `Thread/TripleBuffer.hpp` | `Lumen::Thread::TTripleBuffer<T>` | lock-free latest-snapshot publish/swap of state |

### HAL, filesystem, reflection, logging, diagnostics

| Area / header | Provides | Use when |
| --- | --- | --- |
| `HAL/PlatformSignal.hpp` | `HAL::FPlatformSignal` (alias `HAL::FSignal`), `ESystemSignal`, `Bind`/`Raise`/`Dispatch`/`Unbind`/`Reset` | OS signal trapping (never `std::signal` handling directly) |
| `Filesystem/Path.hpp`, `File.hpp`, `Directory.hpp`, `IFileSystem.hpp`, `IFileHandle.hpp` | `FPath`, file/dir abstractions behind interfaces | any file/dir work |
| `Logging/Logger.hpp`, `LoggingCategory.hpp`, `LoggingVerbosity.hpp` | `FLogger`, `FLogCategory`, verbosity levels | all diagnostics (tests init/shutdown it) |
| `Meta/Reflection.hpp`, `Meta/Overloaded.hpp`, `Meta/SerializeToJSON.hpp` | C++26 reflection-based introspection (`CReflected` concept), `Meta::Overloaded` visitor helper, `Meta::EnumName` (unqualified) and `Meta::EnumQualifiedName` (qualified) enum-name helpers, `Meta::SerializeToJSON` with `FJSONOptions` | field iteration, serialization, enum → name conversion |
| `Tests/UnitTest.hpp` (framework) | `LUMEN_TEST`, `LUMEN_INTEGRATION_TEST`, `LUMEN_CHECK*` | unit tests (see `rules/testing.md`) |
| `Benchmarks/Benchmark.hpp` (framework) | `LUMEN_BENCHMARK*` | benchmarks (see `rules/testing.md`) |

Source lives in `Source/Core/Private/**`, mirroring the `Public/` topics
(`Core/`, `Templates/`, `Thread/`, `Logging/`, `Filesystem/`, `HAL/`,
`Memory/`, `Tests/`, `Benchmarks/`); `Algo/` is header-only. Tests live in
`Source/Core/Tests/{Functional,Integration}`.

## Notes for agents

- Core is the reference implementation of the coding standard — when unsure
  how to write Lumen code, mimic Core's headers.
- For how and when to use `Algo` plus `TOptional`/`TExpected`/`FMonadic` at
  call sites, see `.agents/skills/core-abstractions.md`; the full std → Core
  map and concept list are in `rules/core-first.md`, naming and `noexcept`
  conventions in `rules/coding-style.md`.
- The HAL layer is `Lumen::HAL`: mutex/thread/futex/signal/time types are
  selected by `HAL/Platform*.hpp`; include the `Platform*` header, not a
  platform-specific one.
- Never add module-specific or RHI-flavoured API to Core; keep it generic.
- New Core utilities must come with tests in `Source/Core/Tests/`.
