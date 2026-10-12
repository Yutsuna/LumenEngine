# `.agents/` — How to Contribute to Lumen (Human or AI)

This directory is the portable, versioned source of truth for the agents, rules, and skills that frame every contribution to **Lumen**, a research-oriented high-performance 3D rendering engine written in C++26.

```
.agents/
  rules/    always-on constraints every change must satisfy
  agents/   specialist role charters
  skills/   step-by-step recipes for common, repeatable edits
```

## How these files are consumed

Know what the tooling actually loads, so you do not assume support that does not exist:

- `AGENTS.md` (repository root) and this file are injected into every session as project instructions.
- `.agents/skills/*.md` are discovered as skills. They are only *listed* by description; a skill's body enters the conversation only when you load it with the skill tool. Skill IDs are derived from the file name (e.g. `meson-build`).
- `.agents/rules/*.md` and `.agents/agents/*.md` are **not** auto-loaded and `.agents/agents/*.md` are **not** registered as spawnable subagents. Read the relevant rule before touching code, and read the matching role charter before taking on a specialist task. To delegate, pass a charter's content to an available general-purpose subagent yourself.
- Paths inside this document and inside rules are relative to the repository root unless stated otherwise.

## Instruction Precedence

```text
1. Repository / build system / compiler reality
2. Root AGENTS.md
3. .agents/AGENTS.md (this file)
4. .agents/rules/           — binding once read
5. .agents/agents/           — role charters
6. .agents/skills/           — procedures
7. Ad-hoc notes, prior-agent summaries
```

The task defines scope; the rules define constraints. If a task demands what a rule forbids, surface the conflict. No instruction overrides repository reality or `rules/validation.md` honesty requirements.

## Default Agent Workflow

Every agent must follow this workflow unless a task explicitly states otherwise:

1. **Understand the task.** Clarify ambiguities before writing code.
2. **Inspect the repository.** Read `meson.build`, `Source/meson.build`, affected module `meson.build` files, and relevant C++ source/headers. Check `rules/` for the areas you touch.
3. **Identify constraints.** Module boundaries, dependency direction, backend abstraction, thread-safety assumptions, and public API impact.
4. **Formulate the smallest coherent plan.** Prefer minimal, focused changes.
5. **Implement.** Follow `rules/cpp.md` (style and mandatory file header) and `rules/architecture.md`.
6. **Build affected targets.** Use Meson/Nix; do not introduce CMake. See `rules/build.md`.
7. **Run relevant tests or validation.** Use the sanitizer mechanisms that actually work (see `rules/validation.md`). Record command results verbatim.
8. **Review.** Apply the `code-review` skill (optionally through the `lumen-reviewer` charter).
9. **Report.** Use the reporting format in `rules/validation.md`: summary, affected areas, architectural impact, validation actually performed, performance evidence, known limitations.

## Non-Negotiables

- **Existing repository is the source of truth.** Inspect before modifying. Do not fabricate repository state, and treat known legacy inconsistencies (listed below) as facts, not as assumptions to "fix" opportunistically.
- **Meson and Nix remain the build and dependency systems.** Meson is the build-system authority; Nix is the reproducible dependency/environment authority. Do not introduce CMake, Bazel, Premake, or alternative package managers.
- **Backend independence.** Engine-level code must not depend on Vulkan, OpenGL, X11, SDL, or any other backend-specific detail. Backend code lives behind the RHI (rendering) and Window (platform) abstractions; these are two independent dimensions. See `rules/architecture.md`.
- **Research-grade quality.** Avoid undefined behavior, data races, hidden allocations, ambiguous ownership, and silent failures.
- **Measurement before optimization.** Performance claims require evidence from profiling, benchmarking, or an explicitly stated theoretical basis. See `rules/performance.md`.
- **No fake validation.** Distinguish `Passed`, `Failed`, `Not run`, and `Not applicable`. Never present a belief as an observed result. See `rules/validation.md`.
- **Mandatory file header.** Every new C/C++ file starts with the `/// @file` / `/// @brief` header defined authoritatively in `rules/cpp.md`.
- **Professional technical English.** All identifiers, comments, documentation, error messages, and logs must be in English.
- **Project name is Lumen.** Never introduce placeholder names (TrafficEngine, Volt, EngineName, ...). Legacy occurrences are documented under "Known repository state".

## Known repository state

Verified against the repository at the time of this audit. Treat these as facts; do not silently "fix" them unless the task is exactly that:

| State | Detail |
| --- | --- |
| Legacy package name | `Nix/package.nix` uses `pname = "TrafficEngine"`; `nix build` logs and the installed derivation use that name. The Meson project and all code use `Lumen`. |
| Registered modules | Only `Core`, `AppCore`, `Launch` exist and are listed in `Source/meson.build`. `Math`, `Physics`, `Rendering`, `RHI`, `Window` are planned, not present. |
| Inconsistent file headers | Legacy files use two header styles (`/// @file <FileName>` or a bare `/// <Module>/<Path>/<FileName>`); `Source/Core/Public/Core/Test.hpp` has none. The mandated format applies to new files. |
| Namespace defect | `Source/Core/Public/Core/EnumFlags.hpp` references `LumenEngine::Bool`, but the namespace is `Lumen`. The file compiles only because nothing includes it. |
| Sanitizer options inert | `meson.options` declares `enable_asan`, `enable_ubsan`, `enable_tsan`, `enable_lto`, `enable_testing`, but no `meson.build` file calls `get_option()` on them; they currently have no effect. Use Meson's `b_sanitize` / `b_lto` instead (see `rules/build.md`). |
| No test suite | `meson test` reports "No tests defined." `gtest` is available in the Nix dev shell but not wired into Meson. The `enable_testing` option is unused. |
| No CI | `.github/workflows/` exists but is empty. |
| Empty file | `Source/AppCore/Public/Generic/GenericApplication.hpp` is zero bytes (work in progress). |
| Formatting drift | `Source/AppCore/**` does not currently pass `clang-format`; Core and Launch files do. |
| Unused export macro | `LUMEN_API` in `Core/CoreDefinitions.hpp` has no wiring (`LUMEN_BUILDING_DLL` is never defined). Cross-module symbols use the generated `<MODULE>_EXPORT` from `<Module>Export.hpp`. |

