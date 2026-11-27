---
name: Meson Build Customization
description: How to customise Lumen's declarative Meson build — every ModuleDefinition key, the three target topologies, generated export headers, find() discovery, tests/benchmarks wiring, options and platform conditionals
---

# Meson build customization

Companion to `rules/build.md` (commands, build trees, toolchain, warning
policy), `commands/new-module.md` (scaffolding sequence), `skills/nix-workflow.md`
(where dependencies are declared) and `rules/testing.md` /
`skills/write-benchmarks.md` (the test/benchmark macros). **This page owns only
the declarative Meson mechanics**: how a `meson.build` is structured, which
keys exist, and which target each declaration produces. Do not duplicate
command recipes or toolchain facts here.

**Path convention.** Agent-doc paths under `rules/`, `commands/`, `skills/`,
`modules/`, `agents/` and `workflows/` are relative to `.agents/`
(`AGENTS.md:10-11`); repository paths under `Source/`, `Meson/`, `Nix/`,
`Scripts/` and `Build/` (plus root files) are relative to the repository root.
In tables, file-stem pointers are short
for the obvious file: `Core:7` = `Source/Core/meson.build:7`, `Private:11` =
`Source/Meson/Private/meson.build:11`, `Tests:60` =
`Source/Meson/Tests/meson.build:60`, and so on. Line numbers are evidence:
open the file before editing it.

---

## 1. Build graph at a glance

```
meson.build                 project(), default_options, -freflection
  └─ subdir('Source/')
       ├─ subdir('Meson/')          helpers: ALL_WARNINGS, FileSystem, OSName,
       │                            SourceRoot, GlobalModules, GlobalTestModules,
       │                            PublicIncludes, ModuleDefinitions
       ├─ foreach ModuleName : ModuleNames
       │     if FileSystem.is_file(ModuleName / 'meson.build')
       │       subdir(ModuleName)   module declares ModuleDefinition
       │     endif
       └─ subdir('Meson/Public')    pass 1: include dirs
          subdir('Meson/Private')   pass 2: targets + dependencies
          subdir('Meson/Tests')     pass 3: test() targets
          subdir('Meson/Benchmarks') pass 4: benchmark() targets
```

`subdir` order is load-bearing: the helper globals must exist before a module
file is read, and target passes must run after every `ModuleDefinition` has
been collected (`Source/meson.build:6,30-40`).

Stage files use a `#{ … #}` header with `Reads :` / `Writes :` lines, e.g.
`Source/Meson/Private/meson.build:1-7`. Keep it accurate when editing. Two
current headers are slightly off — trust the code, not the header:

| Stage | Header | Reality |
| --- | --- | --- |
| `Source/Meson/Tests/meson.build:5` | lists `GlobalModules` as read | only `GlobalTestModules` is read (`:60`) |
| `Source/Meson/Benchmarks/meson.build:4` | omits `GlobalTestModules` | it is read at `:61-62` when `BenchmarkTestDependency` is true |

## 2. The `ModuleDefinition` contract

Every `Source/<Module>/meson.build` must end by defining a dictionary named
exactly `ModuleDefinition`; `Source/meson.build:33` copies it and injects
`SourceDirRel`. A module that defines no such variable breaks the loop.

