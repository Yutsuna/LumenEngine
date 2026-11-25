/**
 * @file VulkanAdapterBuilder.hpp
 * @brief Declares construction of one baseline Vulkan adapter.
 */

#pragma once

#include <RHIVulkan/Module/VulkanAdapter.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    /** @return Portable feature flags derived from verified native features, queues, and presentation support. */
    [[nodiscard]] RHI::ERHICapabilityFlags BuildVulkanFeatureFlags ( const FVulkanAdapter &InAdapter,
                                                                     const VkPhysicalDeviceFeatures2 &InFeatures,
                                                                     const VkPhysicalDeviceLimits &InLimits,
                                                                     Bool bExactTimestampPeriod,
                                                                     Bool bGraphicsSupportsTimestamps,
                                                                     Bool bSupportsPresentation ) noexcept;

    /** @brief Builds an adapter, returning no value when the device misses the baseline. */
    [[nodiscard]] TExpected<TOptional<FVulkanAdapter>, RHI::FRHIError>
    BuildVulkanAdapter ( const FVulkanInstance &InInstance, VkPhysicalDevice InPhysicalDevice ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen
