/**
 * @file VulkanQueue.inl
 * @brief Inline implementations for FVulkanQueue
 */

#pragma once

inline LumenEngine::RHI::FVulkanQueue::FVulkanQueue () noexcept = default;

inline LumenEngine::RHI::FVulkanQueue::FVulkanQueue ( VkQueue InQueue, UInt32 InFamilyIndex, UInt32 InQueueIndex ) noexcept
    : Queue( InQueue ), FamilyIndex( InFamilyIndex ), QueueIndex( InQueueIndex )
{
}

inline LumenEngine::RHI::FVulkanQueue::~FVulkanQueue () noexcept = default;

inline VkQueue LumenEngine::RHI::FVulkanQueue::GetHandle () const noexcept
{
    return Queue;
}

inline LumenEngine::UInt32 LumenEngine::RHI::FVulkanQueue::GetFamilyIndex () const noexcept
{
    return FamilyIndex;
}

inline LumenEngine::UInt32 LumenEngine::RHI::FVulkanQueue::GetQueueIndex () const noexcept
{
    return QueueIndex;
}