| Key | Read by | Effect | Default | Real example |
| --- | --- | --- | --- | --- |
| `Name` | `Public:10`, `Private:10`, `Tests:14`, `Benchmarks:13` | Key in `PublicIncludes` / `GlobalModules` / `GlobalTestModules`; target name `<Name>`, `<Name>Tests`, `<Name>Benchmarks`; suite name; export macro prefix | **mandatory** (no `.get()`) | `Core:7` `'Core'` |
| `Type` | `Private:11` | Chooses one of three topologies (§3); unknown value → `error()` at `Private:177-178` | machinery: `'StaticLibrary'`; **scaffold: `'SharedLibrary'`** (`Scripts/NewModule.py:24`) | `Launch:8` `'Executable'` |
| `SourceDir` | **never read** | Inert — every module still declares it, but no `.get('SourceDir')` exists | n/a | `Core:9` |
| `SourceDirRel` | injected at `Source/meson.build:33`; read `Public:11`, `Private:12`, `Tests:15`, `Benchmarks:14` | Relative module dir for `'../../@0@/@1@'.format(...)` includes and `SourceRoot / <dir>` paths | injected automatically | `Source/meson.build:33` |
| `Private` | `Private:16,21`; `Tests:18,46`; `Benchmarks:18,46` | Implementation dir: `find()` sources + private include dir | `'Private'` | `Core:10` |
| `Public` | `Public:12` only | Public include dir exposed through `PublicIncludes` | `'Public'` | `Core:11` |
| `Tests` | `Tests:19,47` | Test source dir + custom-main detection | `'Tests'` | `RHI:53` |
| `Benchmarks` | `Benchmarks:19,47` | Benchmark source dir + custom-main detection | `'Benchmarks'` | `Scene:13` |
| `Depends` | `Private:13,45-53`; `Benchmarks:15,68` | Internal module deps resolved from `GlobalModules`; missing dep → `error()` at `Private:47-51`; also guards benchmark Core auto-link | `[]` | `RHI:54` `['Core','Maths']` |
| `ExternalDeps` | `Private:55` | Appended to `ResolvedDependencies` (pkg-config / `dependency()` objects) | `[]` | `AppCore:9,20` (`sdl3`, `required : true`); `RHIOpenGL:10-15,25`; `RHIVulkan:7-11,21` |
| `ExtraTestSources` | `Tests:36` | Adds `files()` objects to discovered test sources | `[]` | `RHIOpenGL:27-43`; `Renderer:15` |
| `ExtraTestIncludeDirectories` | `Tests:58` | Adds `include_directories()` objects to the test target | `[]` | `RHIOpenGL:45`; `Renderer:16` |
| `ExtraTestDependencies` | `Tests:61` | Appends deps after the module's test dep | `[]` | none in-tree — extension point |
| `TestEnvironment` | `Tests:62` → `env:` at `:81` | `KEY=VALUE` strings for `test()` | `[]` | `RHIVulkan:22-24`, `RHIOpenGL:46-48` (`LSAN_OPTIONS=suppressions=` + `meson.current_source_dir() / 'lsan.supp'`) |
| `ExtraBenchmarkSources` | `Benchmarks:36` | Adds sources to discovered benchmark sources | `[]` | `Renderer:18` |
| `ExtraBenchmarkIncludeDirectories` | `Benchmarks:58` | Adds include dirs to the benchmark target | `[]` | `Renderer:19` |
| `ExtraBenchmarkDependencies` | `Benchmarks:65` | Appends deps after the chosen module/test dep | `[]` | none in-tree — extension point |
| `BenchmarkTestDependency` | `Benchmarks:60` | `true` → link `GlobalTestModules` (implementation objects) instead of `GlobalModules` (shared lib) | `false` | `Renderer:20` `true` |

Globals the machinery supplies, not definition keys:

| Symbol | Defined at | Used by |
| --- | --- | --- |
| `FileSystem = import('fs')` | `Meson:37` | `Source:31`, `Public:18`, `Private:24,30`, `Tests:23,39-40,55`, `Benchmarks:23,39-40,55` |
| `OSName = host_machine.system()` | `Meson:38` | `AppCore:8` |
| `SourceRoot = meson.project_source_root() / 'Source'` | `Meson:44` | `Private:15`, `Tests:17`, `Benchmarks:17` |
| `ALL_WARNINGS` | `Meson:6-29`, `33-35` | `Private:57,87-88,97-98,111-112,140-141,167-168`; `Tests:64,71-72`; `Benchmarks:72,79-80` |
| `ModuleDefinitions` | `Meson:46`, filled `Source:28,33` | all four stage files |
| `PublicIncludes` | `Meson:42`, filled `Public:22` | `Private:18`, `Tests:52`, `Benchmarks:52` |
| `GlobalModules` | `Meson:40`, filled `Private:185` | `Private:46,52`; `Benchmarks:63,67,69` |
| `GlobalTestModules` | `Meson:41`, filled `Private:186` | `Tests:60`; `Benchmarks:61-62` |

`ALL_WARNINGS` is the single source of warning flags (`-Werror`,
`-Wconversion`, `-Wsign-conversion`, `-Wold-style-cast`, `-Wshadow`,
`-Wdouble-promotion`, `-fvisibility=hidden`, …) and `-Og` is appended when
`get_option('optimization') == '0'` (`Meson:31-35`). Any target built outside
the module system receives none of it (`rules/build.md:87-90`).

## 3. Registration contract

