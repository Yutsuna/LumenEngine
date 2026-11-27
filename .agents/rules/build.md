# Build rules

## Commands

All commands run in the Nix devshell (`.envrc`/`direnv` wires it
automatically). CI invokes each one explicitly through `nix develop`
(`.github/workflows/Ci.yml`):

```sh
nix develop --command meson setup Build     # first configure; Build/ is gitignored
nix develop --command meson compile -C Build
nix develop --command meson test -C Build
```

**Three build trees — compile ALL of them at the end of every task:**

| Tree | Config |
| --- | --- |
| `Build` | Debug, ASan+UBSan (`b_sanitize=address,undefined`) |
| `Build-Release` | Release, `-O3` |
| `Build-TSAN` | Debug, TSan (`b_sanitize=thread`) |

Release-only failures happen (e.g. GCC `-Warray-bounds` false positives that
only fire at `-O3` through speculative devirtualization): never declare a task
done on the debug build alone.

Focused verification:

```sh
meson test -C Build --suite Scene           # one module's suite (suite name = module name)
meson test -C Build SceneTests              # one named test target
./Build/Source/Meson/Tests/SceneTests       # test binary directly (filter = argv[1])
```

**Gotcha — reconfigure after source changes:** `Private/`, `Tests/`, and
`Benchmarks/` sources are discovered by `find` at configure time
(`Source/Meson/{Private,Tests,Benchmarks}/meson.build`). After adding,
deleting, or renaming any `.cpp`, reconfigure.

**Prefer reconfiguring via `Scripts/BuildMeson.bash`.** It reconfigures all
three build trees in one shot with the correct settings (`--native-file`,
buildtype, and sanitizer flags), so no tree is left stale:

```sh
Scripts/BuildMeson.bash
```

The script runs `meson setup ... --reconfigure` for `Build`, `Build-Release`,
and `Build-TSAN` in parallel. Only fall back to a manual reconfigure (e.g.
`meson setup Build --reconfigure`) when you intentionally want to touch a
single tree or need different flags.

## Toolchain (do not assume defaults)

- Compiler **GCC 16**, C++ **26**, C **23**, `-freflection` passed
  project-wide (`meson.build`). Meson >= 1.10. The GCC 16 pin lives in the
  Nix layer (`gcc16` in `Nix/deps.nix`, `gcc16Stdenv` in `Nix/package.nix`);
  `Meson/Native.ini` wraps those compilers with `ccache` (`ccache gcc` /
  `ccache g++`).
- clangd cannot parse reflection code; `.clangd` strips `-freflection` for
  tooling. Don't "fix" that config by adding it back.
- `meson.options`: `enable_testing`, `enable_benchmarking` (default true),
  `enable_lto` (release buildtype only, default false).
- The project default is `b_ndebug=if-release`: `LUMEN_ASSERT` traps in Debug
  and compiles out in Release. Keep Debug assertion-enabled when testing
  preconditions.
- Nix devshell (`Nix/shell.nix`, `Nix/deps.nix`): GCC, Meson, Ninja, SDL3,
  Vulkan loader/headers, clang-tools, shaderc, spirv-tools and spirv-cross;
  exports `LD_LIBRARY_PATH`, sets `VK_LAYER_PATH` to validation layers, links
  with `mold`. Shader tooling is available there; no shader generation target
  exists yet.

## Warnings behave like errors

`Source/Meson/meson.build` defines `ALL_WARNINGS` (`warning_level=0` +
explicit flags), including `-Werror`, `-Wconversion`, `-Wsign-conversion`,
`-Wold-style-cast`, `-Wshadow`, `-Wdouble-promotion`. Code must be
warning-clean or the build fails. `-Og` is injected at `optimization=0` to
silence a glibc `_FORTIFY_SOURCE` warning.

Any target built outside the module system will not receive these flags:
put everything through module definitions (`Source/Meson/`).
