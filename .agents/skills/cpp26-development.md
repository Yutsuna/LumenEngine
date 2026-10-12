---
name: cpp26-development
description: Step-by-step workflow for writing or editing C++26 code in Lumen.
---

# C++26 Development

Use this skill for every C++ implementation task in Lumen.

## Before writing

1. Read the target file and its immediate dependencies.
2. Read `rules/cpp.md` (mandatory file header, include convention, namespace, style) and `rules/concurrency.md` if any lifetime crosses threads.
3. Confirm the module's `Public/` include root and whether the symbol must be exported (`<MODULE>_EXPORT`).

## While writing

1. Start every new `.c`/`.cc`/`.cpp`/`.h`/`.hpp` file with the mandatory header, before `#pragma once` — exact format defined in `rules/cpp.md` (path relative to `Source/`, `@brief` states the file's responsibility).
2. Use `namespace Lumen`; angle-bracket includes relative to the module's `Public/` root; export cross-module symbols with `<MODULE>_EXPORT`.
3. Apply Unreal/Epic-inspired naming per `skills/unreal-cpp-style.md`: types/functions `PascalCase`, templates with leading `T`, booleans with `b` prefix.
4. Prefer RAII, explicit ownership, `std::span`, `std::expected`, concepts, and `constexpr` where appropriate.
5. Mark pure accessors `const` and `[[nodiscard]]`.
6. Document thread-safety, ownership, and lifetime in public headers.
7. Keep the code compilable under `ALL_WARNINGS` (`-Werror`, `-Wconversion`, `-Wold-style-cast`, `-Wshadow`, ...).

## Before finishing

1. Run `clang-format` on new files; on legacy files, format only the lines you touched (`rules/cpp.md`).
2. Build the affected module: `ninja -C <builddir>`; run `meson setup --reconfigure <builddir>` first if you added or removed source files (sources are discovered at configure time).
3. Run relevant tests (`meson test -C <builddir>`); note that no tests are currently registered.
4. Sanitizer validation when memory/lifetime/concurrency is touched: rebuild with `-Db_sanitize=address,undefined` or `-Db_sanitize=thread` (the `enable_asan`/`enable_tsan` options are inert — `rules/build.md`).
5. Report results with the status vocabulary from `rules/validation.md`.
