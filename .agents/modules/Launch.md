# Module: Launch

- **Type:** Executable (`Source/Launch/meson.build`), depends on `Core`,
  `AppCore`, `Scene`, `RHI`, `Renderer`. Produces the `Launch` binary
  (`Build/Source/Meson/Private/Launch`, where the `executable()` is declared).
- **Status:** built. Entry point of the engine.

## Public surface (`Source/Launch/Public/`)

- `Engine/` — `LaunchEngine`, `LaunchEngineLoop`, `LaunchEngineConfig(+Parser)`,
  `LaunchEngineModule`, `LaunchFrameTiming`, `LaunchProcessGuard`, and
  `Engine/Modules/` — engine module registration.
- `Launch/` — `LaunchModuleStack.hpp` (module boot order), `LaunchError.hpp`.
- `CLI/` — command-line interface handling.

Private: `Main.cpp`, `LaunchModuleStack.cpp`, engine implementation.

## Notes for agents

- Module boot order lives in `LaunchModuleStack` / `Engine/Modules` — when adding
  a new engine module to the stack, register it there; adding the module to
  `ModuleNames` only compiles it, it does not boot it. The runtime table still
  registers only `Application` and `Window`; Scene/RHI/Renderer engine-module
  registration and backend selection remain a later composition step.
- `LaunchError` is the process-level error wrapper; `ToExitCode()` maps each
  `ELaunchErrorCode` to its stable numeric value (1–6), used as the process
  exit code. **84** (`LUMEN_EXIT_FAILURE`) applies to test/benchmark runners
  only; in `GuardedMain` it is the `ValueOr` fallback.
- `Engine/Modules/LaunchSurfaceBridge.hpp` converts AppCore's opaque
  `FNativeWindowHandles` into the OpenGL backend's scalar native-window
  descriptor and calls the public surface-bridge factory. Launch is the only
  module allowed to perform that conversion; the factory accepts X11, Wayland
  and Win32 native handles (Win32 needs no `NativeDisplay`). Errors are
  propagated unchanged, never replaced by a mock.
- Keep `Main.cpp` minimal; all engine behaviour belongs in the loop/engine
  classes so it stays testable.
