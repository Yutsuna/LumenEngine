/**
 * @file VulkanCheckedConversion.cpp
 * @brief Implements non-template checked Vulkan conversions.
 */

#include <RHIVulkan/Utilities/VulkanCheckedConversion.hpp>

/**
 * Add
 */

Lumen::TExpected<Lumen::UInt64, Lumen::RHI::FRHIError> Lumen::RHIVulkan::CheckedAddUInt64 ( UInt64 InLeft,
                                                                                            UInt64 InRight,
                                                                                            RHI::ERHIErrorCondition InCondition,
                                                                                            FStringView InMessage ) noexcept
{
    if ( const TOptional<UInt64> Sum = RHI::RHICheckedAdd( InLeft, InRight ) ) [[likely]]
    {
        return *Sum;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}

/**
 * Multiply
 */

Lumen::TExpected<Lumen::UInt64, Lumen::RHI::FRHIError> Lumen::RHIVulkan::CheckedMultiplyUInt64 (
    UInt64 InLeft, UInt64 InRight, RHI::ERHIErrorCondition InCondition, FStringView InMessage ) noexcept
{
    if ( const TOptional<UInt64> Product = RHI::RHICheckedMultiply( InLeft, InRight ) ) [[likely]]
    {
        return *Product;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}

/**
 * Viewport
 */

Lumen::TExpected<VkViewport, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::ToVulkanViewport ( const RHI::FRHIViewport &InViewport ) noexcept
{
    const Bool bAllFinite = std::isfinite( InViewport.X ) and std::isfinite( InViewport.Y ) and
                            std::isfinite( InViewport.Width ) and std::isfinite( InViewport.Height ) and
                            std::isfinite( InViewport.MinDepth ) and std::isfinite( InViewport.MaxDepth );

    if ( not bAllFinite or not InViewport.IsValid() ) [[unlikely]]
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Command, "Invalid viewport dimensions or depth range." ) );
    }

    return VkViewport{
        .x        = InViewport.X,
        .y        = InViewport.Y,
        .width    = InViewport.Width,
        .height   = InViewport.Height,
        .minDepth = InViewport.MinDepth,
        .maxDepth = InViewport.MaxDepth,
    };
}
