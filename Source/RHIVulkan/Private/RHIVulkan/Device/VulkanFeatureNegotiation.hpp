/**
 * @file VulkanFeatureNegotiation.cpp
 * @brief Declares pre-allocation validation and native VkDevice feature negotiation.
 */

#pragma once

#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHILimits.hpp>

#include <Templates/Vector.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    struct FVulkanAdapter;

    /**
     * @struct FVulkanNegotiatedDeviceConfig
     * @brief Validated native extensions, Vulkan 1.4 feature chains, and requested queues.
     */
    struct FVulkanNegotiatedDeviceConfig
    {
        RHI::FRHICapabilities NegotiatedCapabilities;
        RHI::FRHILimits NegotiatedLimits;

        TVector<const AnsiChar *> EnabledExtensions;

        VkPhysicalDeviceFeatures2 Features2{};
        VkPhysicalDeviceVulkan12Features Features12{};
        VkPhysicalDeviceVulkan13Features Features13{};

        TOptional<UInt32> GraphicsQueueFamilyIndex;
        TOptional<UInt32> ComputeQueueFamilyIndex;
        TOptional<UInt32> CopyQueueFamilyIndex;
        TVector<UInt32> UniqueQueueFamilyIndices;
    };

    /**
     * @brief Validates InDesc against InAdapter and builds native device enablement structs.
     *
     * @param InAdapter The Vulkan adapter to use.
     * @param InDesc The RHI device description to negotiate.
     *
     * @return A TExpected containing the negotiated device config or an RHI error.
     */
    [[nodiscard]] Lumen::TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError>
    NegotiateVulkanDeviceFeatures ( const FVulkanAdapter &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen
