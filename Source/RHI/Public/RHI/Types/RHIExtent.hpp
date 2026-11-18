/**
 * @file RHIExtent.hpp
 * @brief Defines portable dimensions, offsets, and rectangles.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Maths/Vec.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIExtent3D
     * @brief Width, height, and depth of a resource or operation.
     */
    struct FRHIExtent3D : public Maths::FVec3u
    {
        constexpr FRHIExtent3D () noexcept;
        constexpr FRHIExtent3D ( UInt32 InWidth, UInt32 InHeight, UInt32 InDepth ) noexcept;

        /** @return True when all dimensions are nonzero. */
        [[nodiscard]] constexpr Bool IsValid () const noexcept;

        /** @return True when InPoint lies inside this extent. */
        [[nodiscard]] constexpr Bool Contains ( UInt32 InX, UInt32 InY, UInt32 InZ ) const noexcept;

        /** @return Number of mip levels this extent can have (at least one, at most 32 for a UInt32 extent). */
        [[nodiscard]] constexpr UInt32 GetMaxMipLevelCount () const noexcept;
    };

    /**
     * @struct FRHIOffset3D
     * @brief Signed origin of a copy or resource operation.
     */
    struct FRHIOffset3D : public Maths::FVec3i
    {
        constexpr FRHIOffset3D () noexcept;
        constexpr FRHIOffset3D ( Int32 InX, Int32 InY, Int32 InZ ) noexcept;

        /** @return True when every component is nonnegative. */
        [[nodiscard]] constexpr Bool IsNonNegative () const noexcept;
    };

    /**
     * @struct FRHIRect2D
     * @brief Signed two-dimensional scissor or render rectangle.
     */
    struct FRHIRect2D
    {
        Maths::TVec<Int32, 2> Position = { 0 };
        Maths::TVec<UInt32, 2> Size    = { 0 };

        constexpr FRHIRect2D () noexcept;
        constexpr FRHIRect2D ( Int32 InX, Int32 InY, UInt32 InWidth, UInt32 InHeight ) noexcept;

        /** @return True when width and height are nonzero. */
        [[nodiscard]] constexpr Bool IsValid () const noexcept;
    };

} // namespace RHI

} // namespace Lumen

#include "Inline/RHIExtent.inl"
