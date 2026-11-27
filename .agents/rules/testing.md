# Testing & benchmarks rules

The framework is **in-house** (`Source/Core/Public/Tests/`, `.../Benchmarks/`).
No GoogleTest, Catch or external test dependency. Mirror gtest habits and you
will be wrong.

## Unit tests

- Register with macros (self-register at static init via
  `Lumen::Tests::FUnitTestRegistry`):
  ```cpp
  LUMEN_TEST( FMailBox_Push, ReturnsTrueOnSuccess )
  {
      FMailBox Box;
      LUMEN_CHECK_TRUE( Box.Push( FMessage::Make( 1U, 0ULL ) ) );
  }

  LUMEN_INTEGRATION_TEST( Suite, Name ) { … }   // flagged as integration kind
  ```
- Assertions: `LUMEN_CHECK`, `LUMEN_CHECK_MSG`, `LUMEN_CHECK_EQ`,
  `LUMEN_CHECK_NOT_EQ`, `LUMEN_CHECK_TRUE`, `LUMEN_CHECK_FALSE`,
  `LUMEN_CHECK_HAS_VALUE`, `LUMEN_CHECK_HAS_ERROR`.
- Layout: `Source/<Module>/Tests/{Functional,Integration}/<Topic>/…`,
  mirroring `Public/` topics. Test code follows `rules/coding-style.md`
  (open `namespace Lumen { }` in `Tests/` `.cpp` files, RHI tests included —
  never the shortened `namespace Lumen::RHI { }`; qualify RHI symbols
  explicitly with `Lumen::RHI::`, as required by `rules/coding-style.md`),
  `F/E/T/b` names, precise variable names).
- Every `*.cpp` under `Tests/` compiles into the **single** `<Module>Tests`
  binary; a default `main` (`Meson/TestRunner.cpp`) is appended unless
  `Tests/Main.cpp` or `TestMain.cpp` exists. Never put a `main()` in test files.
- A failed run exits **84**, not 1 (Epitech-style). `meson test` still
  reports it as failure.
- Test targets also get the module's `Private/` headers on the include path —
  use that for white-box tests instead of exporting more symbols. For a shared
  module, tests link the same implementation objects statically through
  `GlobalTestModules`; production consumers still use the shared library through
  `GlobalModules`, and no additional symbols are exported.
- Core is auto-linked into every benchmark target even when not in `Depends`;
  test targets list `Core` in their `Depends` explicitly.

## Running

```sh
meson test -C Build                          # all suites
meson test -C Build --suite Scene            # one module
./Build/Source/Meson/Tests/SceneTests Filter # direct; argv[1] filters suites/tests
```

Binaries land under `Build/Source/Meson/Tests/`. New test files need
`meson setup Build --reconfigure` (see `rules/build.md`).

## Benchmarks

- `Source/<Module>/Benchmarks/**`, macro-driven
  (`Source/Core/Public/Benchmarks/Benchmark.hpp`):
  `LUMEN_BENCHMARK`, `LUMEN_BENCHMARK_ARGS`, `LUMEN_BENCHMARK_RANGE`,
  `LUMEN_MACRO_BENCHMARK`.
- Default `main` from `Meson/BenchRunner.cpp` (override with a local
  `Main.cpp`); second argv = optional JSON report path.

```sh
meson benchmark -C Build                     # or run Build/Source/Meson/Benchmarks/<Module>Benchmarks
```

- Benchmarks detect an optimized build via `__OPTIMIZE__`
  (`LUMEN_BENCHMARK_OPTIMIZED_BUILD`); run them on an optimized build or the
  numbers are meaningless.
