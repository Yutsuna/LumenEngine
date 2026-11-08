# Coding style rules

The standard is **Epic Games / Unreal Engine style applied to C++26**:
senior-quality, modular code first. Prefer the clean design over the quick
hack, and prefer reusing Core (`rules/core-first.md`).

## Naming prefixes

| Prefix | Applies to |
| --- | --- |
| `A` | Actors (`AActor`, verified existing convention) |
| `F` | every other object/framework type (`FMailBox`, `FLogger`, `FPath`) |
| `T` | templates (`TVector`, `TOptional`, `TTripleBuffer<T>`) |
| `E` | enums (`EUnitTestKind`) |
| `I` | pure interfaces (`IFileHandle`, `IFileSystem`) |
| `b` | bools — locals, params, members (`bIsLocked`, `bInParallelPhase`) |

- **PascalCase everywhere**: types, functions, files, enums values, params,
  locals, members. No snake_case, no camelCase.
- **Never single-letter variables.** No `int i`; write `Index`, `Attempt`,
  `ChannelIndex`… The name must say what it holds without reading the docs.
- Use Lumen typedefs over raw/std types in all code: `Int32/UInt32/Bool/USize`,
  `FStringView`, `InOut` style params via reference — not `int`,
  `bool` params into public APIs, not `std::string_view`.

## Error handling: noexcept by definition

- **Lumen is `noexcept` by definition — zero exceptions.** Engine code
  (`.hpp`/`.inl`/`.cpp`, non-test) must never `throw` and never `catch`:
  no exceptions are tolerated as a control flow, by paradigm, safety, and
  performance (no unwinding paths, optimiser-friendly).
  Constructor / destructor / virtual overrides are all `noexcept`
  (`AActor::Receive`, `AActor::~AActor` are the canonical pattern).
- Absence of a value is **`TOptional<T>`**; failure is
  **`TExpected<T, E>`** (`Templates/Expected.hpp` + `Unexpected.hpp`),
  both deriving from `FMonadic` and pipelined with the monadic operations of
  `Templates/Monadic.hpp` (`Transform`, `AndThen`, `OrElse`). No
  exceptions, nothing thrown — see `rules/core-first.md`.
- Do not call std APIs defined to throw (`std::vector` growth producing
  `bad_alloc`, `std::format` on unbuffered failure paths, `std::stoi`…);
  prefer the Core equivalents that return `TOptional`/`TExpected` instead.
- The sole exception (literally) is the **test framework** :
  `LUMEN_CHECK*` macros `throw FUnitTestAssertionFailure`, which the test
  runner catches/reports. Only test code may throw.

## Files: strict separation of concerns

| Extension | Contains |
| --- | --- |
| `.hpp` | signatures and definitions **only** (declarations, contracts) |
| `.inl` | inline/template implementations |
| `.cpp` | implementations |

- `.inl` files live in an `Inline/` directory next to their `.hpp`
  (`Public/Actor/Inline/ActorTypes.inl`), and the `.hpp` ends with a single
  `#include "Inline/<Name>.inl"`.
- Generated headers follow the same rules; wrap generated macros in
  `//clang-format off` … `//clang-format on`.

## Namespaces

- `.hpp`: open `namespace Lumen { }` (optionally a nested sub-namespace for
  the topic, e.g. `Lumen::Tests`).
- `.cpp` and `.inl`: **never** open `namespace Lumen { }` for the whole file.
  Qualify symbols explicitly with `Lumen::`.
  Internal helpers may be `static` or in an anonymous namespace.

## Comments

- Doxygen `/** */` blocks with `@file`, `@brief`, `@class`, `@details`,
  `@param`, `@return` — headers start with a `@file` + `@brief` block.
- One-liner notes use the same `/** */` doxygen style with the tags:
  `NOTE:`, `INFO:`, `TODO:` (uppercase, followed by colon, at the start of
  the comment, e.g. `/** NOTE: To be implemented by children */`).
- Comments explain intent and contracts, not restated code.

## Formatting

- Enforced by `.clang-format` (LLVM-based): Allman braces, 4-space indent,
  column limit 130, aligned consecutive assignments, **spaces inside
  parentheses** and before parameter lists: `Run ( const Int32 Argc )`.
  Don't "fix" this; see `Meson/TestRunner.cpp` for canonical examples.
- Everything builds with `-Werror` plus aggressive warnings
  (`-Wconversion`, `-Wsign-conversion`, `-Wold-style-cast`, `-Wshadow`…);
  code must be warning-clean.
