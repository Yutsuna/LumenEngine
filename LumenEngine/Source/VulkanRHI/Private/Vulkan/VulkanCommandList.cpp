/**
 * @file VulkanCommandList.cpp
 * @brief Implementation of the Vulkan command list.
 */

#include "Vulkan/VulkanCommandList.hpp"
#include "Vulkan/VulkanRHI.hpp"

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

void LumenEngine::VulkanRHI::FVulkanCommandList::DrawMesh ( const RHI::FMeshHandle InMesh )
{
    RHI->DrawMeshInternal( CurrentCmd, InMesh );
}