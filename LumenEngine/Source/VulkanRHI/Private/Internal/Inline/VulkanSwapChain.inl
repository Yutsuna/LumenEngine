/**
 * @file VulkanSwapChain.inl
 * @brief Inline implementations for FVulkanSwapChain
 */

#pragma once

#include "../VulkanSwapChain.hpp"

inline VkSemaphore LumenEngine::RHI::FVulkanSwapChain::GetImageAvailableSemaphore () const noexcept
{
    return FrameSync[CurrentFrame].ImageAvailableSemaphore;
}

inline VkSemaphore LumenEngine::RHI::FVulkanSwapChain::GetRenderFinishedSemaphore () const noexcept
{
    return FrameSync[CurrentFrame].RenderFinishedSemaphore;
}

inline VkFence LumenEngine::RHI::FVulkanSwapChain::GetInFlightFence () const noexcept
{
    return FrameSync[CurrentFrame].InFlightFence;
}

inline VkSwapchainKHR LumenEngine::RHI::FVulkanSwapChain::GetHandle () const noexcept
{
    return Handle;
}

inline LumenEngine::UInt32 LumenEngine::RHI::FVulkanSwapChain::GetCurrentImageIndex () const noexcept
{
    return CurrentImageIndex;
}
