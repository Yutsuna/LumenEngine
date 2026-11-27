/**
 * @file Macros.inl
 * @brief Inline implementations of macros.
 */

#pragma once

#include <Maths/Macros.hpp>

inline Lumen::Bool Lumen::Maths::IsEqual ( const Float32 Fa, const Float32 Fb, const Float32 Tolerance ) noexcept
{
    return Abs( Fa - Fb ) <= Tolerance;
}

inline Lumen::Bool Lumen::Maths::IsEqual ( const Float64 Fa, const Float64 Fb, const Float64 Tolerance ) noexcept
{
    return Abs( Fa - Fb ) <= Tolerance;
}

template <typename Type> constexpr Type Lumen::Maths::Abs ( Type V ) noexcept
{
    if constexpr ( std::is_same_v<Type, Float32> )
    {
        union
        {
            Float32 F;
            UInt32 U;
        } U{ V };
        U.U &= 0x7FFFFFFFU;
        return U.F;
    }
    else
    {
        union
        {
            Float64 F;
            UInt64 U;
        } U{ V };
        U.U &= 0x7FFFFFFFFFFFFFFFULL;
        return U.F;
    }
}

template <typename Type> constexpr Type Lumen::Maths::Max ( const Type A, const Type B ) noexcept
{
    return A < B ? B : A;
}

template <typename Type> constexpr Type Lumen::Maths::Min ( const Type A, const Type B ) noexcept
{
    return B < A ? B : A;
}

template <typename Type> constexpr Type Lumen::Maths::Clamp ( const Type V, const Type Lo, const Type Hi ) noexcept
{
    return Min( Max( V, Lo ), Hi );
}
