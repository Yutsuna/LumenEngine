/**
 * @file VulkanRHI.hpp
 * @brief Main header file for the Vulkan RHI module
 */

#pragma once

#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"

#include "CoreTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanDevice;

    /**
     * @class FVulkanRHI
     * @brief Main class for the Vulkan RHI, responsible for initializing and managing the Vulkan rendering context
     */
    class FVulkanRHI final
    {
    public:

        FVulkanRHI () noexcept;
        ~FVulkanRHI () noexcept;

    public:

        /**
         * @brief Initializes the Vulkan RHI with the provided window handle
         * @param InWindowHandle A pointer to the window handle (platform-specific)
         * @return A TExpected indicating success or failure of the initialization process
         */
        TExpected<void, FString> Initialize ( void *InWindowHandle );

        /**
         * @brief Performs post-initialization tasks after the Vulkan context has been created
         */

        void PostInitialize () noexcept;

        /**
         * @brief Shuts down the Vulkan RHI and releases all associated resources
         */
        void Shutdown () noexcept;

        /**
         * @brief Begins a new frame for rendering, preparing the Vulkan command buffers and synchronization primitives
         */
        void RHIBeginFrame ();

        /**
         * @brief Ends the current frame and presents the rendered image to the screen
         */
        void RHIEndFrame ();

    private:

        TExpected<void, FString> CreateInstance ();
        TExpected<void, FString> CreateSurface ( void *InWindowHandle );

    private:

        VkInstance Instance{ VK_NULL_HANDLE };
        VkSurfaceKHR Surface{ VK_NULL_HANDLE };

        TUniquePtr<FVulkanDevice> Device;

#if !defined( NDEBUG )
        VkDebugUtilsMessengerEXT DebugMessenger{ VK_NULL_HANDLE };
#endif
    };

} // namespace RHI

} // namespace LumenEngine
