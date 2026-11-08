# Lumen — Agent Instruction Index

Lumen is a C++26 rendering engine (Epic Games / Unreal-style coding standard)
built with **Meson** inside a **Nix flake** devshell. Code is organised as one
module per directory under `Source/`.

## Read on demand

Load the rule file for the area you are touching **before** editing, and
treat it as mandatory. Do not preload everything.

| File | Read when… |
| --- | --- |
| `rules/coding-style.md` | writing or reviewing any C++ |
| `rules/core-first.md` | any new function/class/utility — check Core before writing |
| `rules/build.md` | configuring, compiling, running tests/benchmarks |
| `rules/testing.md` | adding or running tests or benchmarks |
| `workflows/git-workflow.md` | creating commits or PRs |
| `modules/<Module>.md` | working in `Source/<Module>/` (status, links, inventory) |
| `agents/` | agent definitions — `reviewer` orchestrator + `reviewer-safety`/`reviewer-norme` subagents, and `documentation` auditor of the `.agents/` corpus (definitions in `agents/*.md`) |
| `commands/` | ready-made recipes: `build-debug`, `build-release`, `test`, `review`, `new-module` |
| `skills/` | procedural guides: `debug-sanitizer`, `write-benchmarks`, `nix-workflow` |

## Modules

| Module | Status | Doc |
| --- | --- | --- |
| `Core` | built (shared library) | `modules/Core.md` |
| `Maths` | built (shared library) | `modules/Maths.md` |
| `Scene` | built (shared library) | `modules/Scene.md` |
| `AppCore` | built (shared library) | `modules/AppCore.md` |
| `Launch` | built (executable entry point) | `modules/Launch.md` |
| `Renderer`, `RHI`, `RHIOpenGL`, `RHIVulkan` | **not built yet** (empty stubs, absent from `ModuleNames`) | `modules/*.md` |

A module is only compiled if listed in `ModuleNames` (`Source/meson.build`).

## Fast facts (always apply)

- All commands run inside the Nix devshell (`.envrc`/`direnv`), e.g.
  `nix develop --command meson compile -C Build`. `Build/` is gitignored.
- `VERSION.md` is a single line read by `meson.build` — it *is* the project
  version. Keep it to one line.
- Test/benchmark binaries exit with **84** on failure, not 1.
