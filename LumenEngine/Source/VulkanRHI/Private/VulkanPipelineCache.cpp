/**
 * @file VulkanPipelineCache.cpp
 * @brief Implementation of the FVulkanPipelineCache class
 */

#include "VulkanPipelineCache.hpp"
#include "VulkanDevice.hpp"
#include "VulkanCore.hpp"

namespace LumenEngine
{
namespace RHI
{
namespace
{
    /** Simple hash combiner */
    template <typename Type> inline void HashCombine ( UInt64 &Seed, const Type &Value )
    {
        Seed ^= std::hash<Type>{}( Value ) + 0x9e3779b9 + ( Seed << 6 ) + ( Seed >> 2 );
    }
} // namespace
} // namespace RHI
} // namespace LumenEngine

LumenEngine::RHI::FVulkanPipelineCache::FVulkanPipelineCache ( FVulkanDevice &InDevice ) noexcept
    : Device( InDevice )
{
    VkPipelineCacheCreateInfo CacheCreateInfo{};
    CacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VULKAN_CHECK( vkCreatePipelineCache( Device.GetLogicalDevice(), &CacheCreateInfo, nullptr, &PipelineCache ) );
}

LumenEngine::RHI::FVulkanPipelineCache::~FVulkanPipelineCache () noexcept
{
    Clear();
    if ( PipelineCache != VK_NULL_HANDLE )
    {
        vkDestroyPipelineCache( Device.GetLogicalDevice(), PipelineCache, nullptr );
        PipelineCache = VK_NULL_HANDLE;
    }
}

VkPipeline LumenEngine::RHI::FVulkanPipelineCache::GetOrCreateGraphicsPipeline ( const VkGraphicsPipelineCreateInfo &InCreateInfo )
{
    UInt64 PipelineHash = HashGraphicsPipeline( InCreateInfo );

    auto Iterator = GraphicsPipelines.find( PipelineHash );
    if ( Iterator != GraphicsPipelines.end() )
    {
        return Iterator->second;
    }

    VkPipeline Pipeline = VK_NULL_HANDLE;
    VULKAN_CHECK( vkCreateGraphicsPipelines( Device.GetLogicalDevice(), PipelineCache, 1, &InCreateInfo, nullptr, &Pipeline ) );

    GraphicsPipelines[PipelineHash] = Pipeline;
    return Pipeline;
}

VkPipeline LumenEngine::RHI::FVulkanPipelineCache::GetOrCreateComputePipeline ( const VkComputePipelineCreateInfo &InCreateInfo )
{
    UInt64 PipelineHash = HashComputePipeline( InCreateInfo );

    auto Iterator = ComputePipelines.find( PipelineHash );
    if ( Iterator != ComputePipelines.end() )
    {
        return Iterator->second;
    }

    VkPipeline Pipeline = VK_NULL_HANDLE;
    VULKAN_CHECK( vkCreateComputePipelines( Device.GetLogicalDevice(), PipelineCache, 1, &InCreateInfo, nullptr, &Pipeline ) );

    ComputePipelines[PipelineHash] = Pipeline;
    return Pipeline;
}

void LumenEngine::RHI::FVulkanPipelineCache::Clear ()
{
    for ( auto &[Hash, Pipeline] : GraphicsPipelines )
    {
        vkDestroyPipeline( Device.GetLogicalDevice(), Pipeline, nullptr );
    }
    GraphicsPipelines.clear();

    for ( auto &[Hash, Pipeline] : ComputePipelines )
    {
        vkDestroyPipeline( Device.GetLogicalDevice(), Pipeline, nullptr );
    }
    ComputePipelines.clear();
}

LumenEngine::UInt64 LumenEngine::RHI::FVulkanPipelineCache::HashGraphicsPipeline ( const VkGraphicsPipelineCreateInfo &InCreateInfo ) const noexcept
{
    UInt64 HashValue = 0;
    LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.layout );
    LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.renderPass );
    LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.subpass );

    // Shader stages
    for ( UInt32 Index = 0; Index < InCreateInfo.stageCount; ++Index )
    {
        LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.pStages[Index].module );
        LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.pStages[Index].stage );
    }

    // Fixed functions (partial hashing for brevity)
    if ( InCreateInfo.pInputAssemblyState )
    {
        LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.pInputAssemblyState->topology );
    }

    if ( InCreateInfo.pRasterizationState )
    {
        LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.pRasterizationState->polygonMode );
        LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.pRasterizationState->cullMode );
    }

    return HashValue;
}

LumenEngine::UInt64 LumenEngine::RHI::FVulkanPipelineCache::HashComputePipeline ( const VkComputePipelineCreateInfo &InCreateInfo ) const noexcept
{
    UInt64 HashValue = 0;
    LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.layout );
    LumenEngine::RHI::HashCombine( HashValue, InCreateInfo.stage.module );
    return HashValue;
}
