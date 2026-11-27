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
- `Frame/` — `FRenderProxyRegistry`, `FRenderProxyEntry`, `FRenderProxyStats`,
  `FRenderAssetRegistry`, `FRenderGeometryAsset`, `FRenderGeometryAssetDesc`,
  `FRenderShaderSet` and `FRenderFrameReadbackDesc`. Proxies and assets are
  owned only by the render thread; composition supplies portable shader bytes
  and geometry, never a backend type.

## Execution contract

- `FRenderExecutor` owns one `FThread`, one bounded Core `TMPSCQueue` of POD
  commands, and a single-reader status triple buffer. `Start` reserves the
  queue and launches the worker; `Enqueue` counts itself in `ActiveProducers`
  before reading the atomic running/stop flags, so rejection stays lock-free
  while `Join` waits for in-flight producers before releasing queue storage.
  Acceptance returns `QueueFull` at capacity and `NotRunning` once stop is
  requested. `RequestStop` clears `IsRunning` immediately under `WakeMutex`,
  the worker drains already queued commands, and `Join` publishes the final
  status including rejections. After `Start`, neither enqueue nor dispatch
  allocates.
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
- With a supplied `FRenderShaderSet` and registered geometry, the frame adds an
  opaque forward pass: the active camera with the lowest proxy ID supplies
  view/projection inline constants, visible proxies issue indexed draws against
  sorted, device-owned geometry, and clear values still come from the snapshot
  sequence. Empty shader bytecode preserves the historical clear-only slice.
- `FRenderProxyRegistry::Sync` validates and canonicalizes the whole mesh
  snapshot by proxy ID, skips invalid records, rejects overflow before mutation
  and never retains snapshot spans. `FRenderAssetRegistry::Register` copies
  CPU geometry through one upload allocation, records the copy on the graphics
  queue and waits for completion before the render thread starts. The frame
  timeline therefore resumes from the queue's completed value after those
  control-time submissions.
- Presentation is opt-in through a portable `IRHISwapchain` in `FRendererDesc`.
  The frame acquires an image, waits on its binary acquire semaphore in the
  submission, signals a binary render-complete semaphore, presents on the
  graphics queue and records `Presented`. `OutOfDate` triggers one `Resize`
  retry; `bSuboptimal` is observable and never silently dropped. Headless
  frames use an offscreen render target and an optional bounded readback whose
  span is valid until the next consumed frame.
- `FNonCopyable` is not used as a base for move-only graph types; those types
  delete copy explicitly and default move. `FRenderGraphContext` takes ownership
  of a moved `FCompiledRenderGraph`, so its bindings and barriers cannot outlive
  the compiled metadata; transient binding also checks the materialized RHI
  descriptor against the declared resource descriptor.

## Render graph contract

- Declaration is capacity-bounded: handles are graph-scoped index/generation
  pairs, arenas are reserved once, and every overflow returns `CapacityExceeded`
  without growing a container. Builders and contexts are move-only.
- Compilation is deterministic: dependencies come from whole-resource accesses
  and explicit edges, the ready set is a min-heap keyed by unique pass ordinal,
  and culling keeps side-effect passes plus the producers and transitive
  producers of exports (not their consumers). Queue dependencies and transitions
  are sorted by total orders derived from segment and pass coordinates.
- Cycles are rejected before culling, including cycles among passes that would
  otherwise be culled. Reading a transient before any writer is
  `ReadBeforeWrite`; two writes to one resource in one pass are
  `AmbiguousWrite`; exporting a transient with no writer is
  `ExportNotProduced`.
- Transients receive half-open lifetimes `[first, last + 1)` in the active
  execution order. The graph only emits `RHI::FRHITransientBatch` requests and a
  handle-to-request map; aliasing is performed by
  `IRHITransientResourceAllocator` in a `BeginBatch`/`Allocate`/`EndBatch` scope,
  never by a second Renderer allocator. Requests whose live intervals overlap
  must receive disjoint byte ranges; compatible disjoint intervals may alias.
