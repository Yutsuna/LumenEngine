/**
 * @file Macros.inl
 * @brief Inline implementations of macros.
 */

#pragma once

#include <Maths/Macros.hpp>

#include <cmath>

Lumen::Bool Lumen::Maths::IsEqual ( const Float32 Fa, const Float32 Fb, const Float32 Tolerance ) noexcept
{
    return std::fabs( Fa - Fb ) <= Tolerance;
}

Lumen::Bool Lumen::Maths::IsEqual ( const Float64 Fa, const Float64 Fb, const Float64 Tolerance ) noexcept
{
    return std::fabs( Fa - Fb ) <= Tolerance;
}

template <typename Type> constexpr Type Lumen::Maths::Max ( Type A, Type B ) noexcept
{
    return std::max( A, B );
}

template <typename Type> constexpr Type Lumen::Maths::Min ( Type A, Type B ) noexcept
{
    return std::min( A, B );
}
