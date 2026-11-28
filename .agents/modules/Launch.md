# Module: Launch

- **Type:** shared library (`Source/Launch/meson.build`, with `TestsExcludeFiles`
  for `Main.cpp`), depends on `Core`, `AppCore`, `Scene`, `Renderer` — **no RHI
  and no backend**. Produces the `Launch` binary.
- **Status:** built. Entry point of the engine.

## Public surface (`Source/Launch/Public/`)

- `Engine/` — `LaunchEngine`, `LaunchEngineLoop`, `LaunchEngineConfig(+Parser)`,
  `LaunchEngineModule`, `LaunchFrameTiming`, `LaunchProcessGuard`, and
  `Engine/Modules/` — engine module registration, including
  `LaunchRendererModule.hpp` (`FLaunchRendererModule`, `FScopedRenderSession`,
  `FRenderSessionDesc`).
- `Launch/` — `LaunchModuleStack.hpp` (module boot order), `LaunchError.hpp`.
- `CLI/` — command-line interface handling.

Private: `Main.cpp`, `LaunchModuleStack.cpp`, engine implementation.

## Notes for agents

- Module boot order lives in `LaunchModuleStack` / `Engine/Modules` — modules
  self-register with `LUMEN_REGISTER_MODULE`. `FLaunchRendererModule` owns the
  renderer and installs its resize handler for the module's whole lifetime;
  clients open a `FScopedRenderSession` to start/stop rendering.
- `Launch` must never include `<RHI/...>` or `<RHIOpenGL/...>` and must not
  declare them in `Depends`: device, backend selection, adapters, swapchain and
  presentation are private to `Renderer/Private`.
- `Launch/Private/Engine/Modules/LaunchRendererModule.cpp` maps AppCore's
  `FNativeWindowHandles` into the neutral `FRenderSurfaceDesc` (opaque scalars
  plus a window-system identifier string). `LaunchSurfaceBridge.hpp/.cpp` and
  their tests were removed: that conversion is now private to Renderer.
- The renderer resize handler is pushed in `FLaunchRendererModule::Create()`
  and popped in its destructor — never from `~FScopedRenderSession()` — so the
  message-handler stack stays strictly LIFO with client handlers.
- `-renderer=<name>` is read from the command line and passed to Renderer as an
  opaque string; no enum naming a backend belongs in `Source/Launch/`.
- No application title (e.g. "Lumina — Path Tracer") is coded in Launch; titles
  stay in the client.
- `LaunchError` is the process-level error wrapper; `ToExitCode()` maps each
  `ELaunchErrorCode` to its stable numeric value (1–6), used as the process
  exit code. **84** (`LUMEN_EXIT_FAILURE`) applies to test/benchmark runners
  only; in `GuardedMain` it is the `ValueOr` fallback.
- Keep `Main.cpp` minimal; all engine behaviour belongs in the loop/engine
  classes so it stays testable.
