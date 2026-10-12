---
name: testing-validation
description: Workflow for adding tests and validating changes in Lumen.
---

# Testing and Validation

Use this skill when adding tests, validating a change, or setting up CI.

## Adding tests

1. Place tests near the code they validate, following the project's conventions.
2. Use the testing framework already in the Nix shell (currently `gtest`).
3. Register tests in Meson so `meson test` discovers them.
4. Keep tests deterministic and independent.

## Validation checklist

1. **Format:** `clang-format` on touched C/C++ files.
2. **Build:** `ninja -C <builddir>` with `-Werror`.
3. **Tests:** `meson test -C <builddir>`.
4. **Static analysis:** compiler warnings, `clang-tidy` if configured.
5. **Sanitizers:**
   - ASan/UBSan for memory, lifetime, or UB changes: `-Denable_asan=true -Denable_ubsan=true`.
   - TSan for concurrency changes: `-Denable_tsan=true`.

## What must not be done

- Do not imply that a successful compilation proves correctness.
- Do not imply that unit tests prove performance.
- Do not imply that one backend's successful execution validates all backends.
- Do not claim success without running and observing the result.
