---
name: parallel-programming
description: Workflow for adding or reviewing concurrency and parallelism in Lumen.
---

# Parallel Programming

Use this skill when adding threads, task systems, job systems, or shared mutable state.

## Design steps

1. Identify whether the work is CPU-bound and can scale across cores.
2. Choose a synchronization model: task system, worker threads, lock-free structure, or explicit locks.
3. Document ownership, thread affinity, and safe access patterns.

## Implementation checks

1. Keep critical sections short and well-ordered to avoid deadlocks.
2. Do not pass references across threads unless lifetime is guaranteed.
3. Prefer message passing or task queues over fine-grained shared state.
4. Use atomics only when you can prove correctness; otherwise use locks.

## Lock-free code

1. Required: justification, correctness argument, memory-ordering analysis, and fallback.
2. Review by the performance or validation agent before merging.

## What must not be done

- Do not recommend lock-free programming by default. The default choice is clear locks or message passing/task queues with simple ordering.
- A lock-free structure requires justification, a correctness argument covering memory ordering, validation (typically ThreadSanitizer plus targeted stress tests), and a fallback strategy. Absent these, use locks.
- Do not introduce concurrency merely because it is theoretically possible. Prefer measurable scalability: introduce concurrency only where a measurement or identified workload justifies it.
- Do not pass references or pointers across threads unless their lifetime is guaranteed for the duration of use.

## Validation

1. Run under ThreadSanitizer (`-Denable_tsan=true`) for concurrency changes.
2. Test on machines with different core counts when possible.
3. Verify deterministic behavior for deterministic subsystems.
