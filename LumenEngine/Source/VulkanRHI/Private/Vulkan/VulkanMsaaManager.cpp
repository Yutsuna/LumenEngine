/**
 * @file VulkanMsaaManager.cpp
 * @brief Implementation of the FVulkanMsaaManager class.
 */

#include "Vulkan/VulkanMsaaManager.hpp"
#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanMsaaRenderTarget.hpp"

namespace
{

void GetMaxUsableSampleCount ( VkPhysicalDevice InPhysicalDevice, VkSampleCountFlagBits &OutMaxSamples ) noexcept
{
    VkPhysicalDeviceProperties Props;
    vkGetPhysicalDeviceProperties( InPhysicalDevice, &Props );

    VkSampleCountFlags Counts = Props.limits.framebufferColorSampleCounts & Props.limits.framebufferDepthSampleCounts;
    OutMaxSamples             = VK_SAMPLE_COUNT_1_BIT;

    if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_64_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_64_BIT;
    }
    else if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_32_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_32_BIT;
    }
    else if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_16_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_16_BIT;
    }
    else if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_8_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_8_BIT;
    }
    else if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_4_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_4_BIT;
    }
    else if ( static_cast<LumenEngine::Bool>( Counts & VK_SAMPLE_COUNT_2_BIT ) )
    {
        OutMaxSamples = VK_SAMPLE_COUNT_2_BIT;
    }
}

void GetRequestedSampleCount ( LumenEngine::RHI::EMsaaLevel InMsaaLevel, VkSampleCountFlagBits &OutRequestedSamples ) noexcept
{
    switch ( InMsaaLevel )
    {
    case LumenEngine::RHI::EMsaaLevel::MSAA_2x:
        OutRequestedSamples = VK_SAMPLE_COUNT_2_BIT;
        break;
    case LumenEngine::RHI::EMsaaLevel::MSAA_4x:
        OutRequestedSamples = VK_SAMPLE_COUNT_4_BIT;
        break;
    case LumenEngine::RHI::EMsaaLevel::MSAA_8x:
        OutRequestedSamples = VK_SAMPLE_COUNT_8_BIT;
        break;
    default:
        OutRequestedSamples = VK_SAMPLE_COUNT_1_BIT;
        break;
    }
}

} // namespace

void LumenEngine::VulkanRHI::FVulkanMsaaManager::Initialize (
    VkPhysicalDevice InPhysicalDevice, VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent, const RHI::FVisualSettings &InSettings )
{
    CurrentSettings = InSettings;
    UpdateMsaaSamples( InPhysicalDevice );
    MsaaRenderTarget.Create( InAllocator, InDevice, InFormat, InExtent, ActiveMsaaSamples );
}

void LumenEngine::VulkanRHI::FVulkanMsaaManager::Shutdown ( VmaAllocator InAllocator, VkDevice InDevice ) noexcept
{
    MsaaRenderTarget.Destroy( InAllocator, InDevice );
}

