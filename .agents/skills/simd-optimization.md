---
name: simd-optimization
description: Workflow for introducing or reviewing SIMD optimizations in Lumen.
---

# SIMD Optimization

Use this skill when optimizing math, geometry, or other data-parallel code with SIMD.

## Prefer first

1. SIMD-friendly data layout (contiguous, aligned, SoA where appropriate).
2. Compiler auto-vectorization with clear loop structure and minimal aliasing.
3. Standard algorithms and ranges that vectorize well.

## Before manual intrinsics

1. Profile and identify the hot loop.
2. Determine the target ISA (SSE/AVX/AVX-512/NEON/SVE).
3. Design an abstraction that hides intrinsics behind a clean interface.
4. Provide a scalar fallback path.

## What must not be done

- Do not introduce manual SIMD intrinsics solely because they might be faster; they require justification, a correctness argument, and a fallback.
- Do not mix vector and scalar code in the same hot loop without explicit remainder/scalar tail handling.

## Implementation

1. Align data to vector boundaries when required.
2. Handle remainder/scalar tail explicitly.
3. Avoid mixing vector and scalar code in the same hot loop.
4. Verify correctness with unit tests covering edge cases and tail handling.

## Validation

1. Benchmark before and after.
2. Run the same tests with the scalar fallback.
3. Check assembly or compiler vectorization reports.
4. Review with the performance agent or code-review skill.
