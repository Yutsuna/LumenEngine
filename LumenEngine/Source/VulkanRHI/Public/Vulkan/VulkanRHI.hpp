/**
 * @file VulkanRHI.hpp
 * @brief Main header file for the Vulkan RHI module
 */

#pragma once

#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "VulkanCore.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanDevice;
    class FVulkanResource;

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

        /** @return Singleton instance of the RHI */
        static FVulkanRHI &Get ();

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
         * @brief Begins a new frame for rendering
         */
        void RHIBeginFrame ();

        /**
         * @brief Ends the current frame and presents the rendered image to the screen
         */
        void RHIEndFrame ();

    public:

        /** @return The underlying Vulkan device wrapper */
        FVulkanDevice &GetDevice () noexcept;

        /**
         * @brief Enqueues a resource for deferred deletion
         * @param Resource The resource to be deleted after MAX_FRAMES_IN_FLIGHT
         */
        void DeferredDeletion ( FVulkanResource *Resource );

    private:

        /** Processes the deferred deletion queue and releases resources that are safe to destroy */
        void TickDeferredDeletion ();

        TExpected<void, FString> CreateInstance ();
        TExpected<void, FString> CreateSurface ( void *InWindowHandle );

    private:

        /** Vulkan instance */
        VkInstance Instance{ VK_NULL_HANDLE };

        /** Vulkan surface */
        VkSurfaceKHR Surface{ VK_NULL_HANDLE };

        /** Logical and physical device wrapper */
        TUniquePtr<FVulkanDevice> Device;

        /** Queue of resources pending deletion, grouped by frame index */
        TVector<FVulkanResource *> PendingDeletionQueue;

#if !defined( NDEBUG )
        /** Debug messenger for validation layers */
        VkDebugUtilsMessengerEXT DebugMessenger{ VK_NULL_HANDLE };
#endif
    };

} // namespace RHI

} // namespace LumenEngine
