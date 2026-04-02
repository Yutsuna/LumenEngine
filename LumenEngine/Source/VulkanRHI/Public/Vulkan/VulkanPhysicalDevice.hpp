/**
 * @file VulkanPhysicalDevice.hpp
 * @brief Encapsulates Vulkan physical device selection and properties.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanPhysicalDevice
     * @brief Encapsulates the selection of the best physical device (GPU).
     */
    class LUMEN_ENGINE_API FVulkanPhysicalDevice final
    {
    public:

        FVulkanPhysicalDevice () noexcept  = default;
        ~FVulkanPhysicalDevice () noexcept = default;

    public:

        /** Selects the most suitable physical device. */
        void Initialize ( VkInstance InInstance, VkSurfaceKHR InSurface );

    public:

        [[nodiscard]] VkPhysicalDevice GetHandle () const noexcept;

    private:

        Bool IsDeviceSuitable ( VkPhysicalDevice InDevice, VkSurfaceKHR InSurface ) const;

    private:

        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
