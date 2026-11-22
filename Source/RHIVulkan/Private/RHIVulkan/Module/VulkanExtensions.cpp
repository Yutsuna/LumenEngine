/**
 * @file VulkanExtensions.cpp
 * @brief Implements Vulkan extension and layer discovery helpers.
 */

#include <RHIVulkan/Module/VulkanExtensions.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <Algo/Any.hpp>

#include <cstring>
#include <utility>

/**
 * Static
 */

Lumen::TExpected<Lumen::RHIVulkan::FVulkanExtensions, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanExtensions::Enumerate () noexcept
{
    FVulkanExtensions Available;

    LUMEN_EXPECT( InstanceExtensions,
                  LUMEN_VK_ENUMERATE( VkExtensionProperties, vkEnumerateInstanceExtensionProperties, nullptr ) );

    Available.Extensions = std::move( InstanceExtensions );

    LUMEN_EXPECT( InstanceLayers, LUMEN_VK_ENUMERATE( VkLayerProperties, vkEnumerateInstanceLayerProperties ) );

    Available.Layers = std::move( InstanceLayers );

    return Available;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanExtensions, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanExtensions::Enumerate ( VkPhysicalDevice InPhysicalDevice ) noexcept
{
    FVulkanExtensions Available;

    LUMEN_EXPECT( DeviceExtensions,
                  LUMEN_VK_ENUMERATE( VkExtensionProperties, vkEnumerateDeviceExtensionProperties, InPhysicalDevice, nullptr ) );
    Available.Extensions = std::move( DeviceExtensions );

    return Available;
}

/**
 * Public
 */

Lumen::Bool Lumen::RHIVulkan::FVulkanExtensions::HasExtension ( const AnsiChar *const InName ) const noexcept
{
    return Algo::AnyOf( Extensions, [InName] ( const VkExtensionProperties &Extension ) noexcept -> Bool
    {
        return std::strcmp( Extension.extensionName, InName ) == 0;
    } );
}

Lumen::Bool Lumen::RHIVulkan::FVulkanExtensions::HasLayer ( const AnsiChar *const InName ) const noexcept
{
    return Algo::AnyOf( Layers, [InName] ( const VkLayerProperties &Layer ) noexcept -> Bool
    {
        return std::strcmp( Layer.layerName, InName ) == 0;
    } );
}
