/**
 * @file VulkanRHI.cpp
 * @brief Implementation of the FVulkanRHI class for Vulkan rendering context management.
 */

#include "VulkanRHI.hpp"

LumenEngine::RHI::FVulkanRHI::FVulkanRHI () noexcept
{
    Device = MakeUnique<FVulkanDevice>();
}