- Barriers are emitted from the last state/queue to the next one. Imported
  resources start from their declared initial state/queue and are restored to
  the declared final state/queue; an initial queue transfer is legal only when
  a state change accompanies it, and is recorded in the destination pass.
  Pure ownership transfers with an unchanged state are rejected because the RHI
  barrier representation requires different before/after states.
- Queue segments are maximal topological ranges, each with a monotonically
  increasing per-queue signal value; cross-segment dependencies become timeline
  waits. No native object, command list or backend resource is created during
  compilation or binding.

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
- `Tests/Functional/RenderGraph/` covers DAG construction, cycle rejection,
  culling, ordinal tie-breaking, half-open lifetimes, import/export, barriers,
  queue transfers, legal and illegal aliasing, and literal-seed fuzz against an
  independent stable-topological-order and live-range model (32 replays).
  `Tests/Integration/RenderGraph/` takes a compiled plan through Null transient
  placement, placed-resource binding, resolved transitions, submission,
  timeline-gated release, command-allocator recycling, validation and injected
  `FNullFaultPlan` failure.
- `Tests/Integration/Frame/FirstImageNullTests.cpp` covers indexed quad draws,
  readback after timeline completion, acquire→submit→present ordering, proxy
  retirement across skipped snapshots, validation-on graph barriers and device
  loss before presentation.
- `Benchmarks/Execution/` measures steady-state enqueue and the full CPU clear
  frame at 16/64/256 extents. `Benchmarks/RenderGraph/` measures graph
  declaration plus compilation and aliased transient placement at 10/100/1000
  passes. `Benchmarks/Frame/` measures proxy diffing, deterministic graph
  planning and the whole opaque Null frame at 100/1000/10000 proxies. The
  opaque frame number includes render-thread handoff latency; no GPU timing is
  reported.

## Notes for agents

- RHI transient allocations carry no resource object: materialize a placed
  buffer/texture with the device and a heap, patch `Allocation.Heap` to that
  heap, and bind it through `FRenderGraphContext` before resolving barriers.
- `FRHITransientBatch::Validate()` only validates individual requests; live-range
  non-overlap is a property of the allocator placement and is asserted by the
  aliasing and fuzz tests rather than by the batch itself.
- `FRenderGraphBuilder::Reset` reuses reserved arenas across frames; a frame
  re-imports its target, declares one side-effect pass, compiles and resolves
  the compiled barriers at their recorded points. No arena grows after `Start`.
- Run `RequestRender` only after publishing a snapshot with a nonzero WorldId.
  A zero WorldId is reported as `Idle` and no target is acquired or presented.
- Snapshot sequences start at one. `FWorld` always publishes with a nonzero
  `WorldId`: an explicit nonzero `FWorldDesc::WorldId` is preserved, otherwise
  a process-wide counter starting at 1 is used. The renderer treats a zero
  `WorldId` as the "not published yet" marker and reports `Idle`; the public
  `FSceneExtractor` API does not validate `WorldId`, so autonomous callers must
  supply a nonzero identity to have their frames rendered.
- A queue submits its command list and itself transitions the list to
  `Pending`; the renderer must hand over an `Executable` list and recycle the
  allocator only after the previous completion is reached.
- Scene snapshots carry opaque `FRenderAssetId` values only. Geometry and
  shader bytes are supplied by composition through `RegisterGeometry` and
  `FRendererDesc::Shaders`; the renderer never loads assets, translates shader
  language or includes a concrete backend.
- Windowed OpenGL presentation is not available yet: `CreateRHIOpenGLSurfaceBridge`
  returns `Unsupported` because the device uses a surfaceless EGL context.
  Presentation is therefore exercised through the portable swapchain contract
  on the Null backend; a separate OpenGL pixel-readback reference remains a
  known gap and must not be claimed as verified.
- Runtime boot composition belongs to `Launch`; adding this module to
  `ModuleNames` compiles it but does not register an engine module. Backend
  selection is a composition concern and must not be introduced here.