1. `ModuleNames` is an ordered list: `Source/meson.build:8-12` (`Core`, `Maths`,
   `RHI`), a Linux-only append at `:14-16` (`RHIOpenGL`), then `:18-26`
   (`Scene`, `Renderer`, `AppCore`, `Launch`, `RHIVulkan`).
2. A module is compiled **only if** its name is in `ModuleNames` **and**
   `Source/<Name>/meson.build` exists — the `FileSystem.is_file` gate at
   `Source/meson.build:31`.
3. Dependency order *is* `ModuleNames` order: `GlobalModules` is filled only
   after a module's targets exist (`Private:185`), and `Depends` resolution
   fails with `error()` if a dependency has not been built yet
   (`Private:45-51`). Do not move a module earlier to "make it build".
4. `ModuleDefinitions += [ModuleDefinition + { 'SourceDirRel' : ModuleName }]`
   (`Source:33`) merges a copy — never mutate the original dictionary.
5. `find()` runs at **configure time** (`Private:31-35`, `Tests:24-28`,
   `Benchmarks:24-28`): after adding, deleting or renaming any `.cpp`/`.c`,
   reconfigure (prefer `Scripts/BuildMeson.bash` on an already configured
   tree; drop `--reconfigure` for a fresh tree, see `rules/build.md:53-55`).
6. Compile ≠ boot: being in `ModuleNames` does not register the runtime module
   stack (`modules/Launch.md:20-22`; `commands/new-module.md:29-31`).
7. New module names must not collide with PR scope tags (`commands/new-module.md:32-34`).

## 4. The three legal `Type` values

All target construction is in `Source/Meson/Private/meson.build`.

### `Executable` (`Private:59-119`)

- With `Private/Main.cpp`: `Main.cpp` goes into the executable; every other
  discovered source goes into a `static_library(<Name>Lib, …)` linked with
  `link_with :` (`:60-100`). Without `Main.cpp`: all sources go straight into
  one `executable()` (`:107-114`).
- Shared kwargs: `include_directories`, `dependencies`, `cpp_args`, `c_args`
  (`ALL_WARNINGS`); `install : true` on the executable, `false` on the helper
  library.
- `ModuleDependency` links the helper library (or just includes+deps when
  there is none); `ModuleTestDependency = ModuleDependency` (`:181-183`).
- Example: `Launch:8` — `executable()` is declared in
  `Source/Meson/Private/meson.build:92-100`, so Meson mirrors that source
  directory: the binary is `Build/Source/Meson/Private/Launch`, next to
  `libLaunchLib.a`.

### `SharedLibrary` (`Private:121-160`)

- `configuration_data()` sets `MODULE` to `ModuleName.to_upper()` (`:122-123`);
  `configure_file()` renders `Meson/ExportTemplate.hpp.inl` into
  `<Module>Export.hpp` (`:125-129`). The generated header lands in the build
  tree (`Build/Source/Meson/Private/`) and is exposed via
  `include_directories('.')` added to **both** public and private include lists
  (`:131-133`). Public classes must include it and use `<NAME>_EXPORT`.
- `BuildDefines = ['-D' + ModuleName.to_upper() + '_BUILD_SHARED']` (`:134`).
- `static_library(<Name>Implementation, PrivateSources, …, pic : true,
  install : false)` (`:136-144`) carries the sources, include dirs,
  dependencies and `ALL_WARNINGS + BuildDefines`; `shared_library(<Name>,
  link_whole : ModuleImplementation, dependencies :, install : true)` (`:145-150`)
  is only a wrapper — putting args on the wrapper instead is a no-op.
- `ModuleDependency` links the shared library; `ModuleTestDependency` uses
  `link_whole : ModuleImplementation` so tests reuse the same objects without
  exporting new symbols (`:151-160`; `rules/testing.md:34-38`).
- `ExportTemplate.hpp.inl:10-11` also has a `_USE_SHARED` / dllimport branch,
  but no code in this repo defines `_USE_SHARED`.
- Example: `Core:8`.

### `StaticLibrary` (`Private:162-175`)

- Single `static_library(<Name>, PrivateSources, …, install : true)`, no
  `pic:` override, no generated export header, no `_BUILD_SHARED` define.
- `ModuleTestDependency = ModuleDependency` (`:181-183`). No in-tree module
  selects this explicitly — it is the machinery's `.get()` fallback, so name
  `Type` explicitly if you want it.

Any other string triggers `error('Unknown module type: …')` (`Private:177-178`).

## 5. Declarative builtins inventory

