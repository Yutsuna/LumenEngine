/**
 * @file VulkanFrameContext.cpp
 * @brief Frame lifecycle and command buffer context implementation.
 */

#include "Vulkan/VulkanFrameContext.hpp"
#include "Vulkan/VulkanCore.hpp"

void LumenEngine::VulkanRHI::FVulkanFrameContext::Initialize ( VkDevice InDevice, UInt32 InGraphicsQueueFamily )
{
    CommandPool.Initialize( InDevice, InGraphicsQueueFamily );

    for ( UInt32 Index = 0; Index < MaxFramesInFlight; ++Index )
    {
        CommandBuffers[Index] = CommandPool.AllocateBuffer( InDevice );
    }

    FrameIndex        = 0;
    CurrentImageIndex = 0;
}

void LumenEngine::VulkanRHI::FVulkanFrameContext::Shutdown ( VkDevice InDevice ) noexcept
{
    CommandPool.Cleanup( InDevice );
}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanFrameContext::BeginFrame ( VkDevice InDevice, FVulkanSwapChain &InSwapChain )
{
    const UInt32 CurrentFrame = GetCurrentFrameIndex();

    InSwapChain.BeginFrame( InDevice, CurrentFrame );

    const std::pair<VkImage, UInt32> NextImagePair = InSwapChain.AcquireNextImage( InDevice, CurrentFrame );
    if ( NextImagePair.first == VK_NULL_HANDLE )
    {
        return false;
    }

    CurrentImageIndex = NextImagePair.second;
    InSwapChain.ResetFences( InDevice, CurrentFrame );

    LUMEN_VK_CHECK( vkResetCommandBuffer( CommandBuffers[CurrentFrame].GetHandle(), 0 ) );
    CommandBuffers[CurrentFrame].Begin( VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );

    return true;
}

void LumenEngine::VulkanRHI::FVulkanFrameContext::SubmitAndPresent ( FVulkanSwapChain &InSwapChain, VkQueue InGraphicsQueue )
{
    const UInt32 CurrentFrame = GetCurrentFrameIndex();

    CommandBuffers[CurrentFrame].End();
    InSwapChain.SubmitAndPresent( CommandBuffers[CurrentFrame].GetHandle(), InGraphicsQueue, CurrentFrame, CurrentImageIndex );

    ++FrameIndex;
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanFrameContext::GetCurrentFrameIndex () const noexcept
{
    return static_cast<UInt32>( FrameIndex % MaxFramesInFlight );
}

LumenEngine::UInt32 LumenEngine::VulkanRHI::FVulkanFrameContext::GetCurrentImageIndex () const noexcept
{
    return CurrentImageIndex;
}

LumenEngine::VulkanRHI::FVulkanCommandBuffer &LumenEngine::VulkanRHI::FVulkanFrameContext::GetCurrentCommandBuffer () noexcept
{
    return CommandBuffers[GetCurrentFrameIndex()];
}

const LumenEngine::VulkanRHI::FVulkanCommandBuffer &LumenEngine::VulkanRHI::FVulkanFrameContext::GetCurrentCommandBuffer () const noexcept
{
    return CommandBuffers[GetCurrentFrameIndex()];
}
