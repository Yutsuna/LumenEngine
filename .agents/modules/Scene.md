# Module: Scene

- **Type:** Shared library (`Source/Scene/meson.build`), depends on `Core`,
  `Maths`. Benchmarks enabled.
- **Status:** built; recent commits focused on it (actors, mailboxes, world).
- **Role:** ECS/actor-model layer of the engine.

## Public surface (`Source/Scene/Public/`)

- `Actor/` — `AActor` (pure actor base, actor-model), `FMailBox` (wait-free
  MPSC mailbox built on `Core/Thread/SPMCQueue.hpp`, uses `FMessage`),
  `FActorMessage`, `FActorRegistry`.
- `World/` — `FWorld` (world/phases, `bInParallelPhase` parallel phasing,
  spatial dirty tracking), `FWorldSpatialRegistry`.
- `Messages/` — `EngineMessageTypes`.
- `Actor/Inline/`, `World/Inline/` — `.inl` implementations.

Tests in `Source/Scene/Tests/{Functional,Integration}/Actor|World|Messages`
(`ActorMailboxFenceTest.cpp`, dead-letter registry tests, world phase tests...).

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
