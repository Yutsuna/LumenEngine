# Module: Launch

- **Type:** Executable (`Source/Launch/meson.build`), depends on `Core`,
  `AppCore`. Produces the `Launch` binary (`Build/Source/Launch/Launch`).
- **Status:** built. Entry point of the engine.

## Public surface (`Source/Launch/Public/`)

- `Engine/` — `LaunchEngine`, `LaunchEngineLoop`, `LaunchEngineConfig(+Parser)`,
  `LaunchEngineModule`, `LaunchFrameTiming`, `LaunchProcessGuard`, and
  `Engine/Modules/` — engine module registration.
- `Launch/` — `ModuleStack.hpp` (module boot order), `LaunchError.hpp`.
- `CLI/` — command-line interface handling.

Private: `Main.cpp`, `ModuleStack.cpp`, engine implementation.

## Notes for agents

- Module boot order lives in `ModuleStack` / `Engine/Modules` — when adding
  a new engine module to the stack, register it there; adding the module to
  `ModuleNames` only compiles it, it does not boot it.
- `LaunchError` is the process-level error wrapper; `ToExitCode()` maps each
  `ELaunchErrorCode` to its stable numeric value (1–6), used as the process
  exit code. **84** (`LUMEN_EXIT_FAILURE`) applies to test/benchmark runners
  only; in `GuardedMain` it is the `ValueOr` fallback.
- Keep `Main.cpp` minimal; all engine behaviour belongs in the loop/engine
  classes so it stays testable.
