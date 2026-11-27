# Rendering rules

Scene-owned snapshot contracts live in `Source/Scene/Public/Rendering/`;
render-graph, executor and facade declarations live in `Source/Renderer/`.
Renderer is compiled after `Scene` — see `modules/Renderer.md`. Follow
`rules/coding-style.md` and `rules/core-first.md` here as everywhere else.

## Scene extraction and publication

- `FSceneExtractor::Publish` (`Source/Scene/Public/Rendering/SceneExtractor.hpp`)
  validates autonomous spans and publishes a complete snapshot: `ValidateInput`
  checks record capacities, that each mesh `TransformIndex` is inside the
  transform span, and that the referenced transform's `ProxyId` equals the
  mesh `ProxyId`. The public contract of `FSceneExtractionInput` also states
  that proxy IDs must be nonzero and unique within each record family; that
  part is the caller's responsibility — `ValidateInput` does not enforce it
  today. A failure is returned before the writer slot is modified.
- `FRenderSnapshotCapacities` are reserved in all three stream slots at
  initialization; publication within them does not allocate. Overflow and bad
  source indices are reported as structured `FRenderSnapshotError` values
  (`RenderSnapshotTypes.hpp`), never silently grown.
- `FRenderSnapshotView` (`RenderSnapshotView.hpp`) is immutable and non-owning.
  Its spans expire at the reader's next successful `SwapReadBuffers` and must
  never be retained by render-graph or RHI command recording beyond that point.
- `FSceneHierarchy` (`Source/Scene/Public/Hierarchy/SceneHierarchy.hpp`)
  publishes an ActorID-ordered transform span that stays immutable until the
  next explicit `UpdateWorld`/`Publish`; mutations and publication are serial
  game-phase operations with no internal synchronization.

## Render graph (declared and compiled in Renderer)

- `FRenderGraphBuilder`
  (`Source/Renderer/Public/RenderGraph/RenderGraphBuilder.hpp`) declares
  passes, whole-resource reads/writes, explicit dependencies and exports under
  fixed `FRenderGraphCapacities`; handles are graph-scoped and compilation
  consumes the builder.
- `FRenderGraphCompiler` produces `FCompiledRenderGraph`: culled passes,
  barriers, transient requests and queue submission metadata. It creates no
  native or backend object (`CompiledRenderGraph.hpp`).
- `FRenderGraphContext` (`RenderGraphContext.hpp`) binds allocator-produced
  transients once, outside draw and dispatch records; it does not allocate
  memory, create RHI resources, record command lists or own the allocator.
- Queue segments and dependencies carry timeline signal/wait values
  (`RenderGraphTypes.hpp`). `FRenderFrameStatus`
  (`Source/Renderer/Public/Execution/RenderFrameTypes.hpp`) reports the
  requested/consumed frame and sequence plus submitted/completed timeline
  values.

## Frame synchronization

- `FRenderSnapshotStream` is latest-only and single-reader: intermediate
  publications are coalesced and counted by `GetDroppedSnapshotCount`
  (`Source/Scene/Public/Rendering/RenderSnapshotStream.hpp`).
- `FRenderExecutor` has one worker thread and one status reader
  (`SwapStatusBuffers`); commands are bounded POD values, with `QueueFull` in
  the declared error set (`RenderExecutor.hpp`).
