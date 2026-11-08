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
3. Found → use the Core type (`TVector`, `FStringView`, `FMutex`…) instead
   of `std::vector`, `std::string_view`, `std::mutex`, … in your code.
4. Not found → place it where it belongs:
   - Genuinely generic and reusable by all modules → add it to **Core**
     (Public + Private + tests), then depend on it.
   - Only meaningful to one module → keep it in that module, but follow the
     same names/patterns as Core so it can graduate later.
5. If a std construct exists but Core's version adds guarantees (header-only,
   no allocation on hot path, i-cache-friendliness, wait-free semantics),
   Core's version wins: prefer `TVector`/`FStringView` over std in all
   public signatures and hot paths.

## TOptional / TExpected / Monadic are mandatory

**Lumen is `noexcept` by definition — no exceptions are tolerated anywhere
in engine code.** Absence and failure therefore cannot be signalled by
throwing; they are part of the type system:

- **Absence** → `TOptional<T>` (`Templates/Optional.hpp`).
- **Failure** → `TExpected<T, E>` (`Templates/Expected.hpp`,
  pre-built errors carried by `TUnexpected` from `Templates/Unexpected.hpp`).
- **Pipeline** → combine results with the **Monadic** operations of
  `Templates/Monadic.hpp`: `Transform` (T->U), `AndThen` (bind into another
  valid value), `OrElse` (recover from an error). Do not unwrap with ad-hoc
  `if (opt) {...}` chains across a public API; express the whole flow
  monadically.

Why this is enforced:

- **Paradigm** — error handling and absence are explicit in signatures;
  `Transform`/`AndThen`/`OrElse` compose failure handling once, instead of
  repeating checks at every call site.
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
  provides wait-free queues and triple buffers for that (see `modules/Core.md`).
- Do not copy-paste a utility into your module "temporarily"; make it either
  a Core addition (with tests) or a properly-namespaced private helper.
