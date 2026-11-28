---
description: Configure and compile the debug build with ASan/UBSan sanitizers
---

# build-debug

Run inside the active local toolchain: direnv loads it via `.envrc` (Nix
devshell on Linux/macOS, MSYS2 UCRT64 on Windows). On Windows without direnv,
prefix commands with `.\Windows\Setup.ps1 -Command "…"`.

If `Build/` exists and is already configured:

```sh
meson setup Build \
    --buildtype=debug \
    -Db_sanitize=address,undefined \
    -Denable_testing=true \
    --reconfigure
ninja -C Build
```

If `Build/` does not exist yet (fresh clone), drop `--reconfigure`
(it errors on an unconfigured directory):

```sh
meson setup Build \
    --buildtype=debug \
    -Db_sanitize=address,undefined \
    -Denable_testing=true
ninja -C Build
```

Notes:

- Slows execution; use for debugging, not for benchmark numbers (see the
  `write-benchmarks` skill).
- Tests give useful stack traces under sanitizers: run
  `meson test -C Build` or the binaries directly under
  `Build/Source/Meson/Tests/`.
- Any source file add/remove needs another `--reconfigure` (see
  `rules/build.md`).
