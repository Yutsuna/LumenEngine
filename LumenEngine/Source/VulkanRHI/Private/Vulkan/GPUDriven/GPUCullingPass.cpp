/**
 * @file GPUCullingPass.cpp
 * @brief Implementation of the GPU frustum-culling compute pass.
 */

#include "Vulkan/GPUDriven/GPUCullingPass.hpp"

#include "Vulkan/GPUDriven/GPUIndirectBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"
#include "Vulkan/VulkanCore.hpp"

#include "Logging/Logger.hpp"

namespace
{

[[nodiscard]] VkShaderModule CreateShaderModule ( VkDevice InDevice, const LumenEngine::RHI::FShaderByteCode &InSpirV )
{
    VkShaderModuleCreateInfo ShaderCI{ .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                       .pNext    = nullptr,
                                       .flags    = 0,
                                       .codeSize = InSpirV.size() * sizeof( LumenEngine::UInt32 ),
                                       .pCode    = InSpirV.data() };

    VkShaderModule Module = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateShaderModule( InDevice, &ShaderCI, nullptr, &Module ) );
    return Module;
}

void ResetIndirectDrawCount ( VkCommandBuffer InCmd, VkBuffer InCountBuffer )
{
    vkCmdFillBuffer( InCmd, InCountBuffer, 0, sizeof( LumenEngine::UInt32 ), 0U );
}

void InsertFillToComputeBarrier ( VkCommandBuffer InCmd, VkBuffer InCountBuffer )
{
    VkBufferMemoryBarrier2 FillBarrier{ .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                                        .pNext               = nullptr,
                                        .srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                        .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                        .dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                        .dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        .buffer              = InCountBuffer,
                                        .offset              = 0,
                                        .size                = VK_WHOLE_SIZE };

    VkDependencyInfo FillDep{ .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                              .pNext                    = nullptr,
                              .dependencyFlags          = 0,
                              .memoryBarrierCount       = 0,
                              .pMemoryBarriers          = nullptr,
                              .bufferMemoryBarrierCount = 1,
                              .pBufferMemoryBarriers    = &FillBarrier,
                              .imageMemoryBarrierCount  = 0,
                              .pImageMemoryBarriers     = nullptr };

    vkCmdPipelineBarrier2( InCmd, &FillDep );
}

} // namespace

LumenEngine::Bool LumenEngine::VulkanRHI::FGPUCullingPass::Initialize ( VkDevice InDevice,
                                                                        VkDescriptorSetLayout InGlobalSetLayout,
                                                                        VkDescriptorSetLayout InSceneSetLayout,
                                                                        VkDescriptorSetLayout InCullSetLayout,
                                                                        const RHI::FShaderByteCode &InSpirV )
{
    if ( InSpirV.empty() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "GPUCullingPass: Empty SPIR-V blob provided." );
        return false;
    }

    VkShaderModule ShaderModule               = CreateShaderModule( InDevice, InSpirV );
    const VkDescriptorSetLayout SetLayouts[3] = { InGlobalSetLayout, InSceneSetLayout, InCullSetLayout };
    const VkPushConstantRange PCRange         = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof( FGPUCullPushConstants ) };

    VkPipelineLayoutCreateInfo LayoutCreateInfo{ .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                 .pNext                  = nullptr,
                                                 .flags                  = 0,
                                                 .setLayoutCount         = 3,
                                                 .pSetLayouts            = SetLayouts,
                                                 .pushConstantRangeCount = 1,
                                                 .pPushConstantRanges    = &PCRange };

    LUMEN_VK_CHECK( vkCreatePipelineLayout( InDevice, &LayoutCreateInfo, nullptr, &CullLayout ) );

    VkComputePipelineCreateInfo PipelineCreateInfo{ .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                                                    .pNext              = nullptr,
                                                    .flags              = 0,
                                                    .stage              = { .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                            .pNext               = nullptr,
                                                                            .flags               = 0,
                                                                            .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
                                                                            .module              = ShaderModule,
                                                                            .pName               = "main",
                                                                            .pSpecializationInfo = nullptr },
                                                    .layout             = CullLayout,
                                                    .basePipelineHandle = VK_NULL_HANDLE,
                                                    .basePipelineIndex  = -1 };

    LUMEN_VK_CHECK( vkCreateComputePipelines( InDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &CullPipeline ) );

    /** Cleanup temporary module */
    vkDestroyShaderModule( InDevice, ShaderModule, nullptr );

    return true;
}

void LumenEngine::VulkanRHI::FGPUCullingPass::Shutdown ( VkDevice InDevice ) noexcept
{
    if ( CullPipeline != VK_NULL_HANDLE )
    {
        vkDestroyPipeline( InDevice, CullPipeline, nullptr );
        CullPipeline = VK_NULL_HANDLE;
    }
    if ( CullLayout != VK_NULL_HANDLE )
    {
        vkDestroyPipelineLayout( InDevice, CullLayout, nullptr );
        CullLayout = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FGPUCullingPass::Execute ( VkCommandBuffer InCmd,
                                                        VkDescriptorSet InGlobalDescSet,
                                                        const FGPUSceneBuffer &InSceneBuffer,
                                                        const FGPUIndirectBuffer &InIndirectBuffer,
                                                        UInt32 InFrameIndex ) const noexcept
{
    if ( not IsReady() )
    {
        return;
    }

    const UInt32 InstanceCount = InSceneBuffer.GetLastInstanceCount();
    if ( InstanceCount == 0U )
    {
        ResetIndirectDrawCount( InCmd, InIndirectBuffer.GetCountBuffer() );
        return;
    }

    InIndirectBuffer.InsertWriteBarrier( InCmd );
    ResetIndirectDrawCount( InCmd, InIndirectBuffer.GetCountBuffer() );
    InsertFillToComputeBarrier( InCmd, InIndirectBuffer.GetCountBuffer() );

    vkCmdBindPipeline( InCmd, VK_PIPELINE_BIND_POINT_COMPUTE, CullPipeline );

    const VkDescriptorSet DescSets[3] = { InGlobalDescSet, InSceneBuffer.GetDescriptorSet( InFrameIndex ), InIndirectBuffer.GetCullDescriptorSet() };
    vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_COMPUTE, CullLayout, 0, 3, DescSets, 0, nullptr );

    FGPUCullPushConstants PC{ .InstanceCount = InstanceCount, .bDisableCulling = 0U };
    vkCmdPushConstants( InCmd, CullLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( FGPUCullPushConstants ), &PC );

    const UInt32 GroupCount = ( InstanceCount + WorkgroupSize - 1U ) / WorkgroupSize;
    vkCmdDispatch( InCmd, GroupCount, 1U, 1U );

    /** Read Barrier: Compute -> DrawIndirect */
    InIndirectBuffer.InsertReadBarrier( InCmd );
}

LumenEngine::Bool LumenEngine::VulkanRHI::FGPUCullingPass::IsReady () const noexcept
{
    return CullPipeline != VK_NULL_HANDLE;
}