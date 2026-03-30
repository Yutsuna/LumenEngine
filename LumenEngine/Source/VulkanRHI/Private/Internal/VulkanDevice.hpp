/**
 * @file VulkanDevice.hpp
 * @brief Vulkan device RHI wrapper
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "VulkanQueue.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    namespace Concepts
    {

        /**
         * @concept CVulkanExtension
         * @brief Checks if one type is convertible to a Vulkan extension name
         * @param TExtension The type to check for Vulkan extension compatibility
         */
        template <typename TExtension>
        concept CVulkanExtension = ::LumenEngine::Concepts::CConvertibleTo<TExtension, const AnsiChar *>;

    } // namespace Concepts

    /**
     * @class FVulkanDevice
     * @brief Represents a Vulkan device and provides methods for initialization and access to the logical and physical devices.
     */
    class FVulkanDevice final
    {
    public:

        FVulkanDevice () noexcept;
        ~FVulkanDevice () noexcept;

    public:

        /**
         * @brief Initializes the Vulkan device
         * @param InInstance Vulkan instance
         * @param InSurface Surface for presentation
         * @return TExpected indicating success or failure
         */
        [[nodiscard]] TExpected<void, FString> Initialize ( VkInstance InInstance, VkSurfaceKHR InSurface );

        /** @return The underlying Vulkan logical device handle */
        [[nodiscard]] VkDevice GetLogicalDevice () const noexcept;

        /** @return The underlying Vulkan physical device handle */
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice () const noexcept;

        /**
         * @brief Enables Vulkan extensions
         * @tparam TExtensions Types convertible to const AnsiChar*
         * @param InExtensions Extensions to enable
         */
        template <Concepts::CVulkanExtension... TExtensions> void EnableExtensions ( TExtensions... InExtensions );

    public:

        /** @return The graphics queue wrapper */
        [[nodiscard]] const FVulkanQueue &GetGraphicsQueue () const noexcept;

        /** @return The compute queue wrapper */
        [[nodiscard]] const FVulkanQueue &GetComputeQueue () const noexcept;

        /** @return The transfer queue wrapper */
        [[nodiscard]] const FVulkanQueue &GetTransferQueue () const noexcept;

        /** @return The presentation queue wrapper */
        [[nodiscard]] const FVulkanQueue &GetPresentQueue () const noexcept;

    private:

        /** Selects a suitable physical device */
        void SelectPhysicalDevice ( VkInstance InInstance, VkSurfaceKHR InSurface );

        /** Creates the logical device and retrieves queues */
        TExpected<void, FString> CreateLogicalDevice ( VkSurfaceKHR InSurface );

    private:

        /** Selected physical device */
        VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };

        /** Created logical device */
        VkDevice LogicalDevice{ VK_NULL_HANDLE };

        /** Graphics queue */
        FVulkanQueue GraphicsQueue;

        /** Compute queue */
        FVulkanQueue ComputeQueue;

        /** Transfer queue */
        FVulkanQueue TransferQueue;

        /** Present queue */
        FVulkanQueue PresentQueue;

        /** List of enabled extensions */
        TVector<const AnsiChar *> EnabledExtensions;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/VulkanDevice.inl"
