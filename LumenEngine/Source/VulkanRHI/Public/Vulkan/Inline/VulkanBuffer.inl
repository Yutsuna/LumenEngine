/**
 * @file VulkanBuffer.inl
 * @brief Inline implementations for FVulkanBuffer
 */

#pragma once

#include "../VulkanBuffer.hpp"

inline VkBuffer LumenEngine::RHI::FVulkanBuffer::GetHandle () const noexcept
{
    return Buffer;
}

inline VkDeviceSize LumenEngine::RHI::FVulkanBuffer::GetSize () const noexcept
{
    return Size;
}
