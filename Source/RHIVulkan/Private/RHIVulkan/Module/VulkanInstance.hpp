/**
 * @file VulkanInstance.hpp
 * @brief Declares Vulkan instance, layer/extension discovery, and debug messenger ownership.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Types/RHIError.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @class FVulkanInstance
     * @brief Owns a Vulkan 1.4 VkInstance and optional debug-utils messenger.
     */
    class FVulkanInstance final : public FNonCopyable, public FNonMovable
    {
    public:

        FVulkanInstance () noexcept  = default;
        ~FVulkanInstance () noexcept = default;

    public:

        /** @brief Creates a Vulkan 1.4 instance with portability and optional debug support. */
        [[nodiscard]] static TExpected<TSharedRef<FVulkanInstance>, RHI::FRHIError> Create ( Bool bEnableDebug ) noexcept;

    public:

        /** @return Native VkInstance handle. */
        [[nodiscard]] VkInstance GetHandle () const noexcept;

        /** @return True when a surface extension was enabled on this instance. */
        [[nodiscard]] Bool SupportsSurface () const noexcept;

        /** @return True when VK_EXT_swapchain_colorspace was enabled. */
        [[nodiscard]] Bool SupportsSwapchainColorspace () const noexcept;

        /** @return True when VK_EXT_debug_utils was enabled. */
        [[nodiscard]] Bool SupportsDebugUtils () const noexcept;

        /** @return True when VK_KHR_portability_enumeration was enabled. */
        [[nodiscard]] Bool SupportsPortabilityEnumeration () const noexcept;

    private:

        FVulkanInstanceHandle Instance;
        FVulkanDebugMessengerHandle DebugMessenger;

        Bool bSurfaceEnabled                = false;
        Bool bSwapchainColorspaceEnabled    = false;
        Bool bDebugUtilsEnabled             = false;
        Bool bPortabilityEnumerationEnabled = false;
    };

} // namespace RHIVulkan

} // namespace Lumen
