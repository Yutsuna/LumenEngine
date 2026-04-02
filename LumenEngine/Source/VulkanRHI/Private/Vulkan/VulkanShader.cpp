/**
 * @file VulkanShader.cpp
 * @brief Implementation of the FVulkanShader class for Vulkan shader management.
 */

#include "Vulkan/VulkanShader.hpp"

namespace
{

}

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanShader::CompileFromFile ( VkDevice Device, const FString &FilePath, VkShaderStageFlagBits Stage ) noexcept
{
}

void LumenEngine::VulkanRHI::FVulkanShader::Cleanup ( VkDevice Device ) noexcept
{
}

VkShaderModule LumenEngine::VulkanRHI::FVulkanShader::GetModule () const noexcept
{
    return ShaderModule;
}

VkShaderStageFlagBits LumenEngine::VulkanRHI::FVulkanShader::GetStage () const noexcept
{
    return Stage;
}
