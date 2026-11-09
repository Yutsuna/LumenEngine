---
description: Compile and run the Lumen test suites (all, one module, or one binary filtered)
---

# test

Inside the Nix devshell. Debug build is enough for tests; sanitizers pair
well with them (see `build-debug`).

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
- Test binaries also see their module's `Private/` headers; Core is linked
  automatically (`rules/testing.md`).
