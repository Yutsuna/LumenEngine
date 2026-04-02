/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the Vulkan Render Hardware Interface.
 */

#include "Vulkan/VulkanRHI.hpp"
#include "Generic/GenericWindow.hpp"

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
    CommandPool.Cleanup( LogicalDevice.GetHandle() );
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
    CommandPool.Initialize( LogicalDevice.GetHandle(), LogicalDevice.GetGraphicsQueueFamily() );

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        CommandBuffers[Index] = CommandPool.AllocateBuffer( LogicalDevice.GetHandle() );
    }
}

void LumenEngine::VulkanRHI::FVulkanRHI::WaitIdle () const noexcept
{
    LogicalDevice.WaitIdle();
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanRHI::BeginFrame ()
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

    LUMEN_VK_CHECK( vkResetCommandBuffer( CommandBuffers[CurrentFrame].GetHandle(), 0 ) );

    CommandBuffers[CurrentFrame].Begin( VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );
    CommandBuffers[CurrentFrame].TransitionImageLayout( Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanRHI::ClearScreen ( const Float32 ClearColor[4] )
{
    const UInt32 CurrentFrame  = FrameIndex % MaxFramesInFlight;
    const VkCommandBuffer &Cmd = CommandBuffers[CurrentFrame].GetHandle();
    const VkImage &Image       = SwapChain.GetImages()[CurrentImageIndex];

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
    const VkImage &Image      = SwapChain.GetImages()[CurrentImageIndex];

    CommandBuffers[CurrentFrame].TransitionImageLayout( Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT );
    CommandBuffers[CurrentFrame].End();

    SwapChain.SubmitAndPresent( CommandBuffers[CurrentFrame].GetHandle(), LogicalDevice.GetGraphicsQueue(), CurrentFrame, CurrentImageIndex );
    ++FrameIndex;
}
