---
name: performance-analysis
description: Workflow for profiling and benchmarking Lumen.
---

# Performance Analysis

Use this skill when investigating performance or justifying an optimization.

## Before optimizing

1. Define a reproducible workload and metric (frame time, throughput, latency, memory).
2. Profile with a sampling or instrumentation tool (`perf`, `Tracy`, `valgrind --tool=callgrind`, GPU profilers).
3. Identify the actual bottleneck. Do not guess.

## During optimization

1. Change one thing at a time.
2. Measure before and after under the same conditions.
3. Record hardware, compiler, build type, and workload.

## What must not be done

- Do not add complexity, hand-written SIMD, or lock-free structures solely because they might be faster.
- Do not claim a performance improvement without a measurement or an explicitly identified theoretical basis.
- Do not present an assumption as a measured fact.

## Validation

1. Re-run the benchmark and confirm the improvement.
2. Run correctness tests to ensure the optimization did not break behavior.
3. Run sanitizers if the optimization changed memory layout or concurrency.
