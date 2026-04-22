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

            /** INFO: Extracts the first valid mesh/pipeline pair to establish the "Batch State" */
            struct FBatchResources
            {
                const FVulkanPipeline *Pipeline = nullptr;
                const FVulkanMesh *Mesh         = nullptr;
                RHI::FPipelineHandle Handle;
            };

            /** INFO: Common helper to bind the graphics state (Pipeline + Global UBO) */
            inline void SetupGraphicsState ( VkCommandBuffer InCmd, const FVulkanPipeline *InPipeline, VkDescriptorSet InGlobalSet ) noexcept
            {
                InPipeline->Bind( InCmd );

                vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, InPipeline->GetLayout(), 0, 1, &InGlobalSet, 0, nullptr );
            }

            /** INFO: Common helper to bind Mesh-specific buffers (VB + IB) */
            inline void BindGeometry ( VkCommandBuffer InCmd, const FVulkanMesh *InMesh ) noexcept
            {
                VkDeviceSize Offset = 0;
                VkBuffer VB         = InMesh->GetVertexBuffer();

                vkCmdBindVertexBuffers( InCmd, 0, 1, &VB, &Offset );
                vkCmdBindIndexBuffer( InCmd, InMesh->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32 );
            }

            [[nodiscard]] FBatchResources GetFirstValidBatch ( const RHI::FSceneSnapshot &InSnapshot,
                                                               USize InCount,
                                                               const RHI::TResourceRegistry<FVulkanMesh, RHI::FMeshTag> &InMeshRegistry,
                                                               const RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry ) noexcept
            {
                for ( USize Index = 0U; Index < InCount; ++Index )
                {
                    const FVulkanPipeline *P = InPipelineRegistry.Get( InSnapshot.Shaders[Index] );
                    const FVulkanMesh *M     = InMeshRegistry.Get( InSnapshot.Meshes[Index] );

                    if ( P != nullptr and M != nullptr )
                    {
                        return { .Pipeline=P, .Mesh=M, .Handle=InSnapshot.Shaders[Index] };
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

                    SetupGraphicsState( InCmd, Pipeline, GlobalSet );

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
                const FBatchResources Batch = GetFirstValidBatch( InSnapshot, InCount, InMeshRegistry, InPipelineRegistry );

                if ( Batch.Pipeline == nullptr )
                {
                    return;
                }

                /** INFO: 1. Data Upload & Compute Frustum Culling */
                InSceneBuffer.Upload( InSnapshot, InMeshRegistry, InPipelineRegistry, InFrameIndex );

                const VkDescriptorSet &GlobalSet = InMemory.GetGlobalDescriptorSet( InFrameIndex );
                InCullingPass.Execute( InCmd, GlobalSet, InSceneBuffer, InIndirectBuffer, InFrameIndex );

                /** INFO: 2. State Setup & Geometry Binding */
                SetupGraphicsState( InCmd, Batch.Pipeline, GlobalSet );
                BindGeometry( InCmd, Batch.Mesh );

                /** INFO: 3. Final Indirect Dispatch */
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

            /** INFO: Select logic path; fallback is only used if the culling shader failed to initialize */
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