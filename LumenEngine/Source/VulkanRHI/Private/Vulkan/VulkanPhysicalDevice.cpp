/**
 * @file VulkanPhysicalDevice.cpp
 * @brief Implementation of the FVulkanPhysicalDevice class.
 */

#include "Vulkan/VulkanPhysicalDevice.hpp"
#include "Vulkan/VulkanCore.hpp"

#include "Container/String.hpp"

namespace
{

/**
 * @brief Checks if the physical device supports Vulkan 1.3 features required by the engine.
 * @param InDevice The physical device to check.
 * @return True if the device supports the required Vulkan 1.3 features, false otherwise.
 */
LumenEngine::Bool HasVulkan13Features ( VkPhysicalDevice InDevice ) noexcept
{
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceFeatures2 Features2{};
    Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    Features2.pNext = &Features13;

    vkGetPhysicalDeviceFeatures2( InDevice, &Features2 );

    const LumenEngine::Bool bSupportsDynamicRendering = Features13.dynamicRendering == VK_TRUE;
    const LumenEngine::Bool bSupportsSync2            = Features13.synchronization2 == VK_TRUE;

    return bSupportsDynamicRendering && bSupportsSync2;
}

LumenEngine::Bool HasDeviceExtensions ( VkPhysicalDevice InDevice ) noexcept
{
    LumenEngine::UInt32 ExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &ExtensionCount, nullptr );

    LumenEngine::TVector<VkExtensionProperties> Extensions( ExtensionCount );
    vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &ExtensionCount, Extensions.data() );

    for ( const VkExtensionProperties &Ext : Extensions )
    {
        if ( LumenEngine::FStringView( Ext.extensionName ) == VK_KHR_SWAPCHAIN_EXTENSION_NAME )
        {
            return true;
        }
    }

    return false;
}

LumenEngine::Bool HasQueueFamilies ( const VkPhysicalDevice &InDevice, const VkSurfaceKHR &InSurface )
{
    LumenEngine::UInt32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( InDevice, &QueueFamilyCount, nullptr );

    LumenEngine::TVector<VkQueueFamilyProperties> QueueFamilies( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( InDevice, &QueueFamilyCount, QueueFamilies.data() );

    for ( LumenEngine::UInt32 Index = 0; Index < QueueFamilyCount; ++Index )
    {
        const VkQueueFamilyProperties &Queue      = QueueFamilies[Index];
        const LumenEngine::Bool bSupportsGraphics = ( Queue.queueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0;

        if ( not bSupportsGraphics )
        {
            continue;
        }

        VkBool32 bSupportsPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( InDevice, Index, InSurface, &bSupportsPresent );

        if ( bSupportsPresent == VK_TRUE )
        {
            return true;
        }
    }

    return false;
}

LumenEngine::Bool IsDeviceSuitable ( const VkPhysicalDevice &InDevice, const VkSurfaceKHR &InSurface ) noexcept
{
    return HasVulkan13Features( InDevice ) && HasDeviceExtensions( InDevice ) && HasQueueFamilies( InDevice, InSurface );
}

VkPhysicalDevice SelectBestDevice ( const LumenEngine::TVector<VkPhysicalDevice> &Devices, VkSurfaceKHR InSurface ) noexcept
{
    VkPhysicalDevice FallbackDevice = VK_NULL_HANDLE;

    for ( const VkPhysicalDevice &Device : Devices )
    {
        if ( not IsDeviceSuitable( Device, InSurface ) )
        {
            continue;
        }

        VkPhysicalDeviceProperties Props{};
        vkGetPhysicalDeviceProperties( Device, &Props );

        if ( Props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        {
            LUMEN_LOG_INFO( LumenEngine::VulkanRHI::LogVulkanRHI, "Selected Discrete GPU: {}", Props.deviceName );
            return Device;
        }

        if ( FallbackDevice == VK_NULL_HANDLE )
        {
            FallbackDevice = Device;
        }
    }

    if ( FallbackDevice != VK_NULL_HANDLE )
    {
        VkPhysicalDeviceProperties Props{};
        vkGetPhysicalDeviceProperties( FallbackDevice, &Props );

        LUMEN_LOG_INFO( LumenEngine::VulkanRHI::LogVulkanRHI, "Selected Fallback GPU: {}", Props.deviceName );
    }

    return FallbackDevice;
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanPhysicalDevice::Initialize ( VkInstance InInstance, VkSurfaceKHR InSurface )
{
    UInt32 DeviceCount = 0;
    vkEnumeratePhysicalDevices( InInstance, &DeviceCount, nullptr );

    if ( DeviceCount == 0 )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find GPUs with Vulkan support." );
    }

    TVector<VkPhysicalDevice> Devices( DeviceCount );
    vkEnumeratePhysicalDevices( InInstance, &DeviceCount, Devices.data() );

    PhysicalDevice = SelectBestDevice( Devices, InSurface );

    if ( PhysicalDevice == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find a suitable GPU that supports Vulkan 1.3 and required features." );
    }
}

VkPhysicalDevice LumenEngine::VulkanRHI::FVulkanPhysicalDevice::GetHandle () const noexcept
{
    return PhysicalDevice;
}
