# Module: Scene

- **Type:** Shared library (`Source/Scene/meson.build`), depends on `Core`,
  `Maths`. Benchmarks enabled.
- **Status:** built.
- **Role:** ECS/actor-model layer of the engine.

## Public surface (`Source/Scene/Public/`)

- `Actor/` — `AActor` (pure actor base, actor-model), `FMailBox` (wait-free
  intrusive MPSC mailbox using `FMessage`; its node free-list uses
  `Thread/SPMCQueue.hpp`, a lock-free bounded SPMC queue),
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
  `FSpatialSnapshotView`, `ESpatialRegistryError`, `ESpatialComponentType`
  in `WorldSpatialRegistry.hpp`).
- `Messages/` — `EngineMessageTypes` (`EEngineMessage`, payload structs).
- `Actor/Inline/` — `.inl` implementations.

Tests in `Source/Scene/Tests/Functional/{Actor,World}` (mailbox/fence/push,
actor message/ref/registry dead-letter, world phases, spatial registry,
lifecycle hooks, bounded capacities, ingress journal capacity/sequence/
concurrent producers/frame-targeted application) and `Tests/Integration/World`
(canonical message/command replay at 1/2/4 workers and ingress-journal replay
of 100 ticks × 32 runs over 3 literal seeds with per-consumer FNV digests).

Benchmarks in `Source/Scene/Benchmarks/` (`SceneBenchmarks` binary, suites
`FMailBox`, `AActor`, `FActorRegistry`, `FWorld`, `FWorldExecutor`,
`FSpatialRegistry`): `Actor/ActorMailboxBenchmarks.cpp` (push/pop, batch drain,
MPSC contention), `Actor/ActorBenchmarks.cpp` (RunFrame, ID alloc),
`Actor/ActorLifecycleBenchmarks.cpp` (spawn/destroy churn, deferred command
apply, broadcast with dead letters), `World/WorldBenchmarks.cpp` (spawn
scaling, full-frame ticks at 100/1k/10k actors),
`World/WorldExecutorBenchmarks.cpp` (multi-phase tick and fan-out at 1/2/4
workers, saturated outbox) and `World/WorldSpatialRegistryBenchmarks.cpp`
(transform updates, idle/all-dirty publish, structural churn). Release build
only — see `skills/write-benchmarks.md`.

## Renderer plan — M2/M3 evidence (`TEMP/PLAN_RENDERER.md`)

- Dispatch is canonical: outboxes merge after the phase fence by
  `(Target, Sender, SenderSequence)`; deferred commands sort by sender, command
  sequence, target and payload ordinal. Never use pointer values or hash-map
  iteration order as tie-breakers.
- Logical death (`PendingKill`) closes registry routing before the last C++
  strong ref is released; an external ref may keep storage alive but receives
  no new messages. Dead letters are bounded at 256 records with per-reason
  counters and a dropped-record counter.
- External concurrent inputs are sequenced by `FWorldIngressJournal`; the
  recorded order, not thread scheduling, is what replay reproduces. Records
  carry a non-decreasing target frame and are consumed once by `FWorld` before
  the actor phases. Full capacity returns `CapacityExceeded` and increments a
  drop counter without allocating or changing accepted records.
- Spawns are deliberately not ingress records: replayable construction would
  need a type-erased factory, so spawns stay serial deferred commands ordered
  by `(Sender, SenderSequence, Target, PayloadOrdinal)`.
- Verified for M2/M3: three build trees, Debug suites, TSan Scene/Core,
  clang-format and the Release benchmark JSON reports in `TEMP/BenchM*_Release.json`.

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
