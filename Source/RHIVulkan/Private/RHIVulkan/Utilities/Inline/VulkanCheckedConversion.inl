/**
 * @file VulkanCheckedConversion.inl
 * @brief Inline implementation of Vulkan checked conversion functions.
 */

#pragma once

#include <RHIVulkan/Utilities/VulkanCheckedConversion.hpp>

/**
 * UInt32
 */

template <typename SourceType>
    requires Lumen::Concepts::CIsIntegral<SourceType>

constexpr Lumen::TExpected<Lumen::UInt32, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::CheckedToVkUInt32 ( SourceType InValue, RHI::ERHIErrorCondition InCondition, FStringView InMessage ) noexcept
{
    if ( const TOptional<UInt32> Converted = RHI::RHICheckedCast<UInt32>( InValue ) ) [[likely]]
    {
        return *Converted;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}

/**
 * Int32
 */

template <typename SourceType>
    requires Lumen::Concepts::CIsIntegral<SourceType>

constexpr Lumen::TExpected<Lumen::Int32, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::CheckedToVkInt32 ( SourceType InValue, RHI::ERHIErrorCondition InCondition, FStringView InMessage ) noexcept
{
    if ( const TOptional<Int32> Converted = RHI::RHICheckedCast<Int32>( InValue ) ) [[likely]]
    {
        return *Converted;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}

/**
 * VkDeviceSize
 */

template <typename SourceType>
    requires Lumen::Concepts::CIsIntegral<SourceType>

constexpr Lumen::TExpected<VkDeviceSize, Lumen::RHI::FRHIError> Lumen::RHIVulkan::CheckedToVkDeviceSize (
    SourceType InValue, RHI::ERHIErrorCondition InCondition, FStringView InMessage ) noexcept
{
    if ( const TOptional<VkDeviceSize> Converted = RHI::RHICheckedCast<VkDeviceSize>( InValue ) ) [[likely]]
    {
        return *Converted;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}

/**
 * VkDeviceSize
 */

template <typename SourceType>
    requires Lumen::Concepts::CIsIntegral<SourceType>
constexpr Lumen::TExpected<Lumen::USize, Lumen::RHI::FRHIError>

Lumen::RHIVulkan::CheckedToUSize ( SourceType InValue, RHI::ERHIErrorCondition InCondition, FStringView InMessage ) noexcept
{
    if ( const TOptional<USize> Converted = RHI::RHICheckedCast<USize>( InValue ) ) [[likely]]
    {
        return *Converted;
    }
    return TUnexpected( RHI::MakeRHIInvalidArgument( InCondition, InMessage ) );
}
