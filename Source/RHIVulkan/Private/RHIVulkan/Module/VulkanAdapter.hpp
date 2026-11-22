/**
 * @file VulkanAdapter.hpp
 * @brief Declares physical-device discovery and Vulkan 1.3 baseline evaluation.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Types/RHIError.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    class FVulkanInstance;

    /**
     * @struct FVulkanAdapter
     * @brief Portable adapter metadata paired with verified native baseline capabilities.
     */
    struct FVulkanAdapter
    {
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        RHI::FRHIAdapterInfo Info;

        TOptional<UInt32> GraphicsQueueFamilyIndex;
        TOptional<UInt32> ComputeQueueFamilyIndex;
        TOptional<UInt32> CopyQueueFamilyIndex;

        Bool bHasPortabilitySubset = false;
        Bool bHasSwapchain         = false;
        Bool bHasBindlessIndexing  = false;
    };

    /** @brief Enumerates physical devices satisfying the Vulkan 1.3 baseline. */
    [[nodiscard]] TExpected<TVector<FVulkanAdapter>, RHI::FRHIError>
    EnumerateVulkanAdapters ( const FVulkanInstance &InInstance ) noexcept;

    /** @brief Selects a matching adapter by ID, or picks the preferred baseline adapter. */
    [[nodiscard]] TExpected<FVulkanAdapter, RHI::FRHIError> SelectVulkanAdapter ( const FVulkanInstance &InInstance,
                                                                                  TOptional<UInt64> InAdapterId ) noexcept;

} // namespace RHIVulkan

} // namespace Lumen
