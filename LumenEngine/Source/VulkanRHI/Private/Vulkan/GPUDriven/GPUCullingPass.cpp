/**
 * @file GPUCullingPass.cpp
 * @brief Implementation of the GPU frustum-culling compute pass.
 */

#include "Vulkan/GPUDriven/GPUCullingPass.hpp"

#include "Vulkan/GPUDriven/GPUCullPushConstants.hpp"
#include "Vulkan/GPUDriven/GPUIndirectBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneBuffer.hpp"
#include "Vulkan/GPUDriven/GPUSceneTypes.hpp"

#include "Vulkan/VulkanCore.hpp"

#include "Container/File.hpp"
#include "Container/Vector.hpp"

#include "Logging/Logger.hpp"

#include <vulkan/vulkan_core.h>

namespace
{

[[nodiscard]] inline TOptional<TVector<UInt8>> ReadComputeShaderCode ( const FString &InShaderPath )
{
    return FIOFile::ReadAllBytes<UInt8>( InShaderPath );
}

[[nodiscard]] VkShaderModule CreateShaderModule ( VkDevice InDevice, const TVector<UInt8> &InShaderCode )
{
    VkShaderModuleCreateInfo ShaderCI{};
    ShaderCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderCI.codeSize = InShaderCode.size();
    ShaderCI.pCode    = reinterpret_cast<const UInt32 *>( InShaderCode.data() );

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateShaderModule( InDevice, &ShaderCI, nullptr, &ShaderModule ) );

    return ShaderModule;
}

[[nodiscard]] VkPipelineLayout
CreateCullPipelineLayout ( VkDevice InDevice, VkDescriptorSetLayout InGlobalSetLayout, VkDescriptorSetLayout InSceneSetLayout, VkDescriptorSetLayout InCullSetLayout )
{
    const VkDescriptorSetLayout SetLayouts[3] = {
        InGlobalSetLayout,
        InSceneSetLayout,
        InCullSetLayout,
    };

    VkPushConstantRange PCRange{};
    PCRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    PCRange.offset     = 0;
    PCRange.size       = sizeof( FGPUCullPushConstants );

    VkPipelineLayoutCreateInfo LayoutCI{};
    LayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LayoutCI.setLayoutCount         = 3;
    LayoutCI.pSetLayouts            = SetLayouts;
    LayoutCI.pushConstantRangeCount = 1;
    LayoutCI.pPushConstantRanges    = &PCRange;

    VkPipelineLayout Layout = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreatePipelineLayout( InDevice, &LayoutCI, nullptr, &Layout ) );

    return Layout;
}

[[nodiscard]] VkPipeline CreateCullComputePipeline ( VkDevice InDevice, VkPipelineLayout InPipelineLayout, VkShaderModule InShaderModule )
{
    VkPipelineShaderStageCreateInfo StageCI{};
    StageCI.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    StageCI.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    StageCI.module = InShaderModule;
    StageCI.pName  = "main";

    VkComputePipelineCreateInfo PipelineCI{};
    PipelineCI.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCI.stage  = StageCI;
    PipelineCI.layout = InPipelineLayout;

    VkPipeline Pipeline = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateComputePipelines( InDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &Pipeline ) );

    return Pipeline;
}

void ResetIndirectDrawCount ( VkCommandBuffer InCmd, VkBuffer InCountBuffer )
{
    vkCmdFillBuffer( InCmd, InCountBuffer, 0, sizeof( UInt32 ), 0U );
}

void InsertFillToComputeBarrier ( VkCommandBuffer InCmd, VkBuffer InCountBuffer )
{
    VkBufferMemoryBarrier2 FillBarrier{};
    FillBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    FillBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    FillBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    FillBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    FillBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    FillBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    FillBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    FillBarrier.buffer              = InCountBuffer;
    FillBarrier.offset              = 0;
    FillBarrier.size                = VK_WHOLE_SIZE;

    VkDependencyInfo FillDep{};
    FillDep.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    FillDep.bufferMemoryBarrierCount = 1;
    FillDep.pBufferMemoryBarriers    = &FillBarrier;

    vkCmdPipelineBarrier2( InCmd, &FillDep );
}

