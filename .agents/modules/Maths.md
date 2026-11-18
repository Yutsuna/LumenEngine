# Module: Maths

- **Type:** Shared library (`Source/Maths/meson.build`), depends on `Core`.
- **Status:** built; Scene depends on it.
- **Doc:** this file lives in `.agents/modules/`; header code and docs stay
  English, standard applies (`rules/coding-style.md`), Core-first applies
  (`rules/core-first.md`).

## Public surface (`Source/Maths/Public/Maths/`)

- `Vec.hpp`, `Matrix.hpp`, `Vertex.hpp` — vector/matrix/vertex math types.
- `SIMD/SIMD.hpp`, `SIMD/SIMDTypes.hpp`, `SIMD/Backend/` — SIMD abstraction and
  per-backend implementations (keep hot paths branch-free and allocation-free).
- `Macros.hpp` — math helper macros.
- `Inline/` — inline/template implementations (`*.inl`, per
  `rules/coding-style.md` file-split rules).

## Notes for agents

- Math hot paths must avoid allocation and dynamic dispatch.
- SIMD backends follow the same API contract; when adding a vectorized
  operation, wire it through `SIMD.hpp` rather than calling intrinsics at
  call sites.
