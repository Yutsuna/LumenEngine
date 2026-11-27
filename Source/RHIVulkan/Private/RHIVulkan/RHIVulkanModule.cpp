/**
 * @file RVulkanModule.cpp
 * @brief Implementation of the RVulkanModule class.
 */

#include <RHIVulkan/RHIVulkanModule.hpp>

#include "Module/VulkanModule.hpp"

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIModule>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::CreateRHIVulkanModule () noexcept
{
    return FVulkanModule::Create();
}
