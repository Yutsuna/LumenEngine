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
  `FMessage` (`ActorMessage.hpp`), `FActorRef` and `ActorID`
  (`ActorTypes.hpp`), `Internal::FActorRegistry` (`ActorRegistry.hpp`),
  `EActorLifecycle`/`EActorTickPhase` and structured send,
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
  (`SceneHierarchy.hpp`, `Inline/SceneHierarchy.inl`). Level iteration is exposed
  through `GetDepth`, `GetLevelStarts`, `GetLevelOrderedTransforms` and an
  allocation-free `ForEachLevel` visitor.
- `WorldPartition/` — `FWorldCellId`, `EWorldCellState`,
  `EPartitionCommandKind`, `EWorldPartitionError`, `FWorldPartitionConfig`,
  `FPartitionJournalRecord`, `FWorldPartitionStats` (`WorldPartitionTypes.hpp`)
  and `FWorldPartition` (`WorldPartition.hpp`), a per-world bounded logical-cell
  registry with a canonical replay journal and no actor ownership.
- `Replication/` — `FReplicationSystem`, `FReplicationConfig`, `FPersistentActorId`
  and structured replication errors (`ReplicationSystem.hpp`,
  `ReplicationTypes.hpp`), `FReplicationFrame`/`FReplicationRecord`
  (`ReplicationFrame.hpp`) and `IReplicationTransport` (`ReplicationTransport.hpp`).
  See `rules/replication.md` for the fixed M9 arbitrages and wire contract.
- `Messages/` — `EngineMessageTypes.hpp` (`EEngineMessage`, payload structs).
- `Actor/Inline/`, `World/Inline/`, `WorldPartition/Inline/`,
  `Replication/Inline/` — `.inl` implementations.

Tests in `Source/Scene/Tests/Functional/{Actor,World,Rendering,Hierarchy,Replication,WorldPartition}`
(mailbox/fence/push, bounded mailbox capacity/saturation/fence/1M-allocation-free
push-pop, actor message/ref/registry dead-letter, world phases, spatial registry,
lifecycle hooks, bounded capacities, ingress journal capacity/sequence/
concurrent producers/frame-targeted application, snapshot canonical order,
latest-only publication, held-view stability and capacity errors) and
`Tests/Integration/{World,Rendering,Replication,WorldPartition}` (canonical
message/command replay at 1/2/4 workers and ingress-journal replay of 100 ticks
× 32 runs over 3 literal seeds with per-consumer FNV digests; World-owned
extraction across skipped frames, deletions, isolated worlds, capacity+1 and
allocation-free publication). Replication coverage is described in the M9
section below; `Tests/Functional/WorldPartition/` and
`Tests/Integration/WorldPartition/` add bounded membership, canonical
journal/replay, swap-remove index integrity, routing reconciliation, ingress
broadcast skipping, side-effect-free authority conflicts and snapshot/routing/
hierarchy isolation.

Benchmarks in `Source/Scene/Benchmarks/` (`SceneBenchmarks` binary, suites
`FMailBox`, `AActor`, `FActorRegistry`, `FWorld`, `FWorldExecutor`,
`FSpatialRegistry`, `FWorldRenderExtraction`, `FSceneHierarchy`,
`FReplicationApply`, `FReplicationEcho`, `FReplicationFrame`,
`FWorldPartitionBench`, `FPartitionProxyChurn`):
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
at 100/1k/10k proxies with bytes copied per iteration),
`Hierarchy/SceneHierarchyBenchmarks.cpp`, `Replication/{ReplicationApply,
ReplicationEcho,ReplicationFrame}Benchmarks.cpp` (M9 suites — see the M9
section), `WorldPartition/WorldPartitionBenchmarks.cpp` (cell churn,
incremental extraction with a fixed dormant half, bounded queries) and
`WorldPartition/PartitionProxyChurnBenchmarks.cpp` (snapshot churn through
rendering DTOs only). Release build only — see `skills/write-benchmarks.md`.

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
  allocator counter. Scene suite 147/147; Debug and TSAN clean.
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
  canonically sorted** snapshot. Capacity failures return a
  `FRenderSnapshotError` with
  `Code == ERenderSnapshotErrorCode::CapacityExceeded`, are observable through
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
  benchmarks at reserved capacities 100/1k/10k: idle (empty publication)
  ~30 ns, all-dirty ~7 us/~81 us/~1.6 ms at 100/1k/10k proxies, structural
  churn ~3.6 us/~42 us/~540 us, with bytes copied reported per iteration.
  Debug, TSAN, clang-format and all three builds are clean.

## M8 — Spatial hierarchy (contract and evidence)

