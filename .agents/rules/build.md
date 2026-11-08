# Build rules

## Commands

All commands run in the Nix devshell (`.envrc`/`direnv` wires it
automatically). What CI runs exactly (`.github/workflows/Ci.yml`):

```sh
nix develop --command meson setup Build     # first configure; Build/ is gitignored
nix develop --command meson compile -C Build
nix develop --command meson test -C Build
```

Focused verification:

```sh
meson test -C Build --suite Scene           # one module's suite (suite name = module name)
meson test -C Build SceneTests              # one named test target
./Build/Source/Meson/Tests/SceneTests       # test binary directly (filter = argv[1])
```

**Gotcha — reconfigure after source changes:** `Private/`, `Tests/`, and
`Benchmarks/` sources are discovered by `find` at configure time
(`Source/Meson/{Private,Tests,Benchmarks}/meson.build`). After adding,
deleting, or renaming any `.cpp`, run:

```sh
meson setup Build --reconfigure
```

## Toolchain (do not assume defaults)

- Compiler **GCC 16** (forced in `Meson/Native.ini`), C++ **26**, C **23**,
  `-freflection` passed project-wide (`meson.build`). Meson >= 1.10.
- clangd cannot parse reflection code; `.clangd` strips `-freflection` for
  tooling. Don't "fix" that config by adding it back.
- `meson.options`: `enable_testing`, `enable_benchmarking` (default true),
  `enable_lto` (release buildtype only, default false).
- Nix devshell (`Nix/shell.nix`, `Nix/deps.nix`): GCC, Meson, Ninja, SDL3,
  Vulkan loader/headers, clang-tools; exports `LD_LIBRARY_PATH`, sets
  `VK_LAYER_PATH` to validation layers, links with `mold`.

## Warnings behave like errors

`Source/Meson/meson.build` defines `ALL_WARNINGS` (`warning_level=0` +
explicit flags), including `-Werror`, `-Wconversion`, `-Wsign-conversion`,
`-Wold-style-cast`, `-Wshadow`, `-Wdouble-promotion`. Code must be
warning-clean or the build fails. `-Og` is injected at `optimization=0` to
silence a glibc `_FORTIFY_SOURCE` warning.

Any target built outside the module system will not receive these flags:
put everything through module definitions (`Source/Meson/`).
