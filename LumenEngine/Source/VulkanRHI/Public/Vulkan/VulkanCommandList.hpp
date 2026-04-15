/**
 * @file VulkanCommandList.hpp
 * @brief Vulkan implementation of IRHICommandList.
 */

#pragma once

#include "Definitions.hpp"
#include "RHI/RHICommandList.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    class FVulkanRHI;

    /**
     * @class FVulkanCommandList
     * @brief Translates generic RHI commands to Vulkan vkCmd calls.
     */
    class LUMEN_ENGINE_API FVulkanCommandList final : public RHI::IRHICommandList
    {
    public:

        explicit FVulkanCommandList ( FVulkanRHI *InRHI ) noexcept;
        ~FVulkanCommandList () override = default;

    public:

        void BeginRendering ( const Float32 InClearColor[4] ) override;
        void EndRendering () override;
        void BindPipeline ( const RHI::FPipelineHandle InPipeline ) override;
        void PushConstants ( const RHI::FPipelineHandle InPipeline, const void *InData, UInt32 InSize, UInt32 InOffset = 0 ) override;
        void DrawMesh ( const RHI::FMeshHandle InMesh ) override;

    public:

        /** @brief Used internally to set the active Vulkan command buffer for this frame */
        void SetActiveCommandBuffer ( VkCommandBuffer InCmd ) noexcept
        {
            CurrentCmd = InCmd;
        }

    private:

        FVulkanRHI *RHI;
        VkCommandBuffer CurrentCmd = VK_NULL_HANDLE;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
