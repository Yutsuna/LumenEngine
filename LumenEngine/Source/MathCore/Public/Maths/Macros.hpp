/**
 * @file Macros.hpp
 * @brief Common macros for LumenEngine MathsCore module
 */

#pragma once

#include "CoreTypes.hpp"

#include <numbers>

namespace LumenEngine
{

namespace Maths
{

    static constexpr Float64 Pi     = std::numbers::pi;
    static constexpr Float64 TwoPi  = 2.0 * Pi;
    static constexpr Float64 HalfPi = Pi / 2.0;

    static constexpr Float64 DegToRad = Pi / 180.0;
    static constexpr Float64 RadToDeg = 180.0 / Pi;

    static constexpr Float64 Epsilon = 1e-6;

} // namespace Maths

} // namespace LumenEngine