### Project, options, control flow

| Construct | Evidence | Notes |
| --- | --- | --- |
| `project()` | `meson.build:6-19` | `'Lumen'`, `'cpp'`, `'c'`; `version : run_command('cat','VERSION.md',check:true).stdout().strip()`; `default_options`; `meson_version : '>=1.10'` |
| `add_project_arguments('-freflection', language : 'cpp')` | `meson.build:21` | project-wide; tooling strips it (`.clangd`) |
| `option()` | `meson.options:6-8` | `enable_testing`, `enable_benchmarking`, `enable_lto` |
| `get_option()` | `Meson:33` (`optimization`), `Tests:9`, `Benchmarks:8` | only three options are consumed |
| `subdir()` / `subdir_done()` | `meson.build:23`; `Source:6,32,37-40`; `Tests:10`, `Benchmarks:9` | `subdir_done()` is the option-off early exit |
| `foreach` / `endforeach` | `Source:30-35`; `Public:9-23`; `Private:9-187` (+ inner `:36,45,65,69`); `Tests:13-84`; `Benchmarks:12-92`; `RHIOpenGL:10-15` | — |
| `if` / `elif` / `else` / `endif` | `Source:14-16,31-34`; `Meson:33-35`; `Private:24-26,30-41,46-51,59,77,82,107,121,162,177,181`; `Tests:9-11,23-34,38-44,55-57`; `Benchmarks:8-10,23-34,38-44,55-57,60-64,67-70`; `AppCore:8-10`; `RHIVulkan:9-11` | `elif` only in the type dispatch |
| `error()` | `Private:47-51` (bad `Depends`), `Private:178` (bad `Type`) | fail at configure time, not later |
| `host_machine.system()` | `Source:14`, cached as `OSName` at `Meson:38` | Linux-only module/dependency wiring |

### Values and operators

| Construct | Evidence | Notes |
| --- | --- | --- |
| dict literals | every module file, e.g. `Core:6-14` | the `ModuleDefinition` variable |
| dict `+` | `Source:33` | `ModuleDefinition + { 'SourceDirRel' : ModuleName }` |
| `.get(key, default)` | `Public:10-12`; `Private:10-13,16,21`; `Tests:14-19,46-47,58,60-62`; `Benchmarks:13-19,46-47,58,60,65` | always supply a default for optional keys |
| `.has_key()` | `Private:46`, `Benchmarks:67` | dependency validation / Core auto-link guard |
| `.contains()` | `Benchmarks:68` | `not ModuleDependencies.contains('Core')` |
| `.length()` | `Private:77,82`; `Tests:38`; `Benchmarks:38` | gates target creation |
| list/scalar `+=` | `Source:15,18,33`; `Meson:34`; `Private:52,55`; `Tests:31,36,43,53,56,58,61`; `Benchmarks:31,36,43,53,56,58,65,69`; `RHIOpenGL:13`; `RHIVulkan:10`; `AppCore:9` | scalar appends one element |
| `/` string path join | `Source:31`; `Meson:44`; `Private:15-16,22,60`; `Tests:17-19,43,49-50`; `Benchmarks:17-19,43,49-50`; `RHIVulkan:23`; `RHIOpenGL:47` | works on relative and absolute strings |
| `.format()` | `Public:14`; `Private:48-49,84,93,127,178`; `Tests:67,77`; `Benchmarks:49-50,75,85` | `@0@` placeholders |
| `.to_upper()` | `Private:123,134` | export macro prefix |
| `.strip()` | `meson.build:10`; `Private:36`; `Tests:29`; `Benchmarks:29` | cleans `run_command().stdout()` |
| `.split('\n')` | `Private:36`; `Tests:29`; `Benchmarks:29` | turns `find` output into a list |
| backslash continuation | `Tests:39-40`; `Benchmarks:39-40,67-68` | put `or` / `and` at the start of the continued line |
| `#{ … #}` header comment | `meson.build:1-4`; `Private:1-7`; `Tests:1-7`; `Benchmarks:1-6` | stage files add `Reads:` / `Writes:` |

### Filesystem, discovery, targets

