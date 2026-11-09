---
name: Write Benchmarks
description: Author and run Lumen benchmarks with LUMEN_BENCHMARK macros correctly (optimized build required)
---

## Where and how to write

1. Location: `Source/<Module>/Benchmarks/<Topic>/**.cpp` — compiled into the
   single `<Module>Benchmarks` binary (like tests, one `main` added from
   `Meson/BenchRunner.cpp`; only add `Main.cpp` if you truly need a custom one).
2. Register via the in-house macros (`Source/Core/Public/Benchmarks/Benchmark.hpp`):
   - `LUMEN_BENCHMARK( Suite, Name )`,
   - `LUMEN_BENCHMARK_ARGS( Suite, Name, Args... )`,
   - `LUMEN_BENCHMARK_RANGE( Suite, Name, Min, Max )`,
   - `LUMEN_MACRO_BENCHMARK( Suite, Name )` — macro kind: heavy whole-stage
     code (asset load, level build, a full frame…), one iteration per sample.
3. Style: follow `rules/coding-style.md` (PascalCase, no single-letter
   variable names, `namespace Lumen`, note the benchmark loop "do not
   optimize away" pitfalls).
4. Do-case: benchmark containers on hot paths must NOT allocate per
   iteration — prefer Core primitives (pre-sized `TVector`, `Core/Thread`
   structures) and benchmark both the contenders from Core you intend to
   replace (Core-first rule).

## How to run

```sh
meson setup Build-Release --buildtype=release   # or the build-release command
ninja -C Build-Release
meson benchmark -C Build-Release
Build/Source/Meson/Benchmarks/<Module>Benchmarks [Filter [report.json]]
```

## Checks

- `LUMEN_BENCHMARK_OPTIMIZED_BUILD` (from `BenchmarkTypes.hpp`) flips on
  `__OPTIMIZE__` — a debug build reports that fact: numbers from debug or
  sanitized builds are throwaway, never comparison material.
- JSON reporter (second argv) writes a comparable artifact — keep it for
  before/after runs.
- Suites mirror the module name (`--suite`), filters are argv[1].
