# Module: Renderer

- **Type:** shared library (`Source/Renderer/meson.build`), depends on `Core`,
  `Maths`, `RHI`, `Scene`.
- **Status:** built. The module is listed after `Scene` in `ModuleNames`
  (`Source/meson.build`) and its tests and benchmarks are compiled.
- **Role:** backend-neutral consumer of immutable Scene snapshots and producer
  of portable RHI submissions on one dedicated render thread. It contains no
  concrete backend, no native handle and no actor or world ownership.

## Public surface (`Source/Renderer/Public/`)

- `Execution/` — `FRenderExecutor`, `FRenderExecutorDesc`,
  `ERenderExecutorError`, `IRenderCommandHandler` (`RenderExecutor.hpp`);
  `FRenderCommand`, `FRenderFrameRequest`, `FRenderFrameStatus`,
  `ERenderCommandType`, `ERenderFrameResult` (`RenderFrameTypes.hpp`);
  `FRenderer`, `FRendererDesc`, `ERendererError` (`Renderer.hpp`).
- `RenderGraph/` — `FRenderGraph`, `FRenderGraphBuilder`,
  `FRenderGraphCompiler` (private), `FCompiledRenderGraph`,
  `FRenderGraphContext`, graph value types and `FRenderError`. Graph
  declaration and compilation create no native or backend object.

## Execution contract

- `FRenderExecutor` owns one `FThread`, one bounded Core `TMPSCQueue` of POD
  commands, and a single-reader status triple buffer. `Enqueue`, `RequestStop`
  and `Join` share one mutex so no producer can push into a queue that is being
  released; acceptance returns `QueueFull` at capacity and `NotRunning` once
  stop is requested. `RequestStop` clears `IsRunning` immediately, drains
  already queued commands, and `Join` publishes the final status including
  rejections. After `Initialize`, neither enqueue nor dispatch allocates.
- `FRenderer` is the sole `IRenderCommandHandler`. It is initialized with a
  portable `IRHIDevice`, a `FRenderSnapshotStream` injected by composition, an
  extent and a command capacity; no backend factory is accepted. It owns the
  queue, allocator, list, clear target, view and timeline semaphore and is the
  only thread that swaps the snapshot read buffers and records or submits
  command lists.
- The vertical slice records a clear pass with dynamic rendering, submits one
  command list with a monotonically increasing completion value, signals a
  timeline semaphore, polls completion with a zero-timeout wait and publishes
  `FRenderFrameStatus`. Snap spans are copied into plain values and never
  retained past the next reader swap. `DeviceLost`/`DeviceClosed` and RHI
  failures are reported through the status without partial presentation.
- `FNonCopyable` is not used as a base for move-only graph types; those types
  delete copy explicitly and default move. `FRenderGraphContext` takes ownership
  of a moved `FCompiledRenderGraph`, so its bindings and barriers cannot outlive
  the compiled metadata; transient binding also checks the materialized RHI
  descriptor against the declared resource descriptor.

## Testing and benchmarks

- `Tests/Functional/Execution/` covers capacity and saturation, drain-before-stop,
  single-consumer identity, stop/join, invalid state and zero enqueue
  allocations. `Tests/Integration/Frame/` runs Scene extraction through a
  snapshot stream into a clear pass on the test-only Null backend with and
  without validation, checks 1000 completed frames, device-loss reporting and
  shutdown, and rejects invalid descriptors before any thread starts.
- The Null backend stays in RHI `Tests`; `RHINullTestSupportSources` and RHI's
  private include directory are wired only into Renderer test and benchmark
  binaries. Production Renderer code includes only `RHI/Public` and
  `Scene/Public/Rendering` headers.
- `Benchmarks/Execution/` measures steady-state enqueue and the full CPU clear
  frame at 16/64/256 extents. No GPU timing is reported.

## Notes for agents

- Snapshot sequences start at one and `WorldId` is nonzero for any published
  snapshot; a zero `WorldId` is the only "not published yet" marker.
- A queue submits its command list and itself transitions the list to
  `Pending`; the renderer must hand over an `Executable` list and recycle the
  allocator only after the previous completion is reached.
- Runtime boot composition belongs to `Launch`; adding this module to
  `ModuleNames` compiles it but does not register an engine module. Backend
  selection is a composition concern and must not be introduced here.
