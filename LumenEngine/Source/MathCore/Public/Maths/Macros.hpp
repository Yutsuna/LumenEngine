/**
 * @file Macros.hpp
 * @brief Common macros for LumenEngine MathCore module
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Math
{

    static constexpr Float64 PI      = 3.14159265358979323846;
    static constexpr Float64 TWO_PI  = 2.0 * PI;
    static constexpr Float64 HALF_PI = PI / 2.0;

    static constexpr Float64 DEG_TO_RAD = PI / 180.0;
    static constexpr Float64 RAD_TO_DEG = 180.0 / PI;

    static constexpr Float64 EPSILON = 1e-6;

} // namespace Math

} // namespace LumenEngine