- `FSceneHierarchy` already existed as a committed, bounded, weak-ID component;
  M8 **integrates, validates and hardens** it rather than creating it. Nodes hold
  only `ActorID` parent/sibling links, local/world matrices, depth and dirty bits:
  no `AActor *`, no `TSharedPtr` parent-child cycle, no renderer type.
- `FWorldDesc::HierarchyCapacity` (default `0`) reserves the hierarchy once at
  construction. Zero keeps the M0-M7 behaviour unchanged. Registration is
  **opt-in** through `FWorld::TryRegisterHierarchyActor`; `TryHierarchyAttach`,
  `TryHierarchyDetach` and `TrySetHierarchyLocalTransform` validate at apply time
  and route through `FDeferredCommandPayload` inside an actor phase. The four new
  command alternatives are appended so existing `Payload.index()` values and the
  canonical `(Sender, SenderSequence, Target, index)` ordering stay stable.
- A stale parent ID is rejected with `ESceneHierarchyError::StaleParent` by
  checking the full generational `ActorID` against `FWorld::Actors`; no public
  registry generation API was added. Queue saturation returns
  `ESceneHierarchyError::DeferredQueueFull` with no mutation.
- Default destruction policy detaches children: `DestroyActorImpl` calls
  `Hierarchy.Unregister`, direct children become roots and keep their current
  world transform as their local transform. Recursive subtree destruction is not
  implicit and remains a separate explicit command if ever needed.
- Exactly one **serial** `Hierarchy.UpdateWorld()` pass runs after the last phase
  fence and before `Registry.Publish()` in both `TryTick` and `TryFixedTick`.
  Workers never write the hierarchy. Published records are copied into
  `FSpatialRegistry` through `UpdateTransform`; `PublishRenderSnapshot()` and
  `FRenderTransformRecord` are unchanged, so the renderer boundary is untouched.
  `ForEachLevel` uses a counting-sort level view rebuilt in place, without
  allocation and with ActorID order inside each level.
- Evidence: 14 new tests under `Tests/Functional/Hierarchy/` and
  `Tests/Integration/Rendering/HierarchyTransformPublicationTests.cpp` cover
  capacity+1, self/unknown/cyclic parents with strong guarantee, detach and
  unregister semantics, canonical level views, deferred application on the next
  tick, stale parents, saturated queues, and complete-snapshot publication with
  removal visible after skipped publishes. Replay digests are computed over
  canonical ranks and matrix bits (never absolute generational IDs or addresses)
  across 3 literal seeds × 32 runs × 1/2/4 workers; 0 allocation after reserve.
- Release benchmarks: `Hierarchy/{Chain,WideTree,LeafUpdates,Publish,StructuralChurn}`
  at 100/1k/10k, and `Renderer/Benchmarks/Frame/HierarchyExtractionBenchmarks.cpp`
  for frame cost bounded by published records (100/1k/10k). The existing AoS node
  layout was kept: the measurements are dominated by the parent-first traversal,
  so an SoA rewrite showed no justification. JSON reports are kept in `TEMP/`.

## M9 — bounded state replication

- Public surface `Replication/` — `FPersistentActorId`, `EActorRole`,
  `EReplicationDirection`, `EReplicationChannel`, `EActorReplicationProperty`,
  `EActorReplicationKind`, `EReplicationControlKind`, `FDirtyPropertyMask`,
  `FReplicationConfig`, `FReplicationStats`, `FReplicationChannelStats`,
  `FReplicationFrame`, `FReplicationRecord`, `FReplicationFrameBuilder`,
  `IReplicationTransport`, `IReplicationActorHandler`, `FReplicationSourceView`
  and `FReplicationSystem`. No `AActor*`, no RHI and no backend type crosses
  this boundary. The four arbitrages (determinism, authority, persistent ID,
  hierarchy destruction) are recorded in `rules/replication.md`.
- `FWorldDesc::ReplicationConfig`/`ReplicationRole` are opt-in. The world never
  creates a transport or a handler: `InitializeReplication` injects both; a zero
  `MaxActors` leaves `GetReplicationSystem()` null. The system ticks in the
  serial control phase after `ApplyIngressJournal()` and before hierarchy
  publication, so replicated local transforms and parents are resolved by the
  same `Hierarchy.UpdateWorld()` pass.
- All storage is reserved by `Initialize`: shadow maps, resend ring, gap buffer,
  control records, pending parents and received inputs. Overflow is structured
  `CapacityExceeded`/`OverflowFrameDropCount` with the previous state intact; no
  per-actor priority is given (documented arbitrage).