[[nodiscard]] inline UInt32 ComputeDispatchGroupCount ( UInt32 InInstanceCount, UInt32 InWorkgroupSize )
{
    return ( InInstanceCount + InWorkgroupSize - 1U ) / InWorkgroupSize;
}

} // namespace

bool LumenEngine::VulkanRHI::FGPUCullingPass::Initialize ( VkDevice InDevice,
                                                           VkDescriptorSetLayout InGlobalSetLayout,
                                                           VkDescriptorSetLayout InSceneSetLayout,
                                                           VkDescriptorSetLayout InCullSetLayout,
                                                           const FString &InShaderPath )
{
    TOptional<TVector<UInt8>> ShaderCode = ReadComputeShaderCode( InShaderPath );

    if ( not ShaderCode.has_value() or ShaderCode->empty() )
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "GPUCullingPass: failed to load compute shader '{}'.", InShaderPath.c_str() );
        return false;
    }

    VkShaderModule ShaderModule = CreateShaderModule( InDevice, ShaderCode.value() );

    CullLayout = CreateCullPipelineLayout( InDevice, InGlobalSetLayout, InSceneSetLayout, InCullSetLayout );

    CullPipeline = CreateCullComputePipeline( InDevice, CullLayout, ShaderModule );

    vkDestroyShaderModule( InDevice, ShaderModule, nullptr );

    LUMEN_LOG_INFO( LogVulkanRHI, "GPUCullingPass: compute pipeline created (workgroup_size={}x1x1).", WorkgroupSize );

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
    if ( CullPipeline == VK_NULL_HANDLE )
    {
        return;
    }

    const UInt32 InstanceCount = InSceneBuffer.GetLastInstanceCount();

    if ( InstanceCount == 0U )
    {
        ResetIndirectDrawCount( InCmd, InIndirectBuffer.GetCountBuffer() );
        return;
    }

    /** INFO: Transition the indirect buffer from read-only to write-only */
    InIndirectBuffer.InsertWriteBarrier( InCmd );

    /** INFO: Zero the draw count atomically before the shader increments it */
    const VkBuffer CountBuffer = InIndirectBuffer.GetCountBuffer();
    ResetIndirectDrawCount( InCmd, CountBuffer );
    InsertFillToComputeBarrier( InCmd, CountBuffer );

    /** INFO: Bind the compute pipeline */
    vkCmdBindPipeline( InCmd, VK_PIPELINE_BIND_POINT_COMPUTE, CullPipeline );

    /** INFO: Bind descriptor sets */
    const VkDescriptorSet DescSets[3] = {
        InGlobalDescSet,
        InSceneBuffer.GetDescriptorSet( InFrameIndex ),
        InIndirectBuffer.GetCullDescriptorSet(),
    };

    vkCmdBindDescriptorSets( InCmd, VK_PIPELINE_BIND_POINT_COMPUTE, CullLayout, 0, 3, DescSets, 0, nullptr );

    /** INFO: Set push constants */
    FGPUCullPushConstants PC{};
    PC.InstanceCount   = InstanceCount;
    PC.bDisableCulling = 0U;

    vkCmdPushConstants( InCmd, CullLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( FGPUCullPushConstants ), &PC );

    /** INFO: Dispatch compute shader ( InstanceCount  / WorkgroupSize ) */
    const UInt32 GroupCount = ComputeDispatchGroupCount( InstanceCount, WorkgroupSize );

    vkCmdDispatch( InCmd, GroupCount, 1U, 1U );

    /** INFO: Insert read barrier; compute-write -> indirect-draw-read */
    InIndirectBuffer.InsertReadBarrier( InCmd );
}

bool LumenEngine::VulkanRHI::FGPUCullingPass::IsReady () const noexcept
{
    return CullPipeline != VK_NULL_HANDLE;
}