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
    InitializeCommandBuffers();

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

    DestroyCommandBuffers();
    DestroySwapChain();
    DestroyVMA();
    DestroyVulkanDevice();
    DestroyVulkanInstance();

    bIsInitialized = false;
    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan RHI shut down." );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVulkanInstance ()
{
    Instance.Cleanup();
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVulkanDevice ()
{
    LogicalDevice.Cleanup();
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyVMA ()
{
    if ( Allocator != VK_NULL_HANDLE )
    {
        vmaDestroyAllocator( Allocator );
        Allocator = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroySwapChain ()
{
    SwapChain.Cleanup( LogicalDevice.GetHandle() );
}

void LumenEngine::VulkanRHI::FVulkanRHI::DestroyCommandBuffers ()
{
    if ( CommandPool != VK_NULL_HANDLE )
    {
        vkFreeCommandBuffers( LogicalDevice.GetHandle(), CommandPool, MaxFramesInFlight, CommandBuffers );
        vkDestroyCommandPool( LogicalDevice.GetHandle(), CommandPool, nullptr );
        CommandPool = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanInstance ( const TSharedPtr<FGenericWindow> &InWindow )
{
    Instance.Initialize( InWindow );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVulkanDevice ()
{
    const TVector<const AnsiChar *> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    PhysicalDevice.Initialize( Instance.GetHandle(), Instance.GetSurface() );
    LogicalDevice.Initialize( PhysicalDevice.GetHandle(), Instance.GetSurface(), DeviceExtensions );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeVMA ()
{
    VmaVulkanFunctions VulkanFunctions    = {};
    VulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    VulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo AllocatorInfo = {};
    AllocatorInfo.physicalDevice         = PhysicalDevice.GetHandle();
    AllocatorInfo.device                 = LogicalDevice.GetHandle();
    AllocatorInfo.instance               = Instance.GetHandle();
    AllocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    AllocatorInfo.pVulkanFunctions       = &VulkanFunctions;
    AllocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    LUMEN_VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &Allocator ) );
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeSwapChain ( const TSharedPtr<FGenericWindow> &InWindow )
{
    const VkPhysicalDevice &PhysicalDeviceHandle = PhysicalDevice.GetHandle();
    const VkDevice &LogicalDeviceHandle          = LogicalDevice.GetHandle();
    const Maths::FVec2u &WindowSize              = static_cast<Maths::FVec2u>( InWindow->GetWindowSize() );

    SwapChain.InitializeSynStructures( LogicalDeviceHandle );
    SwapChain.Create( PhysicalDeviceHandle, LogicalDeviceHandle, Instance.GetSurface(), VK_FORMAT_B8G8R8A8_SRGB, WindowSize, true );
}

LumenEngine::VulkanRHI::FVulkanLogicalDevice LumenEngine::VulkanRHI::FVulkanRHI::GetLogicalDevice () const noexcept
{
    return LogicalDevice;
}

LumenEngine::VulkanRHI::FVulkanPhysicalDevice LumenEngine::VulkanRHI::FVulkanRHI::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice;
}

LumenEngine::VulkanRHI::FVulkanInstance LumenEngine::VulkanRHI::FVulkanRHI::GetInstance () const noexcept
{
    return Instance;
}

VmaAllocator LumenEngine::VulkanRHI::FVulkanRHI::GetAllocator () const noexcept
{
    return Allocator;
}

LumenEngine::VulkanRHI::FVulkanSwapChain &LumenEngine::VulkanRHI::FVulkanRHI::GetSwapChain () noexcept
{
    return SwapChain;
}

void LumenEngine::VulkanRHI::FVulkanRHI::InitializeCommandBuffers ()
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
