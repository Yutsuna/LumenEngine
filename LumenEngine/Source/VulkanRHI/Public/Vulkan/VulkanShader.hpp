/**
 * @file VulkanShader.hpp
 * @brief Declaration of the FVulkanShader class for Vulkan shader management.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/String.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    class LUMEN_ENGINE_API FVulkanShader
    {
    public:

        FVulkanShader () noexcept  = default;
        ~FVulkanShader () noexcept = default;

    public:

        /**
         * @brief Compiles a shader from a file path.
         * @param Device The Vulkan device to create the shader module with.
         * @param FilePath The file path to the shader source code.
         * @param Stage The shader stage (e.g., vertex, fragment) to compile for.
         * @return True if compilation was successful, false otherwise.
         */
        [[nodiscard]] Bool CompileFromFile ( VkDevice Device, const FString &FilePath, VkShaderStageFlagBits Stage ) noexcept;

        void Cleanup ( VkDevice Device ) noexcept;

    public:

        [[nodiscard]] VkShaderModule GetModule () const noexcept;
        [[nodiscard]] VkShaderStageFlagBits GetStage () const noexcept;

    private:

        VkShaderModule ShaderModule{ VK_NULL_HANDLE };
        VkShaderStageFlagBits Stage = VK_SHADER_STAGE_VERTEX_BIT;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