| Construct | Evidence | Notes |
| --- | --- | --- |
| `import('fs')`, `is_file()`, `is_dir()` | `Meson:37`; `Source:31`; `Public:18`; `Private:24,30`; `Tests:23,39-40,55`; `Benchmarks:23,39-40,55` | guards for optional dirs/files |
| `run_command('find', path, '-type','f','(','-name','*.cpp','-o','-name','*.c',')', check : false)` | `Private:31-35`; `Tests:24-28`; `Benchmarks:24-28` | configure-time discovery; `check : false` is deliberate |
| `files()` | `Private:38,64`; `Tests:31,43`; `Benchmarks:31,43`; `RHI:6-22,26-45` | wraps discovered paths and extra sources |
| `include_directories()` | `Public:19`; `Private:19,25,131`; `Tests:53,56`; `Benchmarks:53,56`; `RHI:23-24` | relative `'../../…'` and absolute paths |
| `dependency(name, required : true/false)` + `.found()` | `AppCore:9` (`sdl3`, required); `RHIVulkan:7-11`; `RHIOpenGL:10-15` | opportunistic pattern: guard `.found()` before appending; declare the package in `Nix/deps.nix` too (`skills/nix-workflow.md`) |
| `declare_dependency()` | `Private:101,115,151,156,171` | kwargs: `include_directories`, `link_with` or `link_whole`, `dependencies` |
| `static_library()` | `Private:83-90,136-144,163-170` | `include_directories`, `dependencies`, `cpp_args`, `c_args`, `pic`, `install` |
| `shared_library()` | `Private:145-150` | `link_whole`, `dependencies`, `install` |
| `executable()` | `Private:92-100,107-114`; `Tests:66-74`; `Benchmarks:74-82` | `link_with`, `install`, `build_by_default : false` for tests/benchmarks |
| `configuration_data()` + `.set()` | `Private:122-123` | `MODULE` → uppercase name |
| `configure_file()` | `Private:125-129` | `input : Meson/ExportTemplate.hpp.inl`, `output : '@0@Export.hpp'.format(...)` |
| `test()` | `Tests:76-82` | `workdir : meson.project_source_root()`, `suite : ModuleName`, `env : TestEnvironment`; default 30 s timeout |
| `benchmark()` | `Benchmarks:84-90` | same kwargs plus `timeout : 0` (`:89`) — long benchmarks otherwise kill whole suites |
| `meson.current_source_dir()` | module files at line 9; `RHIVulkan:23`; `RHIOpenGL:47` | module paths and LSAN suppression paths |
| `meson.project_source_root()` | `Meson:44`; `Private:126`; `Tests:43,79`; `Benchmarks:43,87` | shared tools/headers and test workdir |

## 6. Options and conditionals

| Mechanism | Evidence | Effect |
| --- | --- | --- |
| `get_option('optimization') == '0'` | `Meson:31-35` | appends `-Og` to `ALL_WARNINGS` so glibc `_FORTIFY_SOURCE` stays quiet |
| `host_machine.system() == 'linux'` | `Source:14-16` | appends `RHIOpenGL` to `ModuleNames` |
| `OSName == 'linux'` | `AppCore:6-10` | adds `dependency('sdl3', required : true)` |
| `dependency(..., required : false)` + `.found()` | `RHIOpenGL:9-15`; `RHIVulkan:6-11` | optional backends; flags only when pkg-config finds them |
| `get_option('enable_testing')` | `Tests:9-11` | `subdir_done()` when false |
| `get_option('enable_benchmarking')` | `Benchmarks:8-10` | `subdir_done()` when false; `Scripts/BuildMeson.bash:11,17` enables it for release only |
| `enable_lto` | declared `meson.options:8` | **no consumer**: no `get_option('enable_lto')` and no `b_lto` anywhere. Do not promise `-Denable_lto` changes linking |
| project defaults | `meson.build:11-17` | `cpp_std=c++26`, `c_std=c23`, `default_library=shared`, `warning_level=0`, `b_ndebug=if-release` |

## 7. Tests and benchmarks wiring

- Sources under `Tests/` / `Benchmarks/` are found by `find()` at configure
  time; explicit extras come from `Extra*Sources` keys.
- A default `main` is appended from `Meson/TestRunner.cpp` /
  `Meson/BenchRunner.cpp` unless `Tests/Main.cpp`, `Tests/TestMain.cpp`,
  `Benchmarks/Main.cpp` or `Benchmarks/BenchmarkMain.cpp` exists
  (`Tests:39-44`; `Benchmarks:39-44`).
- Include dirs: module `Public/`, the `Tests/` or `Benchmarks/` dir, `Private/`
  if present, plus `Extra*IncludeDirectories`.
