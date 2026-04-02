/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"
#include "Generic/GenericWindow.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    #include "Linux/LinuxWindow.hpp"
    #include <SDL3/SDL_vulkan.h>
#endif

void LumenEngine::VulkanRHI::FVulkanRHI::Initialize ( const TSharedPtr<FGenericWindow> &InWindow )
{

    if ( bIsInitialized )
    {
        return;
    }

    InitializeVulkanInstance( InWindow );
    InitializeVulkanDevice();
    InitializeVMA();
    InitializeSwapChain( InWindow );

    bIsInitialized = true;

    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI initialized successfully." );
}

void LumenEngine::VulkanRHI::FVulkanRHI::Shutdown ()
{
    if ( not bIsInitialized )
    {
        return;
    }

    LogicalDevice.WaitIdle();
    SwapChain.Cleanup( LogicalDevice.GetHandle() );

    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
        Allocator = VK_NULL_HANDLE;
    }

    LogicalDevice.Cleanup();

    if ( Surface != VK_NULL_HANDLE )
    {
        vkb::destroy_surface( Instance, Surface );
        Surface = VK_NULL_HANDLE;
    }

    if ( Instance.instance != VK_NULL_HANDLE )
    {
        vkb::destroy_instance( Instance );
    }

    bIsInitialized = false;
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI shut down." );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanInstance ( const TSharedPtr<FGenericWindow> &InWindow )
{
    vkb::InstanceBuilder Builder;
    vkb::Result<vkb::Instance> InstanceResult = Builder.set_app_name( "Lumen Engine" ).request_validation_layers( true ).require_api_version( 1, 3, 0 ).build();

    if ( not InstanceResult )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to create Vulkan Instance: {}", InstanceResult.error().message() );
    }

    Instance = InstanceResult.value();

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    TSharedPtr<FLinuxWindow> LinuxWindow = StaticCastSharedPtr<FLinuxWindow>( InWindow );
    SDL_Window *OSWindow                 = LinuxWindow->GetOSWindowHandle();

    if ( not SDL_Vulkan_CreateSurface( OSWindow, Instance.instance, nullptr, &Surface ) )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to create Vulkan Surface: {}", SDL_GetError() );
    }
#else
    LUMEN_LOG_FATAL( LogVulkanRHI, "Unsupported platform for surface creation." );
    return false;
#endif
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanDevice ()
{
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.samplerAnisotropy        = VK_TRUE;

    VkPhysicalDeviceVulkan13Features Features13 = {};
    Features13.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    Features13.dynamicRendering                 = VK_TRUE;
    Features13.synchronization2                 = VK_TRUE;

    vkb::PhysicalDeviceSelector Selector{ Instance };
    vkb::Result<vkb::PhysicalDevice> PhysDeviceResult =
        Selector.set_surface( Surface ).set_minimum_version( 1, 3 ).set_required_features( DeviceFeatures ).add_required_extension_features( Features13 ).select();

    if ( not PhysDeviceResult )
    {
        LUMEN_LOG_FATAL( LogVulkanRHI, "Failed to select Vulkan Physical Device: {}", PhysDeviceResult.error().message() );
    }

    PhysicalDevice = PhysDeviceResult.value();

    TVector<const AnsiChar *> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    LogicalDevice.Initialize( PhysicalDevice.physical_device, Surface, DeviceExtensions );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVMA ()
{
    VmaVulkanFunctions VulkanFunctions    = {};
    VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    VulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo AllocatorInfo = {};
    AllocatorInfo.physicalDevice         = PhysicalDevice.physical_device;
    AllocatorInfo.device                 = LogicalDevice.GetHandle();
    AllocatorInfo.instance               = Instance.instance;
    AllocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    AllocatorInfo.pVulkanFunctions       = &VulkanFunctions;
    AllocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeSwapChain ( const TSharedPtr<FGenericWindow> &InWindow )
{
    SwapChain.InitializeSynStructures( LogicalDevice.GetHandle() );
    SwapChain.Create( LogicalDevice, VK_FORMAT_B8G8R8A8_SRGB, static_cast<Maths::FVec2u>( InWindow->GetWindowSize() ), true );
}

LumenEngine::VulkanRHI::FVulkanDevice LumenEngine::VulkanRHI::FVulkanRHI::GetDevice () const noexcept
{
    return LogicalDevice;
}

VkPhysicalDevice LumenEngine::VulkanRHI::FVulkanRHI::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice.physical_device;
}

VkInstance LumenEngine::VulkanRHI::FVulkanRHI::GetInstance () const noexcept
{
    return Instance.instance;
}

VmaAllocator LumenEngine::VulkanRHI::FVulkanRHI::GetAllocator () const noexcept
{
    return Allocator;
}

LumenEngine::VulkanRHI::FVulkanSwapChain &LumenEngine::VulkanRHI::FVulkanRHI::GetSwapChain () noexcept
{
    return SwapChain;
}