void LumenEngine::VulkanRHI::FVulkanMsaaManager::SetVisualSettings ( const RHI::FVisualSettings &InSettings,
                                                                     VkPhysicalDevice InPhysicalDevice,
                                                                     VkDevice InDevice,
                                                                     VmaAllocator InAllocator,
                                                                     VkFormat InFormat,
                                                                     VkExtent2D InExtent,
                                                                     FDeferredDestructionQueue &InDestructionQueue,
                                                                     RHI::TResourceRegistry<FVulkanPipeline, RHI::FPipelineTag> &InPipelineRegistry,
                                                                     UInt64 InAbsoluteFrameIndex )
{
    if ( CurrentSettings.MsaaLevel == InSettings.MsaaLevel )
    {
        return;
    }

    CurrentSettings = InSettings;
    UpdateMsaaSamples( InPhysicalDevice );

    VkImage OldMsaaImage            = VK_NULL_HANDLE;
    VkImageView OldMsaaView         = VK_NULL_HANDLE;
    VmaAllocation OldMsaaAllocation = VK_NULL_HANDLE;

    MsaaRenderTarget.ReleaseOwnership( OldMsaaImage, OldMsaaView, OldMsaaAllocation );

    if ( OldMsaaImage != VK_NULL_HANDLE or OldMsaaView != VK_NULL_HANDLE )
    {
        InDestructionQueue.Enqueue(
            [InDevice, InAllocator, OldMsaaImage, OldMsaaView, OldMsaaAllocation] ()
            {
                if ( OldMsaaView != VK_NULL_HANDLE )
                {
                    vkDestroyImageView( InDevice, OldMsaaView, nullptr );
                }
                if ( OldMsaaImage != VK_NULL_HANDLE )
                {
                    vmaDestroyImage( InAllocator, OldMsaaImage, OldMsaaAllocation );
                }
            },
            InAbsoluteFrameIndex );
    }

    MsaaRenderTarget.Create( InAllocator, InDevice, InFormat, InExtent, ActiveMsaaSamples );

    InPipelineRegistry.ForEach(
        [InDevice, &InDestructionQueue, InAbsoluteFrameIndex, this] ( FVulkanPipeline &Pipeline )
        {
            VkPipeline OldPipeline     = VK_NULL_HANDLE;
            VkPipelineLayout OldLayout = VK_NULL_HANDLE;

            if ( Pipeline.Recreate( InDevice, ActiveMsaaSamples, OldPipeline, OldLayout ).has_value() )
            {
                LUMEN_LOG_INFO( LogVulkanRHI, "Pipeline recreation succeeded for handle {:#x} with new MSAA samples.",
                                reinterpret_cast<uintptr_t>( Pipeline.GetPipelineHandle() ) );
            }
            else
            {
                LUMEN_LOG_ERROR( LogVulkanRHI, "Pipeline recreation failed for handle {:#x}.", reinterpret_cast<uintptr_t>( Pipeline.GetPipelineHandle() ) );
            }

            if ( OldPipeline != VK_NULL_HANDLE or OldLayout != VK_NULL_HANDLE )
            {
                InDestructionQueue.Enqueue(
                    [InDevice, OldPipeline, OldLayout] ()
                    {
                        if ( OldPipeline != VK_NULL_HANDLE )
                        {
                            vkDestroyPipeline( InDevice, OldPipeline, nullptr );
                        }
                        if ( OldLayout != VK_NULL_HANDLE )
                        {
                            vkDestroyPipelineLayout( InDevice, OldLayout, nullptr );
                        }
                    },
                    InAbsoluteFrameIndex );
            }
        } );
}

void LumenEngine::VulkanRHI::FVulkanMsaaManager::RecreateRenderTargetIfNeeded ( VmaAllocator InAllocator, VkDevice InDevice, VkFormat InFormat, VkExtent2D InExtent )
{
    MsaaRenderTarget.RecreateIfNeeded( InAllocator, InDevice, InFormat, InExtent, ActiveMsaaSamples );
}

VkSampleCountFlagBits LumenEngine::VulkanRHI::FVulkanMsaaManager::GetActiveSamples () const noexcept
{
    return ActiveMsaaSamples;
}

const LumenEngine::VulkanRHI::FVulkanMsaaRenderTarget &LumenEngine::VulkanRHI::FVulkanMsaaManager::GetRenderTarget () const noexcept
{
    return MsaaRenderTarget;
}

const LumenEngine::RHI::FVisualSettings &LumenEngine::VulkanRHI::FVulkanMsaaManager::GetCurrentSettings () const noexcept
{
    return CurrentSettings;
}

void LumenEngine::VulkanRHI::FVulkanMsaaManager::UpdateMsaaSamples ( VkPhysicalDevice InPhysicalDevice ) noexcept
{
    VkSampleCountFlagBits MaxSamples       = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits RequestedSamples = VK_SAMPLE_COUNT_1_BIT;

    GetMaxUsableSampleCount( InPhysicalDevice, MaxSamples );
    GetRequestedSampleCount( CurrentSettings.MsaaLevel, RequestedSamples );

    ActiveMsaaSamples = ( static_cast<UInt32>( RequestedSamples ) > static_cast<UInt32>( MaxSamples ) ) ? MaxSamples : RequestedSamples;

    LUMEN_LOG_INFO( LogVulkanRHI, "MSAA samples configuration set to VK_SAMPLE_COUNT_{}_BIT.", static_cast<UInt32>( ActiveMsaaSamples ) );
}
