# Module: Core

- **Type:** Shared library (`Source/Core/meson.build`), no dependencies.
- **Status:** built; everything else depends on it (auto-linked into every
  test/benchmark target even when not in `Depends`).
- **Role:** engine foundation — containers, strings, memory, threading,
  synchronization, filesystem, HAL, reflection, logging, the in-house test
  and benchmark frameworks.

## Public headers map (use these before writing anything new)

| Area / header | Provides | Use when |
| --- | --- | --- |
| `Templates/Vector.hpp` | `TVector<T>` | resizable array (std::vector equivalent) |
| `Templates/Array.hpp` | `TArray<T>` | fixed-capacity array |
| `Templates/Map.hpp` | `FMap` | associative lookup |
| `Templates/Pair.hpp` | `TPair<A,B>` | two-tuple key/value |
| `Templates/Queue.hpp` | `TQueue` | FIFO |
| `Templates/Variant.hpp` | `TVariant<…>` | type-safe union |
| `Templates/Optional.hpp` | `TOptional<T>` | maybe-value (std::optional equivalent) |
| `Templates/Expected.hpp` (+ `Unexpected`, `Monadic.hpp`) | expected-style error handling + monadic ops | error propagation without exceptions |
| `Templates/Function.hpp` | `TFunction`/`TFunc` type-erased callable | callbacks, delegates |
| `Templates/Hash.hpp` | `FHash` | hashing for containers/ids |
| `Templates/AtomicBitset.hpp` | atomic bitset | concurrent flag sets |
| `String/String.hpp`, `StringView.hpp`, `StringUtils.hpp` | `FString`, `FStringView` | all text handling |
| `Memory/UniquePtr.hpp`, `SharedPtr.hpp`, `Span.hpp` | ownership smart pointers, `TSpan` | memory ownership (no raw `new`) |
| `Thread/SPMCQueue.hpp` | wait-free MPSC queue | actor mailboxes, message passing |
| `Thread/TripleBuffer.hpp` | lock-free triple buffer (`TTripleBuffer<T>`) | lock-free publish/swap of state |
| `HAL/Mutex.hpp`, `SpinLock.hpp`, `SharedMutex.hpp` | `FMutex`, `FSpinLock`, `FSharedMutex` | synchronization (not std::mutex) |
| `HAL/PlatformAtomic.hpp`, `PlatformTime.hpp`, `Signal.hpp` | atomics, high-res time, signals | platform access via HAL, not `<atomic>` directly |
| `Filesystem/Path.hpp`, `File.hpp`, `Directory.hpp`, `IFileSystem.hpp`, `IFileHandle.hpp` | `FPath`, file/dir abstractions behind interfaces | any file/dir work |
| `Logging/Logger.hpp`, `LoggingCategory.hpp`, `LoggingVerbosity.hpp` | `FLogger`, `FLogCategory`, verbosity levels | all diagnostics (tests init/shutdown it) |
| `Meta/Reflection.hpp`, `Overloaded.hpp`, `SerializeToJSON.hpp` | C++26 reflection-based introspection (`CReflected` concept), visitors | field iteration, serialization |
| `Concepts/*.hpp` | `CConstructibleFrom`, `CConvertibleTo`, `CSameAs`, `CIsStringLike`, `CTriviallyCopyable` | constraining templates |
| `Core/NonCopyable.hpp`, `NonMovable.hpp` (`FNonCopyable`, `FNonMovable`) | base classes | reuse-control of public classes |
| `Tests/UnitTest.hpp` (framework) | `LUMEN_TEST`, `LUMEN_INTEGRATION_TEST`, `LUMEN_CHECK*` | unit tests (see `rules/testing.md`) |
| `Benchmarks/Benchmark.hpp` (framework) | `LUMEN_BENCHMARK*` | benchmarks (see `rules/testing.md`) |

Source lives in `Source/Core/Private/**` (e.g. `Templates/`, `Thread/`,
`Logging/`, `Filesystem/`, `HAL/`, `Memory/`), tests in
`Source/Core/Tests/{Functional,Integration}`.

## Notes for agents

- Core is the reference implementation of the coding standard — when unsure
  how to write Lumen code, mimic Core's headers.
- Never add module-specific or RHI-flavoured API to Core; keep it generic.
- New Core utilities must come with tests in `Source/Core/Tests/`.
