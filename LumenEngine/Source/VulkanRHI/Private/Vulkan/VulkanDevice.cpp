/**
 * @file VulkanDevice.cpp
 * @brief Implementation of the VulkanDevice class.
 */

#include "Vulkan/VulkanDevice.hpp"

void LumenEngine::VulkanRHI::FVulkanDevice::Initialize ( VkPhysicalDevice InPhysicalDevice,
                                                         VkSurfaceKHR InSurface,
                                                         const TVector<const AnsiChar *> &InRequiredExtensions )
{
    PhysicalDevice = InPhysicalDevice;

    SelectQueueFamilies( InSurface );
    CreateLogicalDevice( InRequiredExtensions );

    vkGetDeviceQueue( Device, GraphicsQueueFamily, 0, &GraphicsQueue );

    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan Logical Device created successfully." );
}

void LumenEngine::VulkanRHI::FVulkanDevice::Cleanup () noexcept
{
    if ( Device != VK_NULL_HANDLE )
    {
        vkDestroyDevice( Device, nullptr );
        Device = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanDevice::WaitIdle () const noexcept
{
    if ( Device != VK_NULL_HANDLE )
    {
        vkDeviceWaitIdle( Device );
    }
}

VkDevice LumenEngine::VulkanRHI::FVulkanDevice::GetHandle () const noexcept
{
    return Device;
}

VkPhysicalDevice LumenEngine::VulkanRHI::FVulkanDevice::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice;
}

VkQueue LumenEngine::VulkanRHI::FVulkanDevice::GetGraphicsQueue () const noexcept
{
    return GraphicsQueue;
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanDevice::GetGraphicsQueueFamily () const noexcept
{
    return GraphicsQueueFamily;
}

void LumenEngine::VulkanRHI::FVulkanDevice::SelectQueueFamilies ( VkSurfaceKHR InSurface )
{
    UInt32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &QueueFamilyCount, nullptr );

    TVector<VkQueueFamilyProperties> QueueFamilies( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &QueueFamilyCount, QueueFamilies.data() );

    for ( UInt32 Index = 0; Index < QueueFamilyCount; ++Index )
    {
        if ( QueueFamilies[Index].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            VkBool32 bPresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( PhysicalDevice, Index, InSurface, &bPresentSupport );

            if ( bPresentSupport == VK_TRUE )
            {
                GraphicsQueueFamily = Index;
                break;
            }
        }
    }

    if ( GraphicsQueueFamily == NullBindlessID )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to find a suitable queue family (Graphics + Present)." );
    }
}

void LumenEngine::VulkanRHI::FVulkanDevice::CreateLogicalDevice ( const TVector<const AnsiChar *> &InRequiredExtensions )
{
    Float32 QueuePriority = 1.0F;

    VkDeviceQueueCreateInfo QueueCreateInfo{};
    QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueCreateInfo.queueFamilyIndex = GraphicsQueueFamily;
    QueueCreateInfo.queueCount       = 1;
    QueueCreateInfo.pQueuePriorities = &QueuePriority;

    // Features standard (Anisotropy, etc.)
    VkPhysicalDeviceFeatures DeviceFeatures{};
    DeviceFeatures.samplerAnisotropy = VK_TRUE;

    // Features Vulkan 1.3 (Dynamic Rendering, Synchronization2)
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    Features13.dynamicRendering = VK_TRUE;
    Features13.synchronization2 = VK_TRUE;

    VkDeviceCreateInfo CreateInfo{};
    CreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.pNext                = &Features13; // Chainage des features Vulkan 1.3
    CreateInfo.queueCreateInfoCount = 1;
    CreateInfo.pQueueCreateInfos    = &QueueCreateInfo;
    CreateInfo.pEnabledFeatures     = &DeviceFeatures;

    CreateInfo.enabledExtensionCount   = static_cast<UInt32>( InRequiredExtensions.size() );
    CreateInfo.ppEnabledExtensionNames = InRequiredExtensions.data();

    // Layers (Gérés par l'instance dans les drivers récents, mais on peut les laisser à 0)
    CreateInfo.enabledLayerCount = 0;

    LUMEN_VK_CHECK( vkCreateDevice( PhysicalDevice, &CreateInfo, nullptr, &Device ) );
}
