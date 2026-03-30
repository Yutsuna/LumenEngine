/**
 * @file VulkanDevice.inl
 * @brief Template implementations for FVulkanDevice
 */

#pragma once

#include "../VulkanDevice.hpp"

template <LumenEngine::RHI::Concepts::CVulkanExtension... TExtensions> void LumenEngine::RHI::FVulkanDevice::EnableExtensions ( TExtensions... InExtensions )
{
    ( EnabledExtensions.push_back( InExtensions ), ... );
}

inline VkDevice LumenEngine::RHI::FVulkanDevice::GetLogicalDevice () const noexcept
{
    return LogicalDevice;
}

inline VkPhysicalDevice LumenEngine::RHI::FVulkanDevice::GetPhysicalDevice () const noexcept
{
    return PhysicalDevice;
}

inline const LumenEngine::RHI::FVulkanQueue &LumenEngine::RHI::FVulkanDevice::GetGraphicsQueue () const noexcept
{
    return GraphicsQueue;
}

inline const LumenEngine::RHI::FVulkanQueue &LumenEngine::RHI::FVulkanDevice::GetComputeQueue () const noexcept
{
    return ComputeQueue;
}

inline const LumenEngine::RHI::FVulkanQueue &LumenEngine::RHI::FVulkanDevice::GetTransferQueue () const noexcept
{
    return TransferQueue;
}

inline const LumenEngine::RHI::FVulkanQueue &LumenEngine::RHI::FVulkanDevice::GetPresentQueue () const noexcept
{
    return PresentQueue;
}
