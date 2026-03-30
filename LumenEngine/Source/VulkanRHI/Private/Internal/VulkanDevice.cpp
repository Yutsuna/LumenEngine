/**
 * @file VulkanDevice.cpp
 * @brief Implementation of the FVulkanDevice class
 */

#include "VulkanDevice.hpp"
#include "VulkanCore.hpp"
#include "Container/Vector.hpp"
#include <vulkan/vulkan_core.h>

LumenEngine::RHI::FVulkanDevice::FVulkanDevice () noexcept = default;

LumenEngine::RHI::FVulkanDevice::~FVulkanDevice () noexcept
{
    if ( LogicalDevice != VK_NULL_HANDLE )
    {
        vkDestroyDevice( LogicalDevice, nullptr );
        LogicalDevice = VK_NULL_HANDLE;
    }
}

LumenEngine::TExpected<void, LumenEngine::FString> LumenEngine::RHI::FVulkanDevice::Initialize ( VkInstance InInstance, VkSurfaceKHR InSurface )
{
    SelectPhysicalDevice( InInstance, InSurface );
    if ( PhysicalDevice == VK_NULL_HANDLE )
    {
        return std::unexpected( FString( "Failed to find a suitable Vulkan physical device." ) );
    }

    return CreateLogicalDevice( InSurface );
}

void LumenEngine::RHI::FVulkanDevice::SelectPhysicalDevice ( VkInstance InInstance, VkSurfaceKHR InSurface )
{
    UInt32 DeviceCount = 0;
    VULKAN_CHECK( vkEnumeratePhysicalDevices( InInstance, &DeviceCount, nullptr ) );

    if ( DeviceCount == 0 )
    {
        return;
    }

    TVector<VkPhysicalDevice> Devices( DeviceCount );
    VULKAN_CHECK( vkEnumeratePhysicalDevices( InInstance, &DeviceCount, Devices.GetData() ) );

    // Simple selection: Prefer Discrete GPU
    for ( const auto &Device : Devices )
    {
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties( Device, &DeviceProperties );

        if ( DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        {
            PhysicalDevice = Device;
            return;
        }
    }

    // Fallback to first device
    PhysicalDevice = Devices[0];
}

LumenEngine::TExpected<void, LumenEngine::FString> LumenEngine::RHI::FVulkanDevice::CreateLogicalDevice ( VkSurfaceKHR InSurface )
{
    UInt32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &QueueFamilyCount, nullptr );

    TVector<VkQueueFamilyProperties> QueueFamilies( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( PhysicalDevice, &QueueFamilyCount, QueueFamilies.GetData() );

    Int32 GraphicsFamily = -1;
    Int32 ComputeFamily  = -1;
    Int32 TransferFamily = -1;
    Int32 PresentFamily  = -1;

    for ( UInt32 Index = 0; Index < QueueFamilyCount; ++Index )
    {
        const auto &QueueFamily = QueueFamilies[Index];

        if ( QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            GraphicsFamily = Index;
        }

        if ( QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
        {
            ComputeFamily = Index;
        }

        if ( QueueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT )
        {
            TransferFamily = Index;
        }

        VkBool32 bPresentSupport = false;
        VULKAN_CHECK( vkGetPhysicalDeviceSurfaceSupportKHR( PhysicalDevice, Index, InSurface, &bPresentSupport ) );
        if ( bPresentSupport )
        {
            PresentFamily = Index;
        }

        if ( GraphicsFamily != -1 && ComputeFamily != -1 && TransferFamily != -1 && PresentFamily != -1 )
        {
            break;
        }
    }

    if ( GraphicsFamily == -1 || PresentFamily == -1 )
    {
        return std::unexpected( FString( "Failed to find necessary queue families." ) );
    }

    TVector<VkDeviceQueueCreateInfo> QueueCreateInfos;
    TVector<Int32> UniqueQueueFamilies;
    UniqueQueueFamilies.PushBack( GraphicsFamily );
    if ( ComputeFamily != -1 && ComputeFamily != GraphicsFamily ) UniqueQueueFamilies.PushBack( ComputeFamily );
    if ( TransferFamily != -1 && TransferFamily != GraphicsFamily && TransferFamily != ComputeFamily ) UniqueQueueFamilies.PushBack( TransferFamily );
    if ( PresentFamily != -1 && PresentFamily != GraphicsFamily && PresentFamily != ComputeFamily && PresentFamily != TransferFamily ) UniqueQueueFamilies.PushBack( PresentFamily );

    Float32 QueuePriority = 1.0f;
    for ( Int32 Family : UniqueQueueFamilies )
    {
        VkDeviceQueueCreateInfo QueueCreateInfo{};
        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = Family;
        QueueCreateInfo.queueCount       = 1;
        QueueCreateInfo.pQueuePriorities = &QueuePriority;
        QueueCreateInfos.PushBack( QueueCreateInfo );
    }

    VkPhysicalDeviceFeatures DeviceFeatures{}; // Minimal features for now

    VkDeviceCreateInfo CreateInfo{};
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.queueCreateInfoCount    = static_cast<UInt32>( QueueCreateInfos.GetSize() );
    CreateInfo.pQueueCreateInfos       = QueueCreateInfos.GetData();
    CreateInfo.pEnabledFeatures        = &DeviceFeatures;
    CreateInfo.enabledExtensionCount   = static_cast<UInt32>( EnabledExtensions.GetSize() );
    CreateInfo.ppEnabledExtensionNames = EnabledExtensions.GetData();

    VULKAN_CHECK( vkCreateDevice( PhysicalDevice, &CreateInfo, nullptr, &LogicalDevice ) );

    // Retrieve Queues
    VkQueue QueueHandle;
    vkGetDeviceQueue( LogicalDevice, GraphicsFamily, 0, &QueueHandle );
    GraphicsQueue = FVulkanQueue( QueueHandle, GraphicsFamily, 0 );

    if ( ComputeFamily != -1 )
    {
        vkGetDeviceQueue( LogicalDevice, ComputeFamily, 0, &QueueHandle );
        ComputeQueue = FVulkanQueue( QueueHandle, ComputeFamily, 0 );
    }
    else
    {
        ComputeQueue = GraphicsQueue;
    }

    if ( TransferFamily != -1 )
    {
        vkGetDeviceQueue( LogicalDevice, TransferFamily, 0, &QueueHandle );
        TransferQueue = FVulkanQueue( QueueHandle, TransferFamily, 0 );
    }
    else
    {
        TransferQueue = GraphicsQueue;
    }

    vkGetDeviceQueue( LogicalDevice, PresentFamily, 0, &QueueHandle );
    PresentQueue = FVulkanQueue( QueueHandle, PresentFamily, 0 );

    return {};
}
