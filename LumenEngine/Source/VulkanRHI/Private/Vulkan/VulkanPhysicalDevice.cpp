/**
 * @file VulkanPhysicalDevice.cpp
 * @brief Implementation of the FVulkanPhysicalDevice class.
 */

#include "Vulkan/VulkanPhysicalDevice.hpp"
#include "Vulkan/VulkanCore.hpp"

#include "Container/String.hpp"
#include "Container/Vector.hpp"

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

    // On cherche d'abord un GPU dédié (Discrete GPU)
    for ( const auto &Device : Devices )
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties( Device, &Properties );

        if ( Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && IsDeviceSuitable( Device, InSurface ) )
        {
            PhysicalDevice = Device;
            LUMEN_LOG_INFO( LogVulkanRHI, "Selected Discrete GPU: {}", Properties.deviceName );
            break;
        }
    }

    // Sinon, on prend le premier GPU compatible (ex: GPU intégré)
    if ( PhysicalDevice == VK_NULL_HANDLE )
    {
        for ( const auto &Device : Devices )
        {
            if ( IsDeviceSuitable( Device, InSurface ) )
            {
                PhysicalDevice = Device;
                VkPhysicalDeviceProperties Properties;
                vkGetPhysicalDeviceProperties( Device, &Properties );
                LUMEN_LOG_INFO( LogVulkanRHI, "Selected Integrated/Fallback GPU: {}", Properties.deviceName );
                break;
            }
        }
    }

    if ( PhysicalDevice == VK_NULL_HANDLE )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find a suitable GPU that supports Vulkan 1.3 and required features." );
    }
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanPhysicalDevice::IsDeviceSuitable ( VkPhysicalDevice InDevice, VkSurfaceKHR InSurface ) const
{
    // 1. Check Vulkan 1.3 Features (Dynamic Rendering & Sync 2)
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceFeatures2 Features2{};
    Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    Features2.pNext = &Features13;

    vkGetPhysicalDeviceFeatures2( InDevice, &Features2 );

    if ( Features13.dynamicRendering == VK_FALSE || Features13.synchronization2 == VK_FALSE )
    {
        return false;
    }

    // 2. Check Extension Support (VK_KHR_swapchain)
    UInt32 ExtensionCount;
    vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &ExtensionCount, nullptr );
    TVector<VkExtensionProperties> AvailableExtensions( ExtensionCount );
    vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &ExtensionCount, AvailableExtensions.data() );

    bool bHasSwapchain = false;
    for ( const auto &Ext : AvailableExtensions )
    {
        if ( FString( Ext.extensionName ) == VK_KHR_SWAPCHAIN_EXTENSION_NAME )
        {
            bHasSwapchain = true;
            break;
        }
    }

    if ( not bHasSwapchain )
    {
        return false;
    }

    // 3. Check Queue Support (Graphics & Present)
    UInt32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( InDevice, &QueueFamilyCount, nullptr );
    TVector<VkQueueFamilyProperties> QueueFamilies( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( InDevice, &QueueFamilyCount, QueueFamilies.data() );

    bool bHasGraphicsAndPresent = false;
    for ( UInt32 i = 0; i < QueueFamilyCount; ++i )
    {
        if ( QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            VkBool32 bPresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( InDevice, i, InSurface, &bPresentSupport );
            if ( bPresentSupport )
            {
                bHasGraphicsAndPresent = true;
                break;
            }
        }
    }

    return bHasGraphicsAndPresent;
}

VkPhysicalDevice LumenEngine::VulkanRHI::FVulkanPhysicalDevice::GetHandle () const noexcept
{
    return PhysicalDevice;
}
