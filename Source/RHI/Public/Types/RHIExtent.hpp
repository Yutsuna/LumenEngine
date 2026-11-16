/**
 * @file RHIExtent.hpp
 * @brief Defines portable dimensions, offsets, and rectangles.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <compare>

namespace Lumen
{

/**
 * @struct FRHIExtent3D
 * @brief Width, height, and depth of a resource or operation.
 */
struct FRHIExtent3D
{
    UInt32 Width  = 1U;
    UInt32 Height = 1U;
    UInt32 Depth  = 1U;

    /** @return True when all dimensions are nonzero. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;
    /** @return True when InPoint lies inside this extent. */
    [[nodiscard]] constexpr Bool Contains ( UInt32 InX, UInt32 InY, UInt32 InZ ) const noexcept;
    friend constexpr auto operator<=>( const FRHIExtent3D &, const FRHIExtent3D & ) = default;
};

/**
 * @struct FRHIOffset3D
 * @brief Signed origin of a copy or resource operation.
 */
struct FRHIOffset3D
{
    Int32 X = 0;
    Int32 Y = 0;
    Int32 Z = 0;

    /** @return True when every component is nonnegative. */
    [[nodiscard]] constexpr Bool IsNonNegative () const noexcept;
    friend constexpr auto operator<=>( const FRHIOffset3D &, const FRHIOffset3D & ) = default;
};

/**
 * @struct FRHIRect2D
 * @brief Signed two-dimensional scissor or render rectangle.
 */
struct FRHIRect2D
{
    Int32 X       = 0;
    Int32 Y       = 0;
    UInt32 Width  = 0U;
    UInt32 Height = 0U;

    /** @return True when width and height are nonzero. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;
    friend constexpr auto operator<=>( const FRHIRect2D &, const FRHIRect2D & ) = default;
};

} // namespace Lumen

#include "Inline/RHIExtent.inl"
