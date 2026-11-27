# Concurrency rules

Thread and synchronization primitives come from Core
(`rules/core-first.md`, `modules/Core.md`); naming and `noexcept` conventions
come from `rules/coding-style.md`. Module-specific ordering contracts are also
recorded in `modules/Scene.md` and `modules/RHIOpenGL.md`.

## Bounded queues

- `Thread::TMPSCQueue<T>` (`Source/Core/Public/Thread/MPSCQueue.hpp`) allocates
  exactly `InCapacity + 1` nodes in `Initialize`; `Push`, `Pop` and node
  recycling never allocate. Producers are lock-free; the single consumer can
  briefly yield while a producer finishes publishing a recycled slot.
- `Initialize` is a serial control operation ("at least N" capacity) and must
  not race with any other queue operation. A full queue returns a structured
  `QueueFull` failure and leaves accepted messages untouched.
- `Pop` and `IsEmpty` are consumer-only. A producer between its tail exchange
  and link store can make the queue transiently appear empty.

## Actors and tick ordering

- `FMailBox` (`Source/Scene/Public/Actor/ActorMailbox.hpp`) is the bounded MPSC
  mailbox over `TMPSCQueue`. `Push`/`TryPush` are safe for concurrent
  producers; `Pop` is single-consumer. Per-producer FIFO is preserved, but
  concurrent producers have no deterministic total order.
- `BeginTick`/`EndTick` fence on accepted-message counts. A producer preempted
  between linking its node and publishing the accepted count can be deferred
  by one additional fence; no message is lost or reordered.
- `FMailBox::Reserve` is a serial control operation: it must not race with a
  producer or consumer (`ActorMailbox.hpp`).
- Failed sends are dead-lettered by `FActorRegistry`
  (`Source/Scene/Private/Actor/ActorRegistry.cpp`), not dropped silently.
  `EActorLifecycle` and `EActorSendError::ActorRetiring` describe retirement
  (`Source/Scene/Public/Actor/ActorLifecycle.hpp`); `FWorld` marks an actor
  pending kill before retiring it (`Source/Scene/Private/World/World.cpp`).

## Latest-only publication and executor threads

- `FRenderSnapshotStream` owns three pre-reserved slots, one extractor
  publishes and one reader calls `SwapReadBuffers`; a view is stable until the
  next successful swap (`Source/Scene/Public/Rendering/RenderSnapshotStream.hpp`).
- `FRenderExecutor` owns one render worker thread, a bounded MPSC queue of POD
  commands, `RequestStop` draining already queued commands, and a single-reader
  status triple buffer
  (`Source/Renderer/Public/Execution/RenderExecutor.hpp`). After queue
  initialization neither enqueue nor dispatch allocates. See
  `modules/Renderer.md`.