## Rule Index

| Rule | Concern |
| --- | --- |
| [`rules/general.md`](rules/general.md) | Repository honesty, minimal changes, dependency discipline, project naming |
| [`rules/cpp.md`](rules/cpp.md) | C++26, Unreal/Epic style, file headers, includes, RAII, UB avoidance |
| [`rules/architecture.md`](rules/architecture.md) | Module boundaries, dependency direction, backend independence, multi-backend design |
| [`rules/performance.md`](rules/performance.md) | Measurement-first, evidence hierarchy, research reproducibility |
| [`rules/concurrency.md`](rules/concurrency.md) | Thread-safety docs, ownership across threads, locking discipline |
| [`rules/build.md`](rules/build.md) | Meson/Nix authority, module registration order, exports, compiler flags |
| [`rules/validation.md`](rules/validation.md) | Status vocabulary, build/test/sanitizer evidence, reporting format |

## Agent Index

Role charters, not spawnable subagents. Each states its own scope; compose them along the collaboration pipelines below.

| Charter | Responsibility | Does not own |
| --- | --- | --- |
| [`agents/lumen-architect.md`](agents/lumen-architect.md) | Module boundaries, dependency direction, API contracts, backend independence | Implementation details, code formatting |
| [`agents/lumen-cpp-engineer.md`](agents/lumen-cpp-engineer.md) | Default C++ implementation: lifetime, ownership, type safety, style | Domain design decisions (rendering, build) |
| [`agents/lumen-rendering-engineer.md`](agents/lumen-rendering-engineer.md) | Generic rendering layer, render passes, RHI interface design | Concrete Vulkan/OpenGL code, platform code |
| [`agents/lumen-backend-engineer.md`](agents/lumen-backend-engineer.md) | Concrete rendering/window backends behind RHI/Window abstractions | Generic rendering logic, abstractions' shape |
| [`agents/lumen-performance-engineer.md`](agents/lumen-performance-engineer.md) | Profiling, benchmarks, optimization evidence | Correctness sign-off, feature implementation |
| [`agents/lumen-build-engineer.md`](agents/lumen-build-engineer.md) | Meson, Nix, module registration, compiler flags | Source-code changes beyond build wiring |
| [`agents/lumen-validation-engineer.md`](agents/lumen-validation-engineer.md) | Tests, sanitizers, regression checks, validation evidence | Feature implementation, optimization |
| [`agents/lumen-reviewer.md`](agents/lumen-reviewer.md) | Pre-completion audit against all rules | Implementing fixes (reports only) |

Intended collaboration (documentation; no orchestration is enforced by the tooling):

```text
Architecture change     → Architect → C++ Engineer → Reviewer
Rendering backend work  → Rendering Engineer → Backend Engineer → C++ Engineer → Reviewer
Performance work        → Performance Engineer → domain Engineer → Reviewer
Build system work       → Build Engineer → C++ Engineer → Reviewer
Any non-trivial change  → ... → Reviewer (code-review skill)
```

## Skill Index

| Skill | Use When |
| --- | --- |
| [`skills/cpp26-development.md`](skills/cpp26-development.md) | Writing or editing C++ source |
| [`skills/lumen-architecture.md`](skills/lumen-architecture.md) | Designing or changing module structure |
| [`skills/unreal-cpp-style.md`](skills/unreal-cpp-style.md) | Naming, formatting, and layout decisions |
| [`skills/meson-build.md`](skills/meson-build.md) | Adding modules, targets, dependencies |
| [`skills/nix-deps.md`](skills/nix-deps.md) | Changing reproducible dependencies |
| [`skills/rhi-backend-dev.md`](skills/rhi-backend-dev.md) | Working on RHI or a rendering/window backend |
| [`skills/rendering-pipeline.md`](skills/rendering-pipeline.md) | Designing rendering passes or pipelines |
| [`skills/simd-optimization.md`](skills/simd-optimization.md) | Optimizing with SIMD |
| [`skills/parallel-programming.md`](skills/parallel-programming.md) | Adding concurrency or parallelism |
| [`skills/performance-analysis.md`](skills/performance-analysis.md) | Profiling or benchmarking |
| [`skills/testing-validation.md`](skills/testing-validation.md) | Adding or running tests and sanitizers |
| [`skills/code-review.md`](skills/code-review.md) | Reviewing a change before finishing |
| [`skills/api-design.md`](skills/api-design.md) | Designing public interfaces |
| [`skills/documentation.md`](skills/documentation.md) | Writing comments or docs |
