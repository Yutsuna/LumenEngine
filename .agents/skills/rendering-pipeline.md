---
name: rendering-pipeline
description: Workflow for designing render passes, pipelines, and GPU submission in Lumen.
---

# Rendering Pipeline

Use this skill when adding or modifying render passes, pipeline state objects, or GPU submission logic.

**Note:** the `Rendering` and `RHI` modules do not exist yet; this skill applies to generic rendering code wherever it currently lives, and to the design of those modules when the task creates them.

## Design steps

1. Identify the pass's inputs, outputs, and side effects.
2. Decide whether the pass is generic (belongs in `Rendering`) or backend-specific (belongs in a backend implementation) — `rules/architecture.md`.
3. Define the RHI resources the pass consumes through the generic interface only; no Vulkan/OpenGL types in generic code.
4. Document frame-level ordering, synchronization, and resource lifetime.
5. Label design rationale as an architectural decision, not a measurement (`rules/performance.md`).

## Implementation checks

1. Keep passes stateless or explicitly stateful; avoid hidden global state.
2. Minimize CPU/GPU synchronization points; make cross-device dependencies explicit.
3. Batch command submissions where possible.
4. Use descriptor/binding updates that match the RHI abstraction rather than backend-specific ones.

## Performance checks

1. Consider draw-call batching, culling, and LOD only where a workload justifies it.
2. Prefer GPU-driven approaches only where profiling supports it (`rules/performance.md`).
3. Avoid premature optimization: profile first, change one variable at a time.

## What must not be done

- Do not generalize one backend's successful run to other backends.
- Do not add hidden global state to render passes.
- Do not introduce backend-specific types, enums, or handles into generic RHI code.
