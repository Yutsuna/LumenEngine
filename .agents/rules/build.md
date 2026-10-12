# Rule: Build system

**Applies to:** any `meson.build` / `meson.options` file, `Nix/` or `flake.nix` file, compiler/linker flag, or dependency addition.

## Authority split

- **Meson** is the build-system authority: targets, flags, options, module wiring.
- **Nix** is the reproducible dependency/environment authority: which tools and libraries exist, and at which versions.
- Do not introduce CMake, Bazel, Premake, or alternative package managers. Do not add ad-hoc system dependencies when the dependency is expected to be reproducible through Nix: project dependencies go through `Nix/deps.nix`. Host tools installed outside Nix may be used for development convenience (editors, `git`), but nothing the build requires may depend on them.
- The existing Meson infrastructure (`meson.build`, `Source/meson.build`, `Source/Meson/`, module `meson.build` files) is authoritative. Do not replace the custom module builder with hand-duplicated conventional Meson configuration.

## Module integration and registration order

- Each `Source/<Module>/meson.build` defines a `ModuleDefinition` dict (`Name`, `Type`, `SourceDir`, `Private`, `Public`, `Depends`).
- `Source/meson.build` iterates `ModuleNames`, collects the definitions, then `Source/Meson/ModuleBuilder/meson.build` creates the targets and registers each built module in `GlobalModules`.
- **Ordering matters:** `Depends` entries are resolved against `GlobalModules` as the builder iterates in registration order. A module may only depend on modules listed **before** it in `ModuleNames` (currently `Core`, then `AppCore`, then `Launch`). A dependency listed later fails configuration with "depends on ... which is not built yet or not found."
- `Depends` resolves **only other Lumen modules**. There is currently no supported path for adding an external `dependency()` through `ModuleDefinition`; supporting one requires extending `Source/Meson/ModuleBuilder/meson.build` itself (treat that as an architecture change, not a drive-by edit).
- **Known repository state:** `Nix/package.nix` declares `pname = "TrafficEngine"`; the Meson project and all code use `Lumen`. This is documented state; do not silently rewrite it as part of unrelated changes.
- Shared modules receive a generated `<Module>Export.hpp` from `Meson/ExportTemplate.hpp.inl`, plus a `-D<MODULE>_BUILD_SHARED` define. Anything invoked across shared-module boundaries must be declared with `<MODULE>_EXPORT`, because modules build with `-fvisibility=hidden` and `-fvisibility-inlines-hidden`.

## Source discovery

- The builder collects `Private/` sources with a configure-time `find` for `*.cpp` and `*.c`. Adding, renaming, or removing a source file therefore requires **reconfiguring Meson** (`meson setup --reconfigure <builddir>`); a plain `ninja -C <builddir>` will not pick the change up.

## Dependencies (Nix)

- Add project dependencies in `Nix/deps.nix` (`nativeBuildInputs` / `buildInputs`); both `Nix/shell.nix` and `Nix/package.nix` consume it.
- Flake outputs are `packages.default` and `devShells.default`: use `nix develop` for the shell and `nix build` (or `nix build .#default`) for the package. There is no `.#lumen` output.
- Do not vendor dependencies under `ThirdParty/` without explicit justification and documentation.

## Compiler flags

- `Source/Meson/meson.build` defines `ALL_WARNINGS` (including `-Werror`), applied to every module target as compile arguments. `warning_level=0` in the root `meson.build` does not disable these warnings.
- Do not silently disable warnings. If a warning must be suppressed, document the reason in the code and in the commit description.

## Declared but inert options — do not rely on them

`meson.options` declares `enable_testing`, `enable_asan`, `enable_ubsan`, `enable_tsan`, and `enable_lto`, but **no `meson.build` file currently reads them** (`get_option()` is never called), so passing them changes nothing. Until they are wired up:

- Sanitizers: use Meson's built-in `-Db_sanitize=address`, `-Db_sanitize=address,undefined`, `-Db_sanitize=thread`.
- LTO: use `-Db_lto=true`.
- Do not report "`-Denable_asan=true` was used" as sanitizer validation — it is a no-op.

Changing `meson.options` or the builder to wire these options is itself a build-system task, not a side effect of enabling a sanitizer.
