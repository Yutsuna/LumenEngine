---
description: Scaffold a new Source module, wire it into the build, and prepare its test/benchmark layout
---

# new-module

1. Scaffold the skeleton:

   ```sh
   python3 Scripts/NewModule.py <Name>       # creates Source/<Name>/{Public,Private}/ + meson.build
   ```

2. **Register it in the build** — `Scripts/NewModule.py` does not do this:
   add the name to `ModuleNames` in `Source/meson.build`, **in dependency
   order** (a module can only depend on modules earlier in the list, see
   `modules/Launch.md` note about boot order vs compile order).
3. Fill the generated `ModuleDefinition` in `Source/<Name>/meson.build`:
   set `Type` (`SharedLibrary` by default) and `Depends`; add `'ExternalDeps'`
   for pkg-config libraries (see `Source/AppCore/meson.build` SDL3 example).
4. Create `Tests/` (auto-discovered) and/or `Benchmarks/` once there is code;
   suites register themselves via `LUMEN_TEST` / `LUMEN_BENCHMARK`
   (`rules/testing.md`).
5. If the module is a shared library: mark public classes with
   `<NAME>_EXPORT` (upper-cased module name) and include the generated
   `<Name>Export.hpp` in public headers (`Meson/ExportTemplate.hpp.inl`).
6. If the module must boot at runtime (engine module), also register it in
   `Launch`'s module stack / `Source/Launch/Public/Engine/Modules/` —
   compiling it is not booting it.
7. Reconfigure: `meson setup Build --reconfigure`. New module names must not
   collide with tags: the PR scope will pick up `Source/<Name>` automatically
   (`workflows/git-workflow.md`).
