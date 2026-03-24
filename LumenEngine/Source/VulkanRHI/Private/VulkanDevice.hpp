/**
 * @file VulkanDevice.hpp
 * @brief Vulkan device RHI wrapper
 */

#pragma once

#include "Concepts/ConvertibleTo.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

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
     * @brief  Represents a Vulkan device and provides methods for initialization and access to the logical and physical devices.
     */
    class FVulkanDevice final
    {
    public:

        FVulkanDevice () noexcept  = default;
        ~FVulkanDevice () noexcept = default;

    public:

        [[nodiscard]] TExpected<void, FString> Initialize ( VkInstance InInstance, VkSurfaceKHR InSurface );

        [[nodiscard]] VkDevice GetLogicalDevice () const;
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice () const;

        template <Concepts::CVulkanExtension... TExtensions> void EnableExtensions ( TExtensions... InExtensions );

    private:

        void SelectPhysicalDevice ( VkInstance InInstance );
        void CreateLogicalDevice ();

    private:

        VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
        VkDevice LogicalDevice{ VK_NULL_HANDLE };
        VkQueue GraphicsQueue{ VK_NULL_HANDLE };

        TVector<const AnsiChar *> EnabledExtensions;
    };

} // namespace RHI

} // namespace LumenEngine
