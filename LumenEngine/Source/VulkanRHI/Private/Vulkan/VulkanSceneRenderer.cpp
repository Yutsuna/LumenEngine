/**
 * @file VulkanSceneRenderer.cpp
 * @brief Implementation of the modular scene rendering logic.
 */

#include "Vulkan/VulkanSceneRenderer.hpp"

#include "Vulkan/GPUDriven/GPUCullingPass.hpp"
#include "Vulkan/GPUDriven/GPUIndirectBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"
#include "Vulkan/VulkanMemory.hpp"
#include "Vulkan/VulkanMesh.hpp"
#include "Vulkan/VulkanPipeline.hpp"

#include <algorithm>
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    namespace
    {

        /** INFO: Common helper to bind the graphics state (Pipeline + Global UBO) */
        void SetupGraphicsState ( VkCommandBuffer InCmd, const FVulkanPipeline *InPipeline, VkDescriptorSet InGlobalSet ) noexcept
        {
            InPipeline->Bind( InCmd );
            vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, InPipeline->GetLayout(), 0, 1, &InGlobalSet, 0, nullptr );
        }

        /** INFO: Common helper to bind Mesh-specific buffers (VB + IB) */
        void BindGeometry ( VkCommandBuffer InCmd, const FVulkanMesh *InMesh ) noexcept
        {
            VkDeviceSize Offset = 0;
            VkBuffer VB         = InMesh->GetVertexBuffer();

            vkCmdBindVertexBuffers( InCmd, 0, 1, &VB, &Offset );
            vkCmdBindIndexBuffer( InCmd, InMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32 );
        }

        void RenderSceneCPU ( VkCommandBuffer InCmd,
                              const RHI::FSceneSnapshot &InSnapshot,
                              USize InCount,
                              const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                              const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                              const FVulkanMemory &InMemory,
                              UInt32 InFrameIndex ) noexcept
        {
            const VkDescriptorSet &GlobalSet = InMemory.GetGlobalDescriptorSet( InFrameIndex );

            for ( USize Index = 0U; Index < InCount; ++Index )
            {
                const FVulkanPipeline *Pipeline = InPipelineRegistry.Get( InSnapshot.Shaders[Index] );
                const FVulkanMesh *Mesh         = InMeshRegistry.Get( InSnapshot.Meshes[Index] );

                if ( Pipeline == nullptr or Mesh == nullptr )
                {
                    continue;
                }

                SetupGraphicsState( InCmd, Pipeline, GlobalSet );
                vkCmdPushConstants( InCmd, Pipeline->GetLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof( Maths::FMatrix4x4f ), &InSnapshot.Transforms[Index] );
                Mesh->BindAndDraw( InCmd );
            }
        }

        void RenderSceneGPU ( VkCommandBuffer InCmd,
                              const RHI::FSceneSnapshot &InSnapshot,
                              USize InCount,
                              UInt32 InFrameIndex,
                              const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                              const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                              const FVulkanMemory &InMemory,
                              const FGPUIndirectBuffer &InIndirectBuffer ) noexcept
        {
            const VkDescriptorSet &GlobalSet = InMemory.GetGlobalDescriptorSet( InFrameIndex );

            /** INFO: Find a valid batch resource to bind the pipeline state */
            for ( USize Index = 0U; Index < InCount; ++Index )
            {
                const FVulkanPipeline *Pipeline = InPipelineRegistry.Get( InSnapshot.Shaders[Index] );
                const FVulkanMesh *Mesh         = InMeshRegistry.Get( InSnapshot.Meshes[Index] );

                if ( Pipeline != nullptr and Mesh != nullptr )
                {
                    SetupGraphicsState( InCmd, Pipeline, GlobalSet );
                    BindGeometry( InCmd, Mesh );
                    break;
                }
            }

            vkCmdDrawIndexedIndirectCount( InCmd, InIndirectBuffer.GetIndirectBuffer(), 0, InIndirectBuffer.GetCountBuffer(), 0,
                                           static_cast<UInt32>( FGPUIndirectBuffer::MaxDraws ), sizeof( VkDrawIndexedIndirectCommand ) );
        }

    } // namespace

    void VulkanSceneRenderer::PrepareScene ( VkCommandBuffer InCmd,
                                             const RHI::FSceneSnapshot &InSceneSnapshot,
                                             UInt32 InFrameIndex,
                                             const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                                             const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                             const FVulkanMemory &InMemory,
                                             FGPUSceneBuffer &InSceneBuffer,
                                             const FGPUIndirectBuffer &InIndirectBuffer,
                                             const FGPUCullingPass &InCullingPass ) noexcept
    {
        if ( not InCullingPass.IsReady() )
        {
            return;
        }

        InSceneBuffer.Upload( InSceneSnapshot, InMeshRegistry, InPipelineRegistry, InFrameIndex );

        const VkDescriptorSet &GlobalSet = InMemory.GetGlobalDescriptorSet( InFrameIndex );
        InCullingPass.Execute( InCmd, GlobalSet, InSceneBuffer, InIndirectBuffer, InFrameIndex );
    }

    void VulkanSceneRenderer::RenderScene ( VkCommandBuffer InCmd,
                                            const RHI::FSceneSnapshot &InSceneSnapshot,
                                            UInt32 InFrameIndex,
                                            const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                                            const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                            const FVulkanMemory &InMemory,
                                            const FGPUIndirectBuffer &InIndirectBuffer,
                                            const FGPUCullingPass &InCullingPass ) noexcept
    {
        const USize SceneCount = std::min( { InSceneSnapshot.Transforms.size(), InSceneSnapshot.Meshes.size(), InSceneSnapshot.Shaders.size() } );

        if ( SceneCount == 0U )
        {
            return;
        }

        if ( InCullingPass.IsReady() )
        {
            RenderSceneGPU( InCmd, InSceneSnapshot, SceneCount, InFrameIndex, InMeshRegistry, InPipelineRegistry, InMemory, InIndirectBuffer );
        }
        else
        {
            RenderSceneCPU( InCmd, InSceneSnapshot, SceneCount, InMeshRegistry, InPipelineRegistry, InMemory, InFrameIndex );
        }
    }

} // namespace VulkanRHI

} // namespace LumenEngine
