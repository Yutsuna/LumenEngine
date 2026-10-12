# Rule: Performance

**Applies to:** any change motivated by speed, scalability, memory use, or any claim about performance.

## Evidence hierarchy

```text
Correctness
    ↓
Architecture
    ↓
Measurement
    ↓
Optimization
```

- Correctness first: never sacrifice correctness for an unmeasured performance assumption.
- Establish that the change fits the architecture before measuring, and measure before claiming an optimization.
- Do not claim a performance improvement without a measurement, or an explicitly identified theoretical basis (e.g. reduced work proven by construction). Say which one you have.
- Profile before optimizing. Identify the actual bottleneck with evidence before changing code.
- Distinguish measured facts, theoretically expected behavior, and assumptions. Never present an assumption as a measured fact (see also `rules/validation.md` status vocabulary).

## Research reproducibility

Lumen is a research platform. In performance-related documentation, keep these four categories separate and labeled:

| Category | Meaning |
| --- | --- |
| **Implementation** | What the code does. |
| **Measurement** | What was empirically observed. |
| **Hypothesis** | What is expected to happen (unverified). |
| **Architectural decision** | Why a design was selected. |

When introducing a benchmark or reporting a measurement, record, where relevant: compiler and version, build configuration, CPU, relevant GPU, operating system, dataset/workload, and benchmark methodology. Do not require every trivial timing observation to carry all of these — require them when the result is used to justify a decision or a claim.

## Avoid speculative optimization

- Do not add complexity, hand-written SIMD, or lock-free structures solely because they might be faster.
- Prefer simple, correct code until profiling justifies a change.
- Avoid unnecessary allocations, copies, virtual dispatch, hidden synchronization, and backend synchronization where avoidable.
- Change one variable at a time; measure before and after under identical conditions.

## Design considerations

Analyze only the dimensions relevant to the change; do not treat this list as a per-change checklist:

- CPU cache locality and data layout (SoA vs AoS where appropriate);
- memory bandwidth;
- allocation behavior and memory pressure;
- branch behavior and predictability;
- instruction-level parallelism;
- SIMD opportunities and vectorizable layouts;
- synchronization, contention, and false sharing;
- CPU/GPU synchronization;
- GPU workload characteristics and command submission;
- resource lifetime and scalability across cores and devices.

## SIMD

- SIMD is a first-class consideration, but architecture-specific intrinsics require a clear abstraction strategy and a correct fallback.
- Prefer compiler auto-vectorization and SIMD-friendly data layouts before manual intrinsics.
- Preserve correctness: alignment, padding, and scalar fallbacks must be handled explicitly.

## Evidence required before claiming success

1. The bottleneck you identified, with profiling or benchmark evidence.
2. Before/after numbers under the same conditions, or an explicit statement that the change is architecturally motivated and not yet measured.
3. Correctness validation (`rules/validation.md`) after the change.
