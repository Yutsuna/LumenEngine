/**
 * @file VulkanCheckedConversion.hpp
 * @brief Declares overflow-checked conversions between RHI and Vulkan scalar types.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/IsIntegral.hpp>

#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIViewport.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

#include <String/StringView.hpp>
#include <Templates/Expected.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /** @brief Converts an integral value to Vulkan UInt32 with overflow checking. */
    template <class TIntegral>
        requires Concepts::CIsIntegral<TIntegral>
    [[nodiscard]] constexpr TExpected<UInt32, RHI::FRHIError>
    CheckedToVkUInt32 ( TIntegral InValue,
                        RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Limit,
                        FStringView InMessage               = "Value exceeds Vulkan UInt32 range." ) noexcept;

    /** @brief Converts an integral value to Vulkan Int32 with range checking. */
    template <class TIntegral>
        requires Concepts::CIsIntegral<TIntegral>
    [[nodiscard]] constexpr TExpected<Int32, RHI::FRHIError>
    CheckedToVkInt32 ( TIntegral InValue,
                       RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Limit,
                       FStringView InMessage               = "Value exceeds Vulkan Int32 range." ) noexcept;

    /** @brief Converts an integral value to VkDeviceSize with range checking. */
    template <class TIntegral>
        requires Concepts::CIsIntegral<TIntegral>
    [[nodiscard]] constexpr TExpected<VkDeviceSize, RHI::FRHIError>
    CheckedToVkDeviceSize ( TIntegral InValue,
                            RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Memory,
                            FStringView InMessage               = "Value exceeds VkDeviceSize range." ) noexcept;

    /** @brief Converts an integral value to USize with range checking. */
    template <class TIntegral>
        requires Concepts::CIsIntegral<TIntegral>
    [[nodiscard]] constexpr TExpected<USize, RHI::FRHIError>
    CheckedToUSize ( TIntegral InValue,
                     RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Limit,
                     FStringView InMessage               = "Value exceeds host USize range." ) noexcept;

    /** @brief Adds two UInt64 values and fails with InvalidArgument on overflow. */
    [[nodiscard]] TExpected<UInt64, RHI::FRHIError>
    CheckedAddUInt64 ( UInt64 InLeft,
                       UInt64 InRight,
                       RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Limit,
                       FStringView InMessage               = "UInt64 addition overflow." ) noexcept;

    /** @brief Multiplies two UInt64 values and fails with InvalidArgument on overflow. */
    [[nodiscard]] TExpected<UInt64, RHI::FRHIError>
    CheckedMultiplyUInt64 ( UInt64 InLeft,
                            UInt64 InRight,
                            RHI::ERHIErrorCondition InCondition = RHI::ERHIErrorCondition::Limit,
                            FStringView InMessage               = "UInt64 multiplication overflow." ) noexcept;

    /** @brief Validates and converts an RHI viewport to a Vulkan viewport. */
    [[nodiscard]] TExpected<VkViewport, RHI::FRHIError> ToVulkanViewport ( const RHI::FRHIViewport &InViewport ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen

#include "Inline/VulkanCheckedConversion.inl"
