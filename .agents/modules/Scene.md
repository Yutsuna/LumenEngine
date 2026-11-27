# Module: Scene

- **Type:** Shared library (`Source/Scene/meson.build`), depends on `Core`,
  `Maths`. Benchmarks enabled.
- **Status:** built.
- **Role:** ECS/actor-model layer of the engine.

## Public surface (`Source/Scene/Public/`)

- `Actor/` — `AActor` (pure actor base, actor-model), `FMailBox` (bounded MPSC
  mailbox using `FMessage` over Core's `Thread/MPSCQueue.hpp` (`TMPSCQueue`), with explicit
  capacity descriptor, fallible `TryPush` and saturation metrics; no `new`/
  `delete` on any message path), `FMailBoxDesc`, `EActorMailboxError`,
  `DefaultMailboxCapacity` (32 slots),
  `FMessage` (`ActorMessage.hpp`), `FActorRegistry`, `FActorRef` and `ActorID`
  (`ActorTypes.hpp`), `EActorLifecycle`/`EActorTickPhase` and structured send,
  spawn and destroy errors (`ActorLifecycle.hpp`), `FActorEmitContext`
  (`ActorEmitContext.hpp`, bounded per-actor outbox).
- `World/` — `FWorld` (world/phases, `bInParallelPhase` parallel phasing,
  spatial dirty tracking, fallible `TryTick`/`TryFixedTick`/`TrySpawnActor`/
  `TryDestroyActor`/`TryBroadcast`, `SetIngressJournal`), `FWorldDesc`/
  `FWorldFrameContext`/`FWorldTickResult` (`WorldTypes.hpp`),
  `FWorldIngressJournal`/`FWorldIngressRecord`/`EWorldIngressKind`/
  `EWorldIngressError` (`WorldIngressJournal.hpp`, bounded sequenced log of
  concurrent external messages/broadcasts/destroys consumed serially at each
  tick and used by replay), `FSpatialRegistry`
  (+ `FSpatialRegistryCapacities`, `FSpatialRegistryStorage`,
  `FSpatialSnapshotView`, `FSpatialCommittedView` (owner-only committed-ID/
  transform view used by extraction), `ESpatialRegistryError`, `ESpatialComponentType`
  in `WorldSpatialRegistry.hpp`), `FWorldRenderContent` (`WorldRenderContent.hpp`,
  authored non-actor render records).
- `Rendering/` — `FSceneExtractor`/`FSceneExtractionInput` (`SceneExtractor.hpp`),
  `FRenderSnapshotStream`, `FRenderSnapshotStorage`, `FRenderSnapshotView`
  and `FRenderSnapshotCapacities`/`FRenderSnapshotHeader`/
  `FRenderSnapshotError` (`RenderSnapshot*.hpp`), plus `RenderRecords.hpp`,
  `RenderIds.hpp` and `Inline/RenderIds.inl`. These headers are the only Scene
  surface the renderer consumes.
- `Hierarchy/` — `FSceneHierarchy`, `FSceneHierarchyTransformRecord`,
  `FSceneHierarchyUpdateResult` and `ESceneHierarchyError`
  (`SceneHierarchy.hpp`).
- `Messages/` — `EngineMessageTypes` (`EEngineMessage`, payload structs).
- `Actor/Inline/`, `World/Inline/` — `.inl` implementations.

Tests in `Source/Scene/Tests/Functional/{Actor,World,Rendering}` (mailbox/fence/push,
bounded mailbox capacity/saturation/fence/1M-allocation-free push-pop,
actor message/ref/registry dead-letter, world phases, spatial registry,
lifecycle hooks, bounded capacities, ingress journal capacity/sequence/
concurrent producers/frame-targeted application, snapshot canonical order,
latest-only publication, held-view stability and capacity errors) and
`Tests/Integration/{World,Rendering}` (canonical message/command replay at
1/2/4 workers and ingress-journal replay of 100 ticks × 32 runs over 3
literal seeds with per-consumer FNV digests; World-owned extraction across
skipped frames, deletions, isolated worlds, capacity+1 and allocation-free
publication).

Benchmarks in `Source/Scene/Benchmarks/` (`SceneBenchmarks` binary, suites
`FMailBox`, `AActor`, `FActorRegistry`, `FWorld`, `FWorldExecutor`,
`FSpatialRegistry`, `FWorldRenderExtraction`):
`Actor/ActorMailboxBenchmarks.cpp` (push/pop, structured TryPush, batch drain,
preallocated saturation, MPSC contention), `Actor/ActorBenchmarks.cpp`
(RunFrame, ID alloc),
`Actor/ActorLifecycleBenchmarks.cpp` (spawn/destroy churn, deferred command
apply, broadcast with dead letters), `World/WorldBenchmarks.cpp` (spawn
scaling, full-frame ticks at 100/1k/10k actors),
`World/WorldExecutorBenchmarks.cpp` (multi-phase tick and fan-out at 1/2/4
workers, saturated outbox) and `World/WorldSpatialRegistryBenchmarks.cpp`
(transform updates, idle/all-dirty publish, structural churn) and
`Rendering/RenderExtractionBenchmarks.cpp` (idle, all-dirty, structural churn
at 100/1k/10k proxies with bytes copied per iteration). Release build
only — see `skills/write-benchmarks.md`.

## Notes for agents

