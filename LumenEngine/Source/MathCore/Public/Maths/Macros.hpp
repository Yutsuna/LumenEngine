/**
 * @file Macros.hpp
 * @brief Common macros for LumenEngine MathsCore module
 */

#pragma once

#include <limits>
#include <numbers>

namespace LumenEngine
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

} // namespace Maths

} // namespace LumenEngine
