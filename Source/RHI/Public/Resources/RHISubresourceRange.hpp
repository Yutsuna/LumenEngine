/**
 * @file RHISubresourceRange.hpp
 * @brief Defines texture mip and array-layer ranges.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @struct FRHISubresourceRange
 * @brief Half-open mip and layer range.
 */
struct RHI_EXPORT FRHISubresourceRange
{
    UInt32 BaseMip    = 0U;
    UInt32 MipCount   = 1U;
    UInt32 BaseLayer  = 0U;
    UInt32 LayerCount = 1U;

    /** @return True when both counts are nonzero. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept;

    /** @return True when this range lies inside the supplied mip/layer counts. */
    [[nodiscard]] Bool IsContainedIn ( UInt32 InMipCount, UInt32 InLayerCount ) const noexcept;

    /** @return The intersection, or empty when disjoint. */
    [[nodiscard]] TOptional<FRHISubresourceRange> Intersect ( const FRHISubresourceRange &InOther ) const noexcept;
};

} // namespace Lumen

#include "Inline/RHISubresourceRange.inl"
