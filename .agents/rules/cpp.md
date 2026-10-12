# Rule: C++ development

**Applies to:** every `.c`, `.cc`, `.cpp`, `.h`, `.hpp` file added or edited under `Source/`.

## Language and toolchain

- Engine code is **C++26**; C interop code is **C23** (`c_std=c23`). Both are set in the root `meson.build`.
- The default toolchain is GCC via Nix (currently GCC 16). Clang tooling (`clang-format`) is used for formatting, not compilation.
- `warning_level=0` is set at project level, but `Source/Meson/ModuleBuilder/meson.build` compiles every module with `ALL_WARNINGS` from `Source/Meson/meson.build`, which includes `-Werror`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wold-style-cast`, `-Wnon-virtual-dtor`, and `-Woverloaded-virtual`. Warnings are failures for module code.
- Prefer modern C++ features only when they improve correctness, expressiveness, compile-time guarantees, performance, or maintainability.

## Mandatory file header

Every **new** C/C++ source or header file must begin with this header, before `#pragma once`, include guards, or any code:

```cpp
///
/// @file <Module>/<Path>/<FileName>
/// @brief <File responsibility>
///
```

- `<Module>/<Path>/<FileName>` is the file's path **relative to `Source/`**, including `Public/` or `Private/`. Example: `/// @file AppCore/Public/Generic/GenericWindow.hpp`.
- The `@brief` states what the file is responsible for (what it owns), not where it lives. Headers such as `/// @brief Header file.` are prohibited.
- This format is authoritative here. Existing files use two legacy variants (file-name only, or a bare path without `@file`/`@brief` tags) and one file has no header at all; see `.agents/AGENTS.md`. Do not rewrite legacy headers unless the task covers them.

## Naming, namespace, and includes

- The project namespace is `Lumen`. Do not introduce additional top-level namespaces; `LumenEngine` references (e.g. in `EnumFlags.hpp`) are a known defect, not a convention.
- Follow the existing **Unreal/Epic-inspired** conventions (see `skills/unreal-cpp-style.md`):
  - Types and functions: `PascalCase` (`FGenericWindow`, `Clear`)
  - Template parameters: `PascalCase` with a leading `T` (`TAtomic<Type>`)
  - Boolean variables: `b` prefix (`bVisible`)
  - Member variables: one module-wide convention; do not mix styles within a module.
- Includes are angle-bracket paths **relative to the including module's `Public/` include root**, e.g. `#include <Core/CoreTypes.hpp>`, `#include <Generic/GenericWindow.hpp>`. The build system propagates dependent modules' `Public/` roots.
- Cross-module symbols in shared modules must be declared with the generated export macro, e.g. `CORE_EXPORT void Test ();` via `#include <CoreExport.hpp>`. Do not rely on `LUMEN_API`; it is not wired for module builds.

## Formatting

- Formatting is mechanical: run `clang-format` with the repository `.clang-format` (LLVM base, Allman braces, 4-space indent, 130 columns, spaces inside parentheses).
- Format **new files completely**. For pre-existing files, format only the lines you touched (`clang-format --lines=<start>:<end>` or `git clang-format`); several legacy files do not currently pass a full `clang-format` run, and reformatting them wholesale violates `rules/general.md` change discipline. Full reformatting is only in scope when the task says so.
- Respect `.editorconfig`: 4-space indentation for C/C++/Meson files, LF line endings, trimmed trailing whitespace.

## Code quality

- Prefer RAII and explicit ownership. Raw pointers are non-owning references or low-level interop only.
- Preserve const-correctness. Mark accessors `const` and mutation paths non-const.
- Avoid undefined behavior: uninitialized reads, out-of-bounds access, strict-aliasing violations, signed overflow, data races.
- Avoid unnecessary allocations, copies, virtual dispatch, and hidden synchronization.
- Use `[[nodiscard]]` on pure accessors, factories, and functions where ignoring the result is almost certainly a bug.
- Use `noexcept` only where the function genuinely does not throw and callers may rely on that contract.
- Prefer `std::span`, `std::expected`, concepts, `constexpr`/`consteval`, scoped enums, and strong types where appropriate.
