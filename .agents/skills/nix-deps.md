---
name: nix-deps
description: Workflow for managing reproducible dependencies with Nix in Lumen.
---

# Nix Dependencies

Use this skill when adding, updating, or removing project dependencies, or changing the dev environment.

**Authority:** Nix is the reproducible dependency/environment authority; Meson remains the build-system authority (`rules/build.md`). Project dependencies must be reproducible through Nix — no ad-hoc system packages the build depends on. Host tools for personal convenience (editor, `git`) are fine. Do not replace Nix with another package manager.

## Key files

- `flake.nix` — outputs: `packages.<system>.default`, `devShells.<system>.default`.
- `Nix/deps.nix` — shared dependency lists consumed by both shell and package.
- `Nix/shell.nix` — dev shell (gcc16, valgrind, gdb, gtest, clang-tools, jq; ccache + mold wiring).
- `Nix/package.nix` — package derivation (uses `gcc16Stdenv`; note its `pname` is the legacy `TrafficEngine` — known repository state, `.agents/AGENTS.md`).
- `.envrc` — direnv enters the shell via `use flake .#default`.

## Adding a dependency

1. Add the package to `Nix/deps.nix` under `nativeBuildInputs` (build tools) or `buildInputs` (libraries).
2. Confirm it is needed by both the dev shell and the package, or scope it accordingly (`shell.nix` adds dev-only tools on top of `deps`).
3. Wire the Meson side per `skills/meson-build.md` — remember external dependencies are not yet supported by the ModuleBuilder.
4. Document the dependency's purpose and version constraints.
5. Do not vendor dependencies under `ThirdParty/` without explicit justification.

## Validation

1. Enter the dev shell: `nix develop` (or `direnv allow`).
2. Reconfigure and build Meson inside the shell: `meson setup --reconfigure build-debug && ninja -C build-debug`.
3. Evaluate the package: `nix build --dry-run` (fast check that the derivation still evaluates). Note the output attribute is `default` — there is **no** `.#lumen` output; `nix build` or `nix build .#default` is correct.
4. Full package build: `nix build` (observed to succeed as of this audit; re-verify rather than assume).

## Constraints

- Do not introduce alternative package managers (Conan, vcpkg, ...) without explicit approval.
- Changing the toolchain (compiler, stdenv) is a build-system decision: involve `lumen-build-engineer` and record the rationale.

## What must not be done

- Do not introduce ad-hoc system dependencies when a dependency is expected to be reproducible through Nix: project dependencies must follow `Nix/deps.nix`.
- Do not replace Nix with another package manager.
- Do not vendor dependencies under `ThirdParty/` without explicit justification and documentation.
