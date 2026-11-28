# Module: Renderer

- **Type:** shared library (`Source/Renderer/meson.build`), public `Depends` on
  `Core`, `Maths`, `Scene`; private `Depends` on `RHI`, `RHIOpenGL` via the
  `PrivateDepends` Meson key.
- **Status:** built. Listed after `RHIOpenGL` in `ModuleNames`
  (`Source/meson.build`) so the private backend dependency resolves.
- **Role:** backend-neutral consumer of immutable Scene snapshots and producer
  of portable submissions on one dedicated render thread. `Renderer/Public`
  contains no RHI type; RHI, backend selection, adapters, swapchain and the
  render graph implementation live in `Renderer/Private`.

## Boundary contract

- `Public/` exposes neutral values (`FRenderExtent3D`, `ERenderFormat`,
  `ERenderShaderFormat`, `ERenderResourceState`, …), opaque resource refs
  (`FRenderTextureRef`, `FRenderPipelineRef`, …), `IRenderFeatureHost`,
  `IRenderFeature`, `IRenderFrameBuilder`, `IRenderCompiledFrame`,
  `IRenderPassEncoder`, `FRenderSurfaceDesc` and the `FRenderer` facade.
- `FRendererDesc` has **no** device, adapter or swapchain; it accepts a neutral
  surface description and an opaque backend preference string interpreted only
  in `Private/Backend/RenderBackendSelection.cpp`.
- `FRenderer` is a PIMPL: `TUniquePtr<FRendererState>` holds every RHI object.
  `HandleRenderCommand` forwards to the private state on the render thread.
- `Frame/RenderAssetTypes.hpp` (public) keeps only `ERenderAssetError` and
  `FRenderGeometryAssetDesc`; `FRenderGeometryAsset` and
  `Frame/RenderAssetRegistry.hpp` are private.
- `RenderGraph/*` and `Execution/RenderFrameExtension.hpp` are private. Clients
  implement `IRenderFeature` instead.
- Barrier state (Committed/Pending) is owned and committed in
  `Private/Backend/RenderFeatureHost.cpp`; features declare intent only and
  never carry transition arrays.
- `IRenderFeatureHost::GetCapabilities().bSoftwareAdapter` is a neutral `Bool`
  used by Lumina to halve its resolution on software adapters.

## Tests and benchmarks

- The Null backend stays in RHI `Tests`; `RHINullTestSupportSources`,
  conformance/private include dirs and `BenchmarkTestDependency` are preserved
  in `Source/Renderer/meson.build`.
- Tests inject a device through the internal hook in
  `Private/Backend/StartWithTestDevice.hpp` (`StartWithTestDevice`), which also
  accepts an optional swapchain for presentation tests. No device type appears
  in `Public/`.
- `RHINullTestSupportSources` are linked only into tests/benchmarks; production
  Renderer code includes only `RHI/Public` from `Renderer/Private`.

## Notes for agents

- `-Werror` plus `-Wconversion`/`-Wsign-conversion`: map enums with explicit
  switches, never cross-family `static_cast`.
- Windowed OpenGL presentation is not available in headless CI (EGL
  surfaceless); use the headless/readback path for validation.
