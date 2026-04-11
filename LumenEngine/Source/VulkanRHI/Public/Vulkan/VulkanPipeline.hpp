/**
 * @file VulkanPipeline.hpp
 * @brief Declaration of the FVulkanPipeline class for Vulkan Graphics Pipeline management.
 */

#pragma once

#include "Container/String.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanPipeline
     * @brief Encapsulates Vulkan graphics pipeline creation, management, and cleanup.
     */
    class LUMEN_ENGINE_API FVulkanPipeline final
    {

    public:

        FVulkanPipeline () noexcept  = default;
        ~FVulkanPipeline () noexcept = default;

    public:

        /**
         * @brief Initializes the Vulkan pipeline.
         * @param InDevice The Vulkan device.
         * @param InColorFormat The color format.
         * @param InVertexPath The path to the vertex shader file.
         * @param InFragmentPath The path to the fragment shader file.
         * @param InGlobalSetLayout The descriptor set layout configured for uniform binding.
         * @return True if initialization was successful, false otherwise.
         */
        Bool
        Initialize ( VkDevice InDevice, VkFormat InColorFormat, const FString &InVertexPath, const FString &InFragmentPath, VkDescriptorSetLayout InGlobalSetLayout );

        /**
         * @brief Cleans up the Vulkan pipeline.
         * @param InDevice The Vulkan device.
         */
        void Cleanup ( VkDevice InDevice ) noexcept;

        /**
         * @brief Binds the Vulkan pipeline.
         * @param InCommandBuffer The command buffer.
         */
        void Bind ( VkCommandBuffer InCommandBuffer ) const noexcept;

    public:

        /** @brief Retrieve the layout to bind descriptor sets. */
        [[nodiscard]] VkPipelineLayout GetLayout () const noexcept;

    private:

        VkPipeline Pipeline             = VK_NULL_HANDLE;
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine