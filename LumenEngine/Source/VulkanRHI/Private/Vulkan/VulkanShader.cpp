/**
 * @file VulkanShader.cpp
 * @brief Implementation of the FVulkanShader class for Vulkan shader management.
 */

#include "Vulkan/VulkanShader.hpp"
#include "Vulkan/VulkanCore.hpp"

#include "Filesystem/File.hpp"

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanShader::CompileFromFile ( VkDevice Device, const FString &FilePath, VkShaderStageFlagBits InStage ) noexcept
{
    Stage = InStage;

    auto ShaderCode = Filesystem::FFile::ReadAllBytes<LumenEngine::UInt8>( Filesystem::FPath( FilePath ) );

    if ( not ShaderCode.has_value() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to read shader file: {}", FilePath.c_str() );
        return false;
    }

    VkShaderModuleCreateInfo CreateInfo{};
    CreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    CreateInfo.codeSize = ShaderCode.value().size();
    CreateInfo.pCode    = reinterpret_cast<const UInt32 *>( ShaderCode.value().data() );

    /** Create the Vulkan shader module */
    LUMEN_VK_CHECK( vkCreateShaderModule( Device, &CreateInfo, nullptr, &ShaderModule ) );

    LUMEN_LOG_INFO( LogVulkanRHI, "Vulkan Shader Module created: {}", FilePath.c_str() );
    return true;
}

void LumenEngine::VulkanRHI::FVulkanShader::Cleanup ( VkDevice Device ) noexcept
{
    if ( ShaderModule != VK_NULL_HANDLE )
    {
        vkDestroyShaderModule( Device, ShaderModule, nullptr );
        ShaderModule = VK_NULL_HANDLE;
    }
}

VkShaderModule LumenEngine::VulkanRHI::FVulkanShader::GetModule () const noexcept
{
    return ShaderModule;
}

VkShaderStageFlagBits LumenEngine::VulkanRHI::FVulkanShader::GetStage () const noexcept
{
    return Stage;
}