- Frames carry absolute values; the dirty mask selects fields. Only `Reliable`
  is acked/baselined per `(Direction, Channel)`. Control records are unsequenced
  and idempotent, including `Ack`, `Nack`, `BaselineRequest` and `BaselineReady`.
  Stale baselines escalate to a full snapshot after a bounded number of resends
  with a cooldown. Wire codec is fixed-endian with an FNV-1a checksum.
- Inputs are bounded wire records translated into the existing
  `FWorldIngressJournal`; no second journal and no network transport are added.
  Visibility is owned by the replication system and published through
  `ERenderTransformFlags`, never through a new renderer field.
- Tests live in `Tests/{Functional,Integration}/Replication/`, benchmarks in
  `Benchmarks/Replication/`. Canonical digests sort by persistent ID and fold
  matrix bits, visibility, parent and alive; local IDs, sequences, acks and
  stats are excluded. Seeds are literal, with 32 runs and 1/2/4 workers.

## M10 — World partition (contract and evidence)

- `FWorldPartition` is opt-in through an all-or-nothing `FWorldDesc::PartitionConfig`;
  a zero configuration leaves M0-M9 behaviour unchanged. Cells are **logical**:
  the registry owns membership and dormancy by weak `ActorID`, never actors,
  and never allocates identities. `FPersistentActorId` is reused from M9 as an
  opaque authority label: with replication active the authority allocates it
  through `ReplicationSystem::AssignPersistentId` and the replica must already
  have it bound; a mismatch is `PersistentConflict`. No property rank is added
  to M9 — ranks 16..31 remain reserved but unused here.
- Assignment/unassignment are **control-time only** and journaled with their
  full control-time payload (`Kind`, `CellId`, `ActorId`, `PersistentId`;
  `FrameIndex` and `SenderSequence` are zero for these records). Load/unload are
  appended alternatives of the existing bounded `FDeferredCommandPayload` and
  applied in `ApplyDeferredCommands()` at the top
  of the tick, before `TickReplication()`, actor phases, the serial hierarchy
  pass, `Registry.Publish()` and extraction. There is no second queue and no
  mid-frame state change: every consumer sees one committed cell state.
- An unloaded actor is not ticked, not broadcast to (including `EWorldIngressKind::Broadcast`
  replays), not routable (`FActorRegistry::CloseRouting`), and physically absent
  from the next full snapshot in all four record families. Routing is reconciled
  on assignment to a still-unloaded cell and restored on unassignment of a
  dormant actor; a conflicting authority label is rejected before any ID is
  allocated, and cell/capacity/journal validation also precedes allocation so
  no irreversible binding leaks on a failed call. This is distinct from M9
  visibility, which keeps the transform and sets `ERenderTransformFlags::None`.
  Dormancy is filtered **only at snapshot extraction**: bound dormant actors stay
  in the replication source view so an acked replica never emits a Destroy
  tombstone for them. Replicated writes are still applied to the hierarchy node
  and become visible on load; composition must not issue local writes to dormant
  actors. Destroy is refused while the partition journal cannot record the
  unassignment, so no destroyed `ActorID` stays registered.
- Unload detaches direct children outside the cell, preserving their world
  transform as local; children inside the cell stay linked but dormant. No
  recursive destruction and no cell ownership of actors.
- Filing contract for a future `concurrency.md`: only the game thread mutates
  the partition. Control-time calls commit immediately; calls issued from a
  parallel phase are deferred and committed by `ApplyDeferredCommands()` at the
  top of the next tick, before `TickReplication()`. Workers never write the
  partition. The canonical digest folds journal fields in this order:
  `(Kind, CellId, PersistentId, FrameIndex, SenderSequence)`, plus cell states —
  never local `ActorID`s or addresses. Evidence uses 3 literal seeds × 32 runs ×
  1/2/4 workers and a scoped allocation counter reporting 0 allocations after
  reserve.
- Release benchmarks are `FWorldPartitionBench/{CellChurn,IncrementalExtraction,
  BoundedQueries}` at 100/1k/10k actors and `FPartitionProxyChurn.Snapshot`
  through rendering DTOs only; no new Renderer include violates the include DAG.
  Tests live in `Tests/{Functional,Integration}/WorldPartition/` and verify
  capacity+1, partial configuration, journal order, swap-remove index integrity
  after unassignment, routing reconciliation on assign/unassign, dormant actors
  skipped by ingress broadcasts, side-effect-free authority conflicts, no M9
  tombstone for a bound dormant actor, destruction of unassigned actors, refusal
  on a full partition journal, conflict counters, 3 seeds × 32 runs × 1/2/4
  workers, skipped publications, two isolated worlds and cross-cell detach.
