/**
 * @file VulkanApplication.hpp
 * @brief Declares Vulkan instance creation parameters and supported features.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIError.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

#include <vulkan/vulkan_core.h>

namespace Lumen
{

namespace RHIVulkan
{

    /**
     * @struct FVulkanApplication
     * @brief Selected instance extensions, layers, and creation flags.
     */
    struct FVulkanApplication
    {
        TVector<const AnsiChar *> EnabledExtensions;
        TVector<const AnsiChar *> EnabledLayers;
        VkInstanceCreateFlags Flags = 0U;

        Bool bSurfaceEnabled                = false;
        Bool bSwapchainColorspaceEnabled    = false;
        Bool bDebugUtilsEnabled             = false;
        Bool bPortabilityEnumerationEnabled = false;

        /** @brief Validates the loader and selects available instance features. */
        [[nodiscard]] static TExpected<FVulkanApplication, RHI::FRHIError> Create ( Bool bEnableDebug ) noexcept;

        /** @return Application information for the Vulkan 1.4 device baseline. */
        [[nodiscard]] VkApplicationInfo MakeApplicationInfo () const noexcept;

        /** @return Debug messenger configuration shared by instance and messenger creation. */
        [[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerCreateInfo () const noexcept;
    };

} // namespace RHIVulkan

} // namespace Lumen
