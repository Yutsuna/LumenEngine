/**
 * @file RHIExtent.inl
 * @brief Inline implementation of RHI extents and offsets.
 */
#pragma once

#include <Types/RHIExtent.hpp>

constexpr Lumen::Bool Lumen::FRHIExtent3D::IsValid () const noexcept
{
    return Width > 0U and Height > 0U and Depth > 0U;
}

constexpr Lumen::Bool Lumen::FRHIExtent3D::Contains ( const UInt32 InX, const UInt32 InY, const UInt32 InZ ) const noexcept
{
    return InX < Width and InY < Height and InZ < Depth;
}

constexpr Lumen::Bool Lumen::FRHIOffset3D::IsNonNegative () const noexcept
{
    return X >= 0 and Y >= 0 and Z >= 0;
}

constexpr Lumen::Bool Lumen::FRHIRect2D::IsValid () const noexcept
{
    return Width > 0U and Height > 0U;
}
