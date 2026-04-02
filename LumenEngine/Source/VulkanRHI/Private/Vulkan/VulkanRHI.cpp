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
    const VkPhysicalDevice &PhysicalDeviceHandle = PhysicalDevice.physical_device;
    const VkDevice &LogicalDeviceHandle          = LogicalDevice.GetHandle();
    const Maths::FVec2u &WindowSize              = static_cast<Maths::FVec2u>( InWindow->GetWindowSize() );

    SwapChain.InitializeSynStructures( LogicalDeviceHandle );
    SwapChain.Create( PhysicalDeviceHandle, LogicalDeviceHandle, Surface, VK_FORMAT_B8G8R8A8_SRGB, WindowSize, true );
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

void LumenEngine::VulkanRHI::FVulkanRHI::CreateCommandBuffers ()
{
    VkCommandPoolCreateInfo PoolInfo{};
    PoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PoolInfo.queueFamilyIndex = LogicalDevice.GetGraphicsQueueFamily();

    LUMEN_VK_CHECK( vkCreateCommandPool( LogicalDevice.GetHandle(), &PoolInfo, nullptr, &CommandPool ) );

    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool        = CommandPool;
    AllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocInfo.commandBufferCount = MaxFramesInFlight;

    LUMEN_VK_CHECK( vkAllocateCommandBuffers( LogicalDevice.GetHandle(), &AllocInfo, CommandBuffers ) );
}

void LumenEngine::VulkanRHI::FVulkanRHI::WaitIdle () const noexcept
{
    LogicalDevice.WaitIdle();
}

bool LumenEngine::VulkanRHI::FVulkanRHI::BeginFrame ()
{
    const UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    VkDevice Device           = LogicalDevice.GetHandle();

    SwapChain.BeginFrame( Device, CurrentFrame );

    auto [Image, ImageIndex] = SwapChain.AcquireNextImage( Device, CurrentFrame );
    if ( Image == VK_NULL_HANDLE )
    {
        return false;
    }

    CurrentImageIndex = ImageIndex;
    SwapChain.ResetFences( Device, CurrentFrame );

    VkCommandBuffer Cmd = CommandBuffers[CurrentFrame];
    LUMEN_VK_CHECK( vkResetCommandBuffer( Cmd, 0 ) );

    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    LUMEN_VK_CHECK( vkBeginCommandBuffer( Cmd, &BeginInfo ) );

    VkImageMemoryBarrier2 TransitionToGeneral{};
    TransitionToGeneral.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    TransitionToGeneral.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToGeneral.srcAccessMask                   = 0;
    TransitionToGeneral.dstStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToGeneral.dstAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    TransitionToGeneral.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    TransitionToGeneral.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    TransitionToGeneral.image                           = Image;
    TransitionToGeneral.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionToGeneral.subresourceRange.baseMipLevel   = 0;
    TransitionToGeneral.subresourceRange.levelCount     = 1;
    TransitionToGeneral.subresourceRange.baseArrayLayer = 0;
    TransitionToGeneral.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.imageMemoryBarrierCount = 1;
    DepInfo.pImageMemoryBarriers    = &TransitionToGeneral;
    vkCmdPipelineBarrier2( Cmd, &DepInfo );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanRHI::ClearScreen ( const Float32 ClearColor[4] )
{
    const UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    VkCommandBuffer Cmd       = CommandBuffers[CurrentFrame];
    VkImage Image             = SwapChain.GetImages()[CurrentImageIndex];

    VkClearColorValue ClearValue{};
    ClearValue.float32[0] = ClearColor[0];
    ClearValue.float32[1] = ClearColor[1];
    ClearValue.float32[2] = ClearColor[2];
    ClearValue.float32[3] = ClearColor[3];

    const VkImageSubresourceRange ClearRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdClearColorImage( Cmd, Image, VK_IMAGE_LAYOUT_GENERAL, &ClearValue, 1, &ClearRange );
}

void LumenEngine::VulkanRHI::FVulkanRHI::EndFrame ()
{
    const UInt32 CurrentFrame = FrameIndex % MaxFramesInFlight;
    VkCommandBuffer Cmd       = CommandBuffers[CurrentFrame];
    VkImage Image             = SwapChain.GetImages()[CurrentImageIndex];

    VkImageMemoryBarrier2 TransitionToPresent{};
    TransitionToPresent.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    TransitionToPresent.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    TransitionToPresent.srcAccessMask                   = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    TransitionToPresent.dstStageMask                    = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    TransitionToPresent.dstAccessMask                   = 0;
    TransitionToPresent.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    TransitionToPresent.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    TransitionToPresent.image                           = Image;
    TransitionToPresent.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionToPresent.subresourceRange.baseMipLevel   = 0;
    TransitionToPresent.subresourceRange.levelCount     = 1;
    TransitionToPresent.subresourceRange.baseArrayLayer = 0;
    TransitionToPresent.subresourceRange.layerCount     = 1;

    VkDependencyInfo DepInfo{};
    DepInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    DepInfo.imageMemoryBarrierCount = 1;
    DepInfo.pImageMemoryBarriers    = &TransitionToPresent;
    vkCmdPipelineBarrier2( Cmd, &DepInfo );

    LUMEN_VK_CHECK( vkEndCommandBuffer( Cmd ) );

    SwapChain.SubmitAndPresent( Cmd, LogicalDevice.GetGraphicsQueue(), CurrentFrame, CurrentImageIndex );
    ++FrameIndex;
}
