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

namespace LumenEngine
{

namespace VulkanRHI
{

    namespace VulkanSceneRenderer
    {

        namespace
        {

            RHI::FPipelineHandle FindBatchPipeline ( const RHI::FSceneSnapshot &InSnapshot,
                                                     USize InCount,
                                                     const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry ) noexcept
            {
                for ( USize Index = 0U; Index < InCount; ++Index )
                {
                    if ( InPipelineRegistry.IsValid( InSnapshot.Shaders[Index] ) )
                    {
                        return InSnapshot.Shaders[Index];
                    }
                }
                return {};
            }

            void RenderFallbackCPU ( VkCommandBuffer InCmd,
                                     const RHI::FSceneSnapshot &InSnapshot,
                                     USize InCount,
                                     const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                     const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
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

                    Pipeline->Bind( InCmd );
                    vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetLayout(), 0, 1, &GlobalSet, 0, nullptr );
                    vkCmdPushConstants( InCmd, Pipeline->GetLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof( Maths::FMatrix4x4f ), &InSnapshot.Transforms[Index] );

                    Mesh->BindAndDraw( InCmd );
                }
            }

            void RenderGPUDriven ( VkCommandBuffer InCmd,
                                   const RHI::FSceneSnapshot &InSnapshot,
                                   USize InCount,
                                   UInt32 InFrameIndex,
                                   const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                                   const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                   const FVulkanMemory &InMemory,
                                   FGPUSceneBuffer &InSceneBuffer,
                                   const FGPUIndirectBuffer &InIndirectBuffer,
                                   const FGPUCullingPass &InCullingPass ) noexcept
            {
                const RHI::FPipelineHandle PipelineHandle = FindBatchPipeline( InSnapshot, InCount, InPipelineRegistry );
                if ( not PipelineHandle.IsValid() )
                {
                    return;
                }

                /** INFO: Prepare GPU Data & Execute Compute Culling */
                InSceneBuffer.Upload( InSnapshot, InMeshRegistry, InPipelineRegistry, InFrameIndex );

                const VkDescriptorSet &GlobalSet = InMemory.GetGlobalDescriptorSet( InFrameIndex );
                InCullingPass.Execute( InCmd, GlobalSet, InSceneBuffer, InIndirectBuffer, InFrameIndex );

                /** INFO: Execute Indirect Draw Batch */
                const FVulkanPipeline *Pipeline = InPipelineRegistry.Get( PipelineHandle );
                Pipeline->Bind( InCmd );
                vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetLayout(), 0, 1, &GlobalSet, 0, nullptr );

                /** INFO: Bind a representative vertex/index buffer to satisfy the pipeline state */
                const FVulkanMesh *RepresentativeMesh = InMeshRegistry.Get( InSnapshot.Meshes[0] );

                if ( RepresentativeMesh != nullptr )
                {
                    VkDeviceSize Offset = 0;
                    VkBuffer VB         = RepresentativeMesh->GetVertexBuffer();

                    vkCmdBindVertexBuffers( InCmd, 0, 1, &VB, &Offset );
                    vkCmdBindIndexBuffer( InCmd, RepresentativeMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32 );
                }

                vkCmdDrawIndexedIndirectCount( InCmd, InIndirectBuffer.GetIndirectBuffer(), 0, InIndirectBuffer.GetCountBuffer(), 0,
                                               static_cast<UInt32>( FGPUIndirectBuffer::MaxDraws ), sizeof( VkDrawIndexedIndirectCommand ) );
            }

        } // namespace

        void RenderScene ( VkCommandBuffer InCmd,
                           const RHI::FSceneSnapshot &InSceneSnapshot,
                           UInt32 InFrameIndex,
                           const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                           const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                           const FVulkanMemory &InMemory,
                           FGPUSceneBuffer &InSceneBuffer,
                           const FGPUIndirectBuffer &InIndirectBuffer,
                           const FGPUCullingPass &InCullingPass ) noexcept
        {
            const USize SceneCount = std::min( { InSceneSnapshot.Transforms.size(), InSceneSnapshot.Meshes.size(), InSceneSnapshot.Shaders.size() } );

            if ( SceneCount == 0U )
            {
                return;
            }

            /** INFO: Decide execution path based on Culling Pass readiness */
            if ( not InCullingPass.IsReady() )
            {
                RenderFallbackCPU( InCmd, InSceneSnapshot, SceneCount, InPipelineRegistry, InMeshRegistry, InMemory, InFrameIndex );
            }
            else
            {
                RenderGPUDriven( InCmd, InSceneSnapshot, SceneCount, InFrameIndex, InMeshRegistry, InPipelineRegistry, InMemory, InSceneBuffer, InIndirectBuffer,
                                 InCullingPass );
            }
        }

    } // namespace VulkanSceneRenderer

} // namespace VulkanRHI

} // namespace LumenEngine