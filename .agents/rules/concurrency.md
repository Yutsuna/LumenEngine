# Rule: Concurrency

**Applies to:** any change that adds threads, tasks, shared mutable state, asynchronous work, or GPU/CPU synchronization.

## Documentation

- Document thread-safety assumptions on every class, function, or data structure that may be accessed concurrently: which thread owns it, which operations are safe without synchronization, and which require external locking.
- State ownership across threads and lifetime across asynchronous work at the point where data crosses a thread boundary.

## Safety

- Avoid data races. Use appropriate synchronization, atomics, or thread-local design.
- When multiple locks can be held simultaneously, establish and document a lock ordering. Avoid holding locks across I/O, GPU submission, or long-running work.
- Consider contention and false sharing: a correct lock or atomic that is hammered by every worker, or a frequently written field sharing a cache line, is a defect for hot paths (evidence per `rules/performance.md`).
- Consider thread affinity where buffers, queues, or GPU work submission assume a stable thread.
- Where CPU work feeds GPU work (or the reverse), synchronization must be explicit: no implicit cross-device ordering assumptions.

## Locking discipline

- Do not recommend lock-free programming by default. The default choice is clear locks or message passing/task queues with simple ordering.
- A lock-free structure requires justification, a correctness argument covering memory ordering, validation (typically ThreadSanitizer plus targeted stress tests), and a fallback strategy. Absent these, use locks.
- Prefer scoped, short critical sections and simple synchronization models.

## Lifetime

- Do not pass references or pointers across threads unless their lifetime is guaranteed for the duration of use.
- Prefer explicit ownership transfer or reference counting over ad-hoc lifetime extensions; ensure objects outlive all asynchronous work that touches them.

## Proportionality

- Do not introduce concurrency merely because it is theoretically possible.
- Prefer measurable scalability: introduce concurrency only where a measurement or identified workload justifies it.
