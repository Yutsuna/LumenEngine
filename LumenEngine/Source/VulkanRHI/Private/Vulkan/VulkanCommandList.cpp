/**
 * @file VulkanCommandList.cpp
 * @brief Implementation of the Vulkan command list.
 */

#include "Vulkan/VulkanCommandList.hpp"
#include "Vulkan/VulkanRHI.hpp"

LumenEngine::VulkanRHI::FVulkanCommandList::FVulkanCommandList ( FVulkanRHI *InRHI ) noexcept : RHI( InRHI )
{
    /* Ctor */
}

void LumenEngine::VulkanRHI::FVulkanCommandList::BeginRendering ( const Float32 InClearColor[4] )
{
    RHI->BeginRenderingInternal( CurrentCmd, InClearColor );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::EndRendering ()
{
    vkCmdEndRendering( CurrentCmd );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::BindPipeline ( const RHI::FPipelineHandle InPipeline )
{
    RHI->BindPipelineInternal( CurrentCmd, InPipeline );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::PushConstants ( const RHI::FPipelineHandle InPipeline, const void *InData, UInt32 InSize, UInt32 InOffset )
{
    RHI->PushConstantsInternal( CurrentCmd, InPipeline, InData, InSize, InOffset );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::DrawMesh ( const RHI::FMeshHandle InMesh )
{
    RHI->DrawMeshInternal( CurrentCmd, InMesh );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::DrawScene ( const RHI::FSceneSnapshot &InSceneSnapshot, const Float32 InClearColor[4] )
{
    RHI->DrawSceneInternal( CurrentCmd, InSceneSnapshot, InClearColor );
}

void LumenEngine::VulkanRHI::FVulkanCommandList::SetActiveCommandBuffer ( VkCommandBuffer InCmd ) noexcept
{
    CurrentCmd = InCmd;
}