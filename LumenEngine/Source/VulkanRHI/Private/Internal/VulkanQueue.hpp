/**
 * @file VulkanQueue.hpp
 * @brief Vulkan Queue RHI wrapper
 */

#pragma once

#include "CoreTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class FVulkanQueue
     * @brief Wrapper for VkQueue, managing its handle and family index.
     */
    class FVulkanQueue final
    {
    public:

        FVulkanQueue () noexcept;
        FVulkanQueue ( VkQueue InQueue, UInt32 InFamilyIndex, UInt32 InQueueIndex ) noexcept;

        ~FVulkanQueue () noexcept;

    public:

        /** @return The underlying Vulkan queue handle */
        [[nodiscard]] inline VkQueue GetHandle () const noexcept;

        /** @return The queue family index */
        [[nodiscard]] inline UInt32 GetFamilyIndex () const noexcept;

        /** @return The index of the queue within its family */
        [[nodiscard]] inline UInt32 GetQueueIndex () const noexcept;

    private:

        /** The Vulkan queue handle */
        VkQueue Queue{ VK_NULL_HANDLE };

        /** The queue family index */
        UInt32 FamilyIndex{ 0 };

        /** The index of the queue within its family */
        UInt32 QueueIndex{ 0 };
    };

} // namespace RHI

} // namespace LumenEngine

#include "VulkanQueue.inl"
