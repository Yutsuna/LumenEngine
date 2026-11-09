---
name: Nix Workflow
description: How to change or use the Nix layer — flake.nix architecture, declarative deps in Nix/deps.nix, nix build/devshell usage
---

## Architecture (verified layout)

```
flake.nix            — entry point; systems: x86_64-linux, aarch64-linux,
                       x86_64-darwin, aarch64-darwin (forAllSystems)
  ├─ packages.default  = pkgs.callPackage ./Nix/package.nix   (the Lumen package)
  └─ devShells.default = pkgs.callPackage ./Nix/shell.nix     (the devshell)

Nix/deps.nix         — SINGLE declarative source of truth for inputs
Nix/package.nix      — gcc16Stdenv.mkDerivation; builds the engine
Nix/shell.nix        — mkShell wrapping deps with dev env hooks
```

- `Nix/deps.nix` is **the only place** to declare toolchain and libraries.
  `package.nix` and `shell.nix` both `import` it. Never add a dependency
  directly to the flake or to shell/package files.
- Choose the right list in `deps.nix`:
  - `nativeBuildInputs` — build-time tools (gcc16, meson, ninja, pkg-config;
    `mold`, `mesa` behind `lib.optionals … isLinux`).
  - `buildInputs` — runtime libs (sdl3, vulkan-loader, vulkan-headers).
  - `devInputs` — dev-only tools, present in the devshell but NOT in the
    package (valgrind, gdb, clang-tools, jq, nil, nixd, vulkan-tools,
    vulkan-validation-layers).
- Platform conditionals must follow the existing pattern
  (`lib.optionals pkgs.stdenv.hostPlatform.isLinux [...]`) — darwin is a
  supported target; don't leak Linux-specific inputs into it.

## shell.nix hooks (why the devshell behaves specially)

- `LD_LIBRARY_PATH` is composed from `deps.buildInputs` (`makeLibraryPath`)
  so runtime libs resolve without `nix build`.
- `VK_LAYER_PATH` points at the validation layers.
- Linux-only: `NIX_CFLAGS_LINK="-fuse-ld=mold"`.
- `.envrc` does `use flake .#default --accept-flake-config` + `PATH_add
  result/bin` — **after** a successful `nix build`, the freshly built
  binaries are available on PATH immediately.


## package.nix details agents must respect

- Version is read as the **first line of `../VERSION.md`** — keep that file
  single-line or the package version breaks.
- `src = pkgs.lib.cleanSource ../.` — gitignored junk is cleaned, but keep
  new stray directories out of the source tree anyway.
- The meson/ninja inputs bring the standard Meson/Nix build hooks; the
  package-level knobs are the `meson.options` fields (e.g. `-Denable_lto`).

## Commands

```sh
nix develop                      # enter the devshell (direnv does it automatically)
nix build                        # build packages.default → ./result symlink
nix flake show                   # inspect outputs
nix develop --command meson test -C Build   # CI-style, non-interactive
```

## Workflow when adding a dependency

1. Add it to the correct list in `Nix/deps.nix` (choose native/build/dev).
2. If a module consumes it via pkg-config, wire the meson side too:
   `dependency('...')` in that module's `ModuleDefinition` under
   `'ExternalDeps'` (see `Source/AppCore/meson.build` SDL3 example) —
   nix buildInputs and meson `dependency()` must match names.
3. `direnv reload` (or re-enter `nix develop`) so LD_LIBRARY_PATH/hooks pick
   the change; `nix build` only if you need the package/result.
4. If it's a runtime lib consumed by an executable, verify with the
   `Launch` binary in the devshell before the `nix build` route.
