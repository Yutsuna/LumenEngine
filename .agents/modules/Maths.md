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
- `Macros.hpp` — math constants and scalar helpers in `Lumen::Maths`
  (implementations in `Inline/Macros.inl`):
  - `Concepts::CFloatingPoint<Type>` — constrains the angle constants to
    `std::is_floating_point_v<Type>`.
  - Constants: `Pi<Type>`, `TwoPi<Type>`, `HalfPi<Type>`, `DegToRad<Type>`,
    `RadToDeg<Type>` (floating-point `Type`) and `Epsilon<Type>` =
    `std::numeric_limits<Type>::epsilon()` (any `Type`).
  - `IsEqual( Float32 Fa, Float32 Fb, Tolerance = Epsilon<Float32> )` and the
    `Float64` overload — `[[nodiscard]]`, `LUMEN_FORCE_INLINE`, `noexcept`;
    compares `Abs( Fa - Fb ) <= Tolerance`.
  - `Max<Type>( A, B )`, `Min<Type>( A, B )`, `Clamp<Type>( V, Lo, Hi )`
    (`Min( Max( V, Lo ), Hi )`) and `Abs<Type>( V )` — all `constexpr` and
    `noexcept`. `Abs` returns the magnitude by clearing the sign bit of the
    `Float32`/`Float64` representation.
- `Inline/` — inline/template implementations (`*.inl`, per
  `rules/coding-style.md` file-split rules).

## Notes for agents

- Math hot paths must avoid allocation and dynamic dispatch.
- SIMD backends follow the same API contract; when adding a vectorized
  operation, wire it through `SIMD.hpp` rather than calling intrinsics at
  call sites.
