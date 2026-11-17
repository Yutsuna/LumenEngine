/**
 * @file RHIExtent.inl
 * @brief Inline implementation of RHI extents and offsets.
 */

#pragma once

#include <Types/RHIExtent.hpp>

/**
 * Extent3D
 */

constexpr Lumen::FRHIExtent3D::FRHIExtent3D () noexcept : TVec( 1U, 1U, 1U )
{
    /* */
}

constexpr Lumen::FRHIExtent3D::FRHIExtent3D ( UInt32 InWidth, UInt32 InHeight, UInt32 InDepth ) noexcept
    : TVec( InWidth, InHeight, InDepth )
{
    /* */
}

constexpr Lumen::Bool Lumen::FRHIExtent3D::IsValid () const noexcept
{
    return Width > 0U and Height > 0U and Depth > 0U;
}

/**
 * Offset3D
 */

constexpr Lumen::FRHIOffset3D::FRHIOffset3D () noexcept = default;

constexpr Lumen::FRHIOffset3D::FRHIOffset3D ( Int32 InX, Int32 InY, Int32 InZ ) noexcept : TVec( InX, InY, InZ )
{
}

constexpr Lumen::Bool Lumen::FRHIOffset3D::IsNonNegative () const noexcept
{
    return X >= 0 and Y >= 0 and Z >= 0;
}

/**
 * Rect2D
 */

constexpr Lumen::FRHIRect2D::FRHIRect2D () noexcept = default;

constexpr Lumen::FRHIRect2D::FRHIRect2D ( Int32 InX, Int32 InY, UInt32 InWidth, UInt32 InHeight ) noexcept
    : Position( InX, InY ), Size( InWidth, InHeight )
{
    /* */
}

constexpr Lumen::Bool Lumen::FRHIRect2D::IsValid () const noexcept
{
    return Size.X != 0 && Size.Y != 0;
}
