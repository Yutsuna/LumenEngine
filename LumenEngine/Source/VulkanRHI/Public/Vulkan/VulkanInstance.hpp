/**
 * @file VulkanInstance.hpp
 * @brief Declaration of the FVulkanInstance class.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

class FGenericWindow;

namespace VulkanRHI
{

    /**
     * @class FVulkanInstance
     * @brief Encapsulates the Vulkan Instance and Window Surface.
     */
    class LUMEN_ENGINE_API FVulkanInstance final
    {
    public:

        FVulkanInstance () noexcept  = default;
        ~FVulkanInstance () noexcept = default;

    public:

        /** Initializes the Vulkan instance and creates the window surface. */
        void Initialize ( const TSharedPtr<FGenericWindow> &InWindow );

        /** Cleans up the instance and surface. */
        void Cleanup () noexcept;

    public:

        [[nodiscard]] VkInstance GetHandle () const noexcept;
        [[nodiscard]] VkSurfaceKHR GetSurface () const noexcept;

    private:

        void CreateInstance ( VkInstanceCreateInfo &CreateInfo, const TSharedPtr<FGenericWindow> &InWindow );

    private:

        VkInstance Instance  = VK_NULL_HANDLE;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
