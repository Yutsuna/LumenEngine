# Module: Core

- **Type:** Shared library (`Source/Core/meson.build`), no dependencies.
- **Status:** built; everything else depends on it (auto-linked into every
  test/benchmark target even when not in `Depends`).
- **Role:** engine foundation — containers, strings, memory, algorithms,
  threading, synchronization, filesystem, HAL, reflection, logging, the
  in-house test and benchmark frameworks.

## Public headers map (use these before writing anything new)

Some `Templates/` headers are currently thin aliases over `std` (`TVector`,
`TArray`, `TMap`, `TPair`, `TQueue`, `TVariant`, `TFunction`, `TSpan`) or over
`std::atomic` (`TAtomic`, `FAtomicFlag`). Always use the Lumen name anyway:
aliases may be replaced by real implementations later, and the name keeps call
sites consistent. Hand-written replacements exist for optional/expected,
threads, stop tokens, condition variables, atomics bitset, locks, the HAL and
the algorithms.

### Containers, strings, ownership

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Templates/Vector.hpp` | `TVector<T>` | resizable array (`std::vector` equivalent) |
| `Templates/Array.hpp` | `TArray<T, Size>` | fixed-size array (`std::array` equivalent) |
| `Templates/Map.hpp` | `TMap<K, V>` | hash-map lookup (`std::unordered_map` equivalent) |
| `Templates/Pair.hpp` | `TPair<A, B>` | two-tuple key/value |
| `Templates/Queue.hpp` | `TQueue<T>` | FIFO (`std::queue` equivalent) |
| `Templates/Variant.hpp` | `TVariant<…>` | type-safe union |
| `Templates/Optional.hpp` | `TOptional<T>` | maybe-value (hand-written) |
| `Templates/Expected.hpp` (+ `Unexpected.hpp`, `Monadic.hpp`, `MonadicDetails.hpp`) | `TExpected<T, E>`, `TUnexpected<E>`, `FMonadic`, `Transform`/`AndThen`/`OrElse` | error propagation without exceptions |
| `Templates/Function.hpp` | `TFunction<Signature>` | type-erased callable (`std::function` equivalent) |
| `String/String.hpp`, `StringView.hpp`, `StringUtils.hpp` | `FString`, `FStringView` | all text handling (currently aliases; TODO real types) |
| `Memory/UniquePtr.hpp`, `SharedPtr.hpp`, `Span.hpp` | `TUniquePtr`/`MakeUnique`, `TSharedPtr`/`TSharedRef`/`MakeShared`/`MakeShareable`, `TSpan` | memory ownership (no raw `new`) |
| `Core/CoreTypes.hpp` | `Int8`…`UInt64`, `Float32/64`, `Bool`, `USize`, `ISize`, `UPtr/IPtr`, `NullPtrType`, `TAtomic<T>`, `FAtomicFlag` | Lumen typedefs and atomics |

### Algorithms and utilities

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Algo/Sort.hpp` (+ `IntroSort.hpp`, `StableSort` via `MergeSort.hpp`, `QuickSort.hpp`, `HeapSort.hpp`, `InsertionSort.hpp`) | `Algo::Sort`, `Algo::StableSort`, `Algo::IntroSort`, `Algo::QuickSort`, `Algo::HeapSort`, `Algo::InsertionSort` | sorting without `std::sort`; prefer `Sort` (introsort, in-place, O(n log n) worst case), `StableSort` only when stability is required |
| `Algo/Find.hpp` | `Algo::Find`, `FindIf`, `FindIfNot`, `FindIfEqual` (+ `…OrNone` returning `TOptional`) | searching ranges without `std::find` |
| `Algo/ForEach.hpp` | `Algo::ForEach`, predicate-filtered `ForEach` | iterating ranges |
| `Templates/Hash.hpp` | `FHash::TypeHash<T>()` | compile-time stable type hashing |
| `Templates/Invoke.hpp` | `Invoke`, `TInvokeResult`/`TInvokeResultT`, `TIsInvocable`/`TIsInvocableV` | invoking callables/member pointers without `std::invoke` |
| `Templates/AtomicBitset.hpp` | `FAtomicBitset` (`Set`/`Clear`/`Test`/`ForEachSetBitAndClear`) | concurrent dirty tracking / flag sets |
| `Core/EnumFlags.hpp`, `NonCopyable.hpp`, `NonMovable.hpp` | `LUMEN_ENUM_FLAGS`, `FNonCopyable`, `FNonMovable` | bitmask enums, reuse control |
| `Concepts/*.hpp` | `CConstructibleFrom`, `CConvertibleTo`, `CIsSame`/`CSameAs`, `CIsStringLike`, `CTriviallyCopyable`, `CIsInvocable`, `CIsLockable`, `CSortableRange` | constraining templates |

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
| `Meta/Reflection.hpp`, `Overloaded.hpp`, `SerializeToJSON.hpp` | C++26 reflection-based introspection (`CReflected` concept), visitors | field iteration, serialization |
| `Tests/UnitTest.hpp` (framework) | `LUMEN_TEST`, `LUMEN_INTEGRATION_TEST`, `LUMEN_CHECK*` | unit tests (see `rules/testing.md`) |
| `Benchmarks/Benchmark.hpp` (framework) | `LUMEN_BENCHMARK*` | benchmarks (see `rules/testing.md`) |

Source lives in `Source/Core/Private/**`, mirroring the `Public/` topics
(`Templates/`, `Thread/`, `Logging/`, `Filesystem/`, `HAL/`, `Memory/`,
`Tests/`, `Benchmarks/`). Tests live in
`Source/Core/Tests/{Functional,Integration}`.

## Notes for agents

- Core is the reference implementation of the coding standard — when unsure
  how to write Lumen code, mimic Core's headers.
- The HAL layer is `Lumen::HAL`: mutex/thread/futex/signal/time types are
  selected by `HAL/Platform*.hpp`; include the `Platform*` header, not a
  platform-specific one.
- Never add module-specific or RHI-flavoured API to Core; keep it generic.
- New Core utilities must come with tests in `Source/Core/Tests/`.
