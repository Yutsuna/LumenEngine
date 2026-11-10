/**
 * @file Macros.hpp
 * @brief Common macros for Lumen MathsCore module
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <limits>
#include <numbers>

namespace Lumen
{

namespace Maths
{

    namespace Concepts
    {
        template <typename Type>
        concept CFloatingPoint = std::is_floating_point_v<Type>;
    }

    template <Concepts::CFloatingPoint Type> static inline constexpr Type Pi = std::numbers::pi_v<Type>;

    template <Concepts::CFloatingPoint Type> static inline constexpr Type TwoPi = Type{ 2 } * Pi<Type>;

    template <Concepts::CFloatingPoint Type> static inline constexpr Type HalfPi = Pi<Type> / Type{ 2 };

    template <Concepts::CFloatingPoint Type> static inline constexpr Type DegToRad = Pi<Type> / Type{ 180 };

    template <Concepts::CFloatingPoint Type> static inline constexpr Type RadToDeg = Type{ 180 } / Pi<Type>;

    template <typename Type> static inline constexpr Type Epsilon = std::numeric_limits<Type>::epsilon();

    [[nodiscard]] LUMEN_FORCE_INLINE Bool IsEqual ( Float32 Fa, Float32 Fb, const Float32 Tolerance = Epsilon<Float32> ) noexcept;
    [[nodiscard]] LUMEN_FORCE_INLINE Bool IsEqual ( Float64 Fa, Float64 Fb, const Float64 Tolerance = Epsilon<Float64> ) noexcept;

} // namespace Maths

} // namespace Lumen

#include "Inline/Macros.inl"
