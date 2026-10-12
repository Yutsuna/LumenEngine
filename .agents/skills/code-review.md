---
name: code-review
description: Reusable code-review checklist for Lumen changes.
---

# Code Review

Use this skill before finishing any non-trivial change.

## Correctness

- [ ] No undefined behavior (uninitialized reads, out-of-bounds, strict aliasing, signed overflow, data races).
- [ ] Lifetimes are explicit and correct across function/struct/thread boundaries.
- [ ] Error conditions are handled; no silent failures.
- [ ] API contracts are clear.

## Architecture

- [ ] Module boundaries respected; no circular dependencies.
- [ ] Backend-specific code isolated behind RHI/Window abstractions.
- [ ] Public API changes are documented and minimal.
- [ ] Abstractions add semantic value, not just forwarding layers.

## Performance

- [ ] No unnecessary allocations or copies.
- [ ] Data structures are cache-friendly for their access pattern.
- [ ] Synchronization is necessary and proportionate.
- [ ] No speculative optimization without measurement.

## Concurrency

- [ ] Thread-safety documented.
- [ ] No data races, deadlocks, or unsafe cross-thread lifetimes.
- [ ] Lock ordering documented where multiple locks are held.

## Maintainability

- [ ] Ownership is clear.
- [ ] Naming and style follow `rules/cpp.md` and `.clang-format`.
- [ ] New files have the mandatory header.
- [ ] Documentation explains why, not just what.

## Build and style

- [ ] Meson files updated if needed.
- [ ] Shared symbols exported with `<MODULE>_EXPORT`.
- [ ] `clang-format` run on touched files.
- [ ] Build passes under `-Werror`.

## What must not be done

- Do not turn "I believe this compiles" into "Build passed" without executing the build and observing the result.
- Do not turn "should be thread-safe" into "Thread-safe" without sufficient evidence (TSan run or documented contract).
- Do not imply that a successful compilation proves correctness.
- Do not imply that unit tests prove performance.
- Do not imply that one backend's successful execution validates all backends.
- Do not accept claims of performance improvement without measurement or theoretical basis.

## Output
