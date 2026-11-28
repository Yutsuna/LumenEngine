---
description: Configure and compile the release build used for benchmarks
---

# build-release

Run inside the active local toolchain: direnv loads it via `.envrc` (Nix
devshell on Linux/macOS, MSYS2 UCRT64 on Windows). On Windows without direnv,
prefix commands with `.\Windows\Setup.ps1 -Command "…"`.

If `Build-Release/` does not exist yet:

```sh
meson setup Build-Release --buildtype=release
ninja -C Build-Release
```

If it does:

```sh
meson setup Build-Release --buildtype=release --reconfigure
ninja -C Build-Release
```

`-Denable_lto=true` is currently a no-op: `meson.options:8` declares the
option, but no meson file calls `get_option('enable_lto')` or sets `b_lto`.
Do not present it as working LTO (`skills/meson-build-customization.md`).

Notes:

- This is the build benchmarks must run against — `LUMEN_BENCHMARK_OPTIMIZED_BUILD`
  detects `__OPTIMIZE__` and the `debug-sanitizer`/debug numbers are not
  meaningful (see `rules/testing.md`).
- `Build-Release/` is gitignored; never commit it.