- Actor communication only via mailbox `Push` of `FMessage`s; no shared
  mutable actor state — the mailbox return value (enqueue success) matters
  and is tested.
- Message delivery failures are **dead-lettered** by the registry, not
  dropped silently; keep that contract when touching `ActorRegistry`.
- Spatial sorting/dirty tracking is deferred until publish — don't eagerly
  re-sort inside mutation paths (the deferred-publish design lives in
  `FSpatialRegistry`).
- `FSpatialRegistry` follows the rendering snapshot pattern: three
  pre-reserved `FSpatialRegistryStorage` slots are owned by the registry and
  readers receive an immutable `FSpatialSnapshotView` that expires at the next
  successful swap. `Publish()` must never allocate; overflow is rejected by
  `TryRegisterSpatialEntity` with `ESpatialRegistryError::CapacityExceeded`
  and counted, never silently grown.
- `FWorld` owns its registry and exposes `GetSpatialRegistry()`; the static
  `FSpatialRegistry::Get()` remains only as a legacy shim for callers that
  have not been migrated.
- Do not export private/internal tags; only Public/ symbols carry
  `SCENE_EXPORT`.

## M1 — bounded mailbox (contract and evidence)

- `FMailBoxDesc{ Capacity = 32 }` sets the exact initial slot count. `Reserve`
  is a **serial control operation** with "at least N" semantics: a no-op when
  the current capacity already satisfies the request, otherwise it allocates
  a replacement queue and transfers pending messages without resetting the
  accepted/consumed counters. An invalid capacity leaves a mailbox that
  rejects every send with `EActorSendError::MailboxFull`.
- `Push` keeps the legacy `Bool` contract and logs rejections; `TryPush`
  returns `TExpected<void, EActorSendError>` with `MailboxFull` for both a
  full and an uninitialized mailbox. Every rejection increments
  `GetSaturationCount()` and leaves all accepted messages untouched. The
  registry translates that failure into the `MailboxFull` dead-letter reason.
- `BeginTick`/`EndTick` now fence on **accepted-message counts** instead of a
  tail pointer. Producers publish their accepted count after linking the node;
  a producer preempted in that window can be deferred by one additional
  fence. No message is lost or reordered. `Pop` is single-consumer only.
- Progress: producers are lock-free through Core's MPSC free ring; the single
  consumer can briefly yield while a producer finishes publishing a recycled
  slot. There is no `new`/`delete` in `Push`, `TryPush`, `Pop` or recycling.
- Evidence: 6 new `FMailBox_Bounded` tests, including capacity N/N+1,
  saturation counts, two-fence batching, growth with pending messages and
  **1M push/pop with 0 allocations** measured by a scoped test-only global
  allocator counter. Scene suite 140/140; Debug and TSAN clean.
- Release trade-off analysed: replacing the SPMC free list with a generic
  multi-producer MPSC free ring adds a CAS on send plus one accepted-count
  atomic. Micro `PushPopMessage` moved from ~19 ns to ~32 ns and the M2
  world benchmarks correspondingly slowed (e.g. `MessageFanOut/1` ~119 →
  ~239 us). This is the intentional M1 cost: bounded deterministic storage
  with zero hot-path allocation. Batch drain improved (~295 → ~150 us at
  4096 messages). No hidden allocation remains on any message path.

## Immutable render snapshot extraction

- `FWorld` owns one `FRenderSnapshotStream`, one `FSceneExtractor`, and all
  record scratch storage per instance. There is no process-global path in the
  renderer boundary and no actor, asset or RHI ownership in any record.
- Extraction is opt-in: any non-zero capacity in `FWorldDesc::RenderCapacities`
  enables a publication after the last phase fence and spatial publish in
  `TryTick`/`TryFixedTick`. The default descriptor keeps the historical
  allocation and sequencing behaviour unchanged.
- Transform records are derived from `FSpatialRegistry::GetCommittedView()`:
  `FRenderProxyId` is the committed `ActorID`, the matrix is the committed
  world transform, and visibility/bounds are deterministic defaults. Mesh,
  camera and light records come from `FWorldRenderContent`, authored only in
  control operations. Mesh `TransformIndex` is rewritten from `ProxyId`, so
  callers never need to know the canonical rank.
- `PublishRenderSnapshot()` validates every family against its reserved
  capacity before touching scratch arrays, then publishes a **complete,
  canonically sorted** snapshot. Capacity failures return
  `FRenderSnapshotError::CapacityExceeded`, are observable through
  `GetLastRenderError()` and leave the previous publication and reader view
  untouched. A later success clears the error.
- The reader side is latest-only: exactly one thread calls
  `SwapRenderSnapshotReadBuffers()`; a view stays immutable until that swap,
  intermediate publications are counted by `GetDroppedSnapshotCount()`, and
  removals remain visible because every snapshot is complete rather than
  delta-encoded.
- Evidence: 7 integration tests and 2 rendering contract files, including
  two isolated worlds, deletion across skipped publications, a held view
  surviving two later publications, capacity+1 rejection, canonical order
  after scrambled registration and **zero allocations across 1000 publishes**
  measured with a scoped global allocation counter. Release extraction
  benchmarks at 100/1k/10k proxies: idle ~30 ns, all-dirty ~7 us/~81 us/~1.6 ms,
  structural churn ~3.6 us/~42 us/~540 us, with bytes copied reported per
  iteration. Debug, TSAN, clang-format and all three builds are clean.
