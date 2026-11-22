/**
 * @file VulkanExtensions.hpp
 * @brief Declares an owned snapshot of Vulkan extensions and layers.
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
     * @class FVulkanExtensions
     * @brief Owns discovered Vulkan properties and provides lookup operations.
     */
    class FVulkanExtensions
    {
    public:

        /** @brief Enumerates instance extensions and layers. */
        [[nodiscard]] static TExpected<FVulkanExtensions, RHI::FRHIError> Enumerate () noexcept;

        /** @brief Enumerates extensions exposed by a physical device. */
        [[nodiscard]] static TExpected<FVulkanExtensions, RHI::FRHIError>
        Enumerate ( VkPhysicalDevice InPhysicalDevice ) noexcept;

    public:

        /** @return True when InName is present in the extension snapshot. */
        [[nodiscard]] Bool HasExtension ( const AnsiChar *InName ) const noexcept;

        /** @return True when InName is present in the layer snapshot. */
        [[nodiscard]] Bool HasLayer ( const AnsiChar *InName ) const noexcept;

    private:

        FVulkanExtensions () noexcept = default;

    private:

        TVector<VkExtensionProperties> Extensions;
        TVector<VkLayerProperties> Layers;
    };

} // namespace RHIVulkan

} // namespace Lumen
