---
description: Compile and run the Lumen test suites (all, one module, or one binary filtered)
---

# test

Inside the active local toolchain (direnv via `.envrc`: Nix devshell on
Linux/macOS, MSYS2 UCRT64 on Windows). Debug build is enough for tests;
sanitizers pair well with them (see `build-debug`).

```sh
meson compile -C Build && meson test -C Build          # everything
meson test -C Build --suite Scene                      # one module (suite = module name)
meson test -C Build SceneTests                         # one test target
./Build/Source/Meson/Tests/SceneTests Filter           # binary directly; argv[1] filters
```

Notes:

- Tests are in-house (`LUMEN_TEST`, `LUMEN_INTEGRATION_TEST` — no gtest);
  a failure exits **84**, not 1 (`Meson/TestRunner.cpp`).
- New/deleted/renamed test `.cpp` files need `meson setup Build --reconfigure`
  before they are visible to the build (`rules/build.md`).
- Test binaries also see their module's `Private/` headers. Test dependencies
  come from `GlobalTestModules` (`Source/Meson/Tests/meson.build`), which
  inherits the module's `Depends` — modules list `Core` there explicitly.
  Automatic Core injection is benchmark-only (`rules/testing.md`).
