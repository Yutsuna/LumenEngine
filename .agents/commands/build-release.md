---
description: Configure and compile the release build used for benchmarks
---

# build-release

Run inside the Nix devshell (direnv / `nix develop`).

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

Optional: link-time optimisation (`meson.options` gate, release-only):

```sh
meson setup Build-Release --buildtype=release -Denable_lto=true
```

Notes:

- This is the build benchmarks must run against — `LUMEN_BENCHMARK_OPTIMIZED_BUILD`
  detects `__OPTIMIZE__` and the `debug-sanitizer`/debug numbers are not
  meaningful (see `rules/testing.md`).
- `Build-Release/` is gitignored; never commit it.