- Dependencies: `GlobalTestModules` (implementation objects for shared
  modules) or `GlobalModules`, plus `Extra*Dependencies`. Core is auto-linked
  into benchmarks only when `ModuleName != 'Core'`, `Core` exists and is not
  already in `Depends` (`Benchmarks:67-70`); tests must list `Core` themselves.
- `BenchmarkTestDependency : true` makes a benchmark link the test dependency
  (`Renderer:20`) — needed when it exercises private/static implementation
  objects rather than the exported shared library.
- Failure exit code is **84**, never 1 — tests (`Meson/TestRunner.cpp:32`,
  `rules/testing.md:32-33`) and benchmarks (`Meson/BenchRunner.cpp:39`,
  `AGENTS.md:63`).

## 8. Extension recipes

1. **Add a pkg-config dependency**: declare the input in the right list of
   `Nix/deps.nix` (`skills/nix-workflow.md`), then in the module file:
   `OptionalDeps = []; Dep = dependency('foo', required : false); if Dep.found()
   OptionalDeps += Dep; endif` and set `'ExternalDeps' : OptionalDeps`
   (pattern from `RHIVulkan:6-11,21`). Use `required : true` only when the
   module cannot work without it (`AppCore:9`).
2. **Add another module's test helpers to a suite**: export `files()` lists
   and `include_directories()` objects from that module's `meson.build`
   (`RHI:6-24`), then set `ExtraTestSources` / `ExtraTestIncludeDirectories`
   in the consumer (`RHIOpenGL:27-45`, `Renderer:15-16`). Because module files
   are read in `ModuleNames` order, only reference modules listed earlier.
3. **Add an environment variable to tests**: append `'KEY=VALUE'` to
   `TestEnvironment` (`RHIVulkan:22-24`).
4. **Add a benchmark-only dependency**: use `ExtraBenchmarkDependencies` or
   `ExtraBenchmarkIncludeDirectories` (`Benchmarks:58,65`); do not pollute
   `Depends`, which also feeds the production target.
5. **Change a module's `Type`**: edit only its `meson.build` `Type` key, then
   check consumers of the module's `declare_dependency()` — switching to/from
   `SharedLibrary` changes symbol visibility and export-header generation.
6. **Add a new module**: follow `commands/new-module.md` first; this page
   supplies the full key table for step 3 of that command.

## 9. Pitfalls / anti-patterns

- Never edit generated files under `Build/`, `Build-Release/` or `Build-TSAN/`;
  reconfigure instead (`AGENTS.md`, `rules/build.md:35-51`).
- Never create a target outside the module passes — it loses `ALL_WARNINGS`
  including `-Werror` (`rules/build.md:87-90`).
- Do not put args on the `shared_library()` wrapper; they belong on the
  implementation `static_library()` (`Private:136-150`).
- `'SourceDir'` is inert; rely on the injected `SourceDirRel`.
- `Type` has two different defaults: machinery `StaticLibrary` vs scaffold
  `SharedLibrary` (`NewModule.py:24`). Always set it explicitly.
- Shared-module tests need `ModuleTestDependency` / `link_whole`; benchmarks
  need `BenchmarkTestDependency : true` if they link implementation objects.
- Benchmarks use `timeout : 0`; do not "fix" it to 30 s. Tests keep the default.
- `-Og` at `optimization : 0` and `b_ndebug=if-release` are deliberate — do
  not remove them as "debug leftovers".
- `enable_lto` is declared but unwired; do not treat it as a working knob.
- Missing `Depends` entries and unknown `Type` values are configure-time
  errors, not warnings (`Private:47-51,177-178`).
- Reconfigure after every source add/delete/rename: `find()` is configure-time.

## 10. Verification checklist

- [ ] Module listed in `ModuleNames` in dependency order, with a
      `meson.build` present.
- [ ] Every optional key uses `.get(key, default)` in the consumer; no
      reliance on `SourceDir`.
- [ ] `Depends` names resolve to earlier modules; external packages exist in
      both `Nix/deps.nix` and `dependency()`.
- [ ] `Type` set explicitly; export header and `<NAME>_EXPORT` used for
      `SharedLibrary`.
- [ ] Reconfigure after any `meson.build` or source change — choose the
      reconfigure route per `rules/build.md:35-55`.
- [ ] All three build trees compile (`rules/build.md:15-25`).
- [ ] Relevant suites pass (`rules/testing.md:42-51`); failures report exit 84.
