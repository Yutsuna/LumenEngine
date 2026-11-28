# Lumen — Agent Instruction Index

Lumen is a C++26 rendering engine (Epic Games / Unreal-style coding standard)
built with **Meson** inside a **repository-local toolchain**: a Nix flake
devshell on Linux/macOS, a portable MSYS2 UCRT64 under `.win-env/` on Windows.
`.envrc` activates the right one for the host OS. Code is organised as one
module per directory under `Source/`.

## Read on demand

Load the rule file for the area you are touching **before** editing, and
treat it as mandatory. Do not preload everything. Document paths are relative
to `.agents/`; an explicit `.agents/` prefix is equally valid.

| File | Read when… |
| --- | --- |
| `rules/coding-style.md` | writing or reviewing any C++ |
| `rules/core-first.md` | any new function/class/utility — check Core before writing |
| `rules/build.md` | configuring, compiling, running tests/benchmarks |
| `rules/testing.md` | adding or running tests or benchmarks |
| `rules/replication.md` | working on M9 Scene replication — fixed arbitrages, wire contract, World integration, canonical replay |
| `workflows/git-workflow.md` | creating commits or PRs |
| `modules/<Module>.md` | working in `Source/<Module>/` (status, links, inventory) |
| `agents/` | agent definitions — `reviewer` orchestrator + `reviewer-safety`/`reviewer-norme` subagents, `documentation` auditor of the `.agents/` corpus, and `test-writer` for unit-test authoring (definitions in `agents/*.md`) |
| `commands/` | ready-made recipes: `build-debug`, `build-release`, `test`, `review`, `new-module` |
| `skills/` | procedural guides: `core-abstractions`, `debug-sanitizer`, `write-benchmarks`, `nix-workflow`, `windows-workflow`, `meson-build-customization` (declarative Meson mechanics: `ModuleDefinition` keys, target types) |

## Modules

| Module | Status | Doc |
| --- | --- | --- |
| `Core` | built (shared library) | `modules/Core.md` |
| `Maths` | built (shared library) | `modules/Maths.md` |
| `RHI` | built (shared library; Null + validation backends) | `modules/RHI.md` |
| `RHIOpenGL` | built (OpenGL 4.6; EGL on Linux/macOS, WGL on Windows; full conformance matrix on Linux) | `modules/RHIOpenGL.md` |
| `Scene` | built (shared library) | `modules/Scene.md` |
| `Renderer` | built (shared library; execution facade + render graph sources) | `modules/Renderer.md` |
| `AppCore` | built (shared library) | `modules/AppCore.md` |
| `Launch` | built (executable entry point) | `modules/Launch.md` |
| `RHIVulkan` | built (shared library, listed in `ModuleNames`; do not depend on it from production renderer code yet) | `modules/RHIVulkan.md` |

A module is compiled only if listed in `ModuleNames` (`Source/meson.build`) and
has a `meson.build` in its `Source/` directory.

**Private dependencies:** `ModuleDefinition` accepts a `PrivateDepends` key.
It is compiled/linked into the module's target and its tests/benchmarks, but is
absent from the `declare_dependency()` consumed by clients. `Renderer` uses it
for `RHI`/`RHIOpenGL`; `Launch` and clients must never declare or include a
backend.

## Fast facts (always apply)

- Three build trees exist and **all three must compile at the end of every
  task** — a task is not done until each one builds cleanly:
  1. `Build` — Debug, ASan+UBSan (`b_sanitize=address,undefined`)
  2. `Build-Release` — Release, `-O3`
  3. `Build-TSAN` — Debug, TSan (`b_sanitize=thread`)
- `.agents/` is the canonical agent-documentation directory. `.kimi-code` and
  `.opencode` are symbolic links to it: never edit files through those paths;
  all agent-documentation changes must be made directly under `.agents/`.
- All commands run inside the active local toolchain (direnv loads it via
  `.envrc`: Nix devshell on Linux/macOS, MSYS2 UCRT64 on Windows; CI uses
  `nix develop` explicitly), e.g. `meson compile -C Build`. Never rely on
  globally installed compilers or build tools. The root `.gitignore` covers `Build/` and
  `Build-Release/` only: `Build-TSAN/` is not listed there, but its own
  `Build-TSAN/.gitignore` (`*`) ignores it, so it does not appear in `git status`.
- Prefer the Lumen Core library over `std` (threads, locks, stop tokens,
  condition variables, containers, algorithms…). Use `std` only when Core has
  no equivalent yet — see `rules/core-first.md`.
- `VERSION.md` is a single line read by `meson.build` — it *is* the project
  version. Keep it to one line.
- Test/benchmark binaries exit with **84** on failure, not 1.
