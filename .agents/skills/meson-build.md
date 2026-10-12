---
name: meson-build
description: Workflow for modifying Lumen's Meson build configuration, adding modules, or changing compiler settings.
---

# Meson Build

Use this skill when editing any `meson.build` file or build option. Authoritative constraints: `rules/build.md`.

## Key files

- `meson.build` — project entry point (C++26, C23, shared libraries).
- `meson.options` — declared options; note that `enable_testing`, `enable_asan`, `enable_ubsan`, `enable_tsan`, `enable_lto` are currently **unread** and therefore inert (`rules/build.md`).
- `Source/meson.build` — `ModuleNames` registration list; drives everything downstream.
- `Source/Meson/meson.build` — `ALL_WARNINGS` (includes `-Werror`).
- `Source/Meson/ModuleBuilder/meson.build` — target creation, `GlobalModules`, export header generation, source discovery.
- `Source/<Module>/meson.build` — per-module `ModuleDefinition` dict (`Name`, `Type`, `SourceDir`, `Private`, `Public`, `Depends`).
- `Meson/ExportTemplate.hpp.inl` — `<MODULE>_EXPORT` template.

## Adding a module

1. Run `Scripts/NewModule.py <ModuleName>` to scaffold `Public/`, `Private/`, and the `ModuleDefinition` (the script does **not** register the module).
2. Edit `Source/<Module>/meson.build`: set `Type` and `Depends`.
3. Add the module name to `ModuleNames` in `Source/meson.build` — **after** every module it depends on, because `GlobalModules` is populated in registration order and `Depends` resolves only against already-registered modules.
4. Keep the dependency graph a DAG; do not add a dependency to dodge an abstraction problem (`rules/architecture.md`).

## Adding sources

- The builder discovers `Private/**/*.cpp|*.c` with a configure-time `find`. After adding, renaming, or removing a source file, run `meson setup --reconfigure <builddir>`; a plain `ninja -C <builddir>` will not see the change.

## Adding dependencies

- **Nix side (always):** add the package to `Nix/deps.nix` under `nativeBuildInputs` or `buildInputs`; `Nix/shell.nix` and `Nix/package.nix` both consume it.
- **Meson side:** `Depends` accepts only other Lumen modules. There is currently no supported way to pass an external `dependency()` to module targets — that requires extending `Source/Meson/ModuleBuilder/meson.build`, which is a deliberate build-system change, not a workaround.
- Do not vendor into `ThirdParty/` without explicit justification.

## Compiler flags and options

- Warnings come from `ALL_WARNINGS` applied by the ModuleBuilder; do not add `-Wno-*` silently (`rules/build.md`).
- For sanitizers and LTO use `-Db_sanitize=address[,undefined]`, `-Db_sanitize=thread`, `-Db_lto=true` — not the inert `enable_*` options.

## Validation

1. Reconfigure: `meson setup --reconfigure <builddir>` (existing build dirs: `build-debug`, `build-release`).
2. Build: `ninja -C <builddir>`.
3. Confirm no warnings under `-Werror`.
4. If the target graph changed, verify with `meson introspect <builddir> --targets` or the configure output.
5. Report results with `rules/validation.md` statuses — only claim what you ran.
