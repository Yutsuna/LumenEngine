---
name: unreal-cpp-style
description: Quick reference for applying Lumen's Unreal/Epic-inspired C++ style.
---

# Unreal/Epic C++ Style

Use this skill when making naming, formatting, or layout decisions in Lumen C++ code.

## Style, not engine technology

Lumen is **inspired by Epic's coding style**, not a fork of Unreal Engine.

- **In scope (style):** naming, class/file organization, readability, API conventions, header/source organization, the formatting rules below.
- **Out of scope (Unreal technology):** do not import Unreal Engine macros, reflection/UObject systems, `TArray`/`TSharedPtr` and other UE containers, `GENERATED_BODY`, `FMath` UE idioms, or any Unreal-specific architecture into Lumen — unless a task explicitly requires a specific UE mechanism, with justification.
- Lumen equivalents come from the standard library (`std::span`, `std::vector`, `std::unique_ptr`, ...) or from `Core` (e.g. `TAtomic` in `Core/CoreTypes.hpp`).

## Naming

| Construct | Convention | Example |
| --- | --- | --- |
| Class/struct/enum | `PascalCase` | `FGenericWindow` |
| Function | `PascalCase` | `Clear`, `Show` |
| Template parameter | `PascalCase` with `T` prefix | `TAtomic<Type>` |
| Concept | `PascalCase` with `C` prefix (optional) | `CContainer` |
| Boolean variable | `b` prefix | `bVisible` |
| Member variable | one module-consistent style | follow existing module |
| Module export macro | `<MODULE>_EXPORT` (generated) | `CORE_EXPORT` |
| Other macros | `ALL_CAPS` with `LUMEN_` prefix | `LUMEN_PLATFORM_LINUX` |
| Namespace | `Lumen` only | `namespace Lumen` |

## Formatting (from `.clang-format` — authoritative)

- Allman braces; 4-space indent; 130-column limit; `clang-format` is the final word.
- **Function declarations and definitions:** space before and inside the parens — `void Clear ();`, `void f ( int a )`.
- **Function calls:** space inside the parens, none before — `f( a )`, empty parens — `Test()`.
- **Control statements:** `if ( Condition )`, `while ( Condition )`.
- **C-style casts (only where unavoidable):** `( void )Argc` — spaces inside.
- Pointer alignment right: `const AnsiChar *Argv[]`.
- Public API first, private state last within a class; one declaration/statement per line.

## Layout

- Keep functions focused and short when possible.
- Match the surrounding file; do not re-style untouched code (see `rules/cpp.md`).

## Checklist

1. Every new file has the mandatory header (format in `rules/cpp.md`).
2. `clang-format` run on new files (touched lines only for legacy files).
3. Build confirms no warnings under `-Werror`.
