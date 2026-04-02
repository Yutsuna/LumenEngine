/**
 * @file VulkanLogicalDevice.hpp
 * @brief Declaration of the VulkanLogicalDevice class, which encapsulates Vulkan device management.
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Vulkan/VulkanCore.hpp"

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanLogicalDevice
     * @brief Encapsulates the Vulkan logical device and its associated resources.
     */
    class LUMEN_ENGINE_API FVulkanLogicalDevice final
    {
    public:

        FVulkanLogicalDevice () noexcept  = default;
        ~FVulkanLogicalDevice () noexcept = default;

    public:

        /**
         * @brief Initializes the Vulkan device natively.
         * @param InPhysicalDevice The selected physical device.
         * @param InSurface The window surface (needed to check presentation support).
         * @param InRequiredExtensions Extensions to enable (e.g. VK_KHR_swapchain).
         */
        void Initialize ( VkPhysicalDevice InPhysicalDevice, VkSurfaceKHR InSurface, const TVector<const AnsiChar *> &InRequiredExtensions );

        /** @brief Cleans up the Vulkan device. */
        void Cleanup () noexcept;

        /** @brief Waits for the device to become idle, ensuring all operations have completed. */
        void WaitIdle () const noexcept;

    public:

        [[nodiscard]] VkDevice GetHandle () const noexcept;
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice () const noexcept;
        [[nodiscard]] VkQueue GetGraphicsQueue () const noexcept;
        [[nodiscard]] UInt32 GetGraphicsQueueFamily () const noexcept;

    private:

        void SelectQueueFamilies ( VkSurfaceKHR InSurface );
        void CreateLogicalDevice ( const TVector<const AnsiChar *> &InRequiredExtensions );

    private:

        VkDevice Device                 = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

        VkQueue GraphicsQueue      = VK_NULL_HANDLE;
        UInt32 GraphicsQueueFamily = NullBindlessID;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
