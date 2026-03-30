/**
 * @file VulkanResource.cpp
 * @brief Implementation of base classes for RHI resources
 */

#include "VulkanResource.hpp"
#include "VulkanRHI.hpp"
#include "LaunchEngineLoop.hpp"

LumenEngine::RHI::FVulkanResource::FVulkanResource () noexcept = default;

LumenEngine::RHI::FVulkanResource::~FVulkanResource () noexcept = default;

void LumenEngine::RHI::FVulkanResource::DeferredRelease ()
{
    DeletionFrame = GEngineLoop.GetFrameIndex();
    FVulkanRHI::Get().DeferredDeletion( this );
}
