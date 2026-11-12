# Module: Scene

- **Type:** Shared library (`Source/Scene/meson.build`), depends on `Core`,
  `Maths`. Benchmarks enabled.
- **Status:** built.
- **Role:** ECS/actor-model layer of the engine.

## Public surface (`Source/Scene/Public/`)

- `Actor/` — `AActor` (pure actor base, actor-model), `FMailBox` (wait-free
  intrusive MPSC mailbox using `FMessage`; its node free-list uses
  `Core/Thread/SPMCQueue.hpp`, a lock-free bounded SPMC queue),
  `FMessage` (`ActorMessage.hpp`), `FActorRegistry`, `FActorRef` and `ActorID`
  (`ActorTypes.hpp`).
- `World/` — `FWorld` (world/phases, `bInParallelPhase` parallel phasing,
  spatial dirty tracking), `FSpatialRegistry` (+ `FSpatialRegistryData`,
  `ESpatialComponentType`).
- `Messages/` — `EngineMessageTypes` (`EEngineMessage`, payload structs).
- `Actor/Inline/` — `.inl` implementations.

Tests in `Source/Scene/Tests/Functional/{Actor,World}` (mailbox/fence/push,
actor message/ref/registry dead-letter, world phases, spatial registry...).
`Source/Scene/Tests/Integration/` exists but is currently empty.

Benchmarks in `Source/Scene/Benchmarks/` (`SceneBenchmarks` binary, suites
`FMailBox`, `AActor`, `FActorRegistry`, `FWorld`, `FSpatialRegistry`):
`Actor/ActorMailboxBenchmarks.cpp` (push/pop, batch drain, MPSC contention),
`Actor/ActorBenchmarks.cpp` (RunFrame, ID alloc), `World/WorldBenchmarks.cpp`
(spawn scaling, full-frame ticks at 100/1k/10k actors), and
`World/WorldSpatialRegistryBenchmarks.cpp` (transform updates, idle/all-dirty
publish, structural churn). Release build only — see
`skills/write-benchmarks.md`.

## Notes for agents

- Actor communication only via mailbox `Push` of `FMessage`s; no shared
  mutable actor state — the mailbox return value (enqueue success) matters
  and is tested.
- Message delivery failures are **dead-lettered** by the registry, not
  dropped silently; keep that contract when touching `ActorRegistry`.
- Spatial sorting/dirty tracking is deferred until publish — don't eagerly
  re-sort inside mutation paths (recent commit "Defer spatial dirty tracking
  until publish").
- Do not export private/internal tags; only Public/ symbols carry
  `SCENE_EXPORT`.
