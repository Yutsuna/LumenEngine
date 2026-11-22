/**
 * @file VulkanFeatureNegotiation.cpp
 * @brief Implements Vulkan device capability/limit gate and feature-chain assembly.
 */

#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHI/Types/RHIQueueType.hpp>

#include <RHIVulkan/Device/VulkanFeatureNegotiation.hpp>
#include <RHIVulkan/Module/VulkanAdapter.hpp>

#include <Algo/Find.hpp>

/**
 * Private
 */

namespace
{

Lumen::TOptional<Lumen::UInt32> FindQueueFamilyIndex ( const Lumen::RHIVulkan::FVulkanAdapter &InAdapter,
                                                       Lumen::RHI::ERHIQueueType InType ) noexcept
{
    switch ( InType )
    {
    case Lumen::RHI::ERHIQueueType::Graphics:
        return InAdapter.GraphicsQueueFamilyIndex;
    case Lumen::RHI::ERHIQueueType::Compute:
        return InAdapter.ComputeQueueFamilyIndex;
    case Lumen::RHI::ERHIQueueType::Copy:
        return InAdapter.CopyQueueFamilyIndex;
    default:
        return {};
    }
}

void AppendUniqueFamilyIndex ( Lumen::TVector<Lumen::UInt32> &InOutFamilies, Lumen::UInt32 InFamilyIndex ) noexcept
{
    if ( Lumen::Algo::Find( InOutFamilies, InFamilyIndex ) != nullptr )
    {
        return;
    }

    InOutFamilies.push_back( InFamilyIndex );
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::RHIVulkan::FVulkanNegotiatedDeviceConfig, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::NegotiateVulkanDeviceFeatures ( const FVulkanAdapter &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( DescValidation, InDesc.Validate() );
    LUMEN_EXPECT( CapValidation, InDesc.RequiredCapabilities.Validate() );

    const RHI::FRHICapabilities &Supported = InAdapter.Info.Capabilities;
    const RHI::FRHICapabilities &Required  = InDesc.RequiredCapabilities;

    /** TODO(M0-R1): delegate feature gating to public RHI::CheckRHICapabilityGate once RHIBackendSupport.hpp lands. */
    if ( not Supported.Supports( Required.Features ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability,
                                                     "Selected Vulkan adapter lacks requested capability flags." ) );
    }

    if ( Required.ShaderModelMajor > Supported.ShaderModelMajor or
         ( Required.ShaderModelMajor == Supported.ShaderModelMajor and Required.ShaderModelMinor > Supported.ShaderModelMinor ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability,
                                                     "Selected Vulkan adapter lacks requested shader model version." ) );
    }

    FVulkanNegotiatedDeviceConfig Config{};
    Config.NegotiatedCapabilities = Supported;
    Config.NegotiatedLimits       = InAdapter.Info.Limits;

    for ( const RHI::FRHIQueueRequest &Request : InDesc.Queues )
    {
        const RHI::FRHIQueueFamilyInfo *MatchingFamily = nullptr;

        for ( const RHI::FRHIQueueFamilyInfo &Family : InAdapter.Info.QueueFamilies )
        {
            if ( Family.Type == Request.Type )
            {
                MatchingFamily = &Family;
                break;
            }
        }

        if ( MatchingFamily == nullptr )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue,
                                                         "Requested queue type is not exposed by this Vulkan adapter." ) );
        }

        if ( Request.Count > MatchingFamily->Count )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Limit,
                                                         "Requested queue count exceeds supported Vulkan queue family count." ) );
        }

        const TOptional<UInt32> NativeFamily = FindQueueFamilyIndex( InAdapter, Request.Type );
        if ( not NativeFamily.HasValue() )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue,
                                                         "Requested queue family has no backing Vulkan queue index." ) );
        }

        switch ( Request.Type )
        {
        case RHI::ERHIQueueType::Graphics:
            Config.GraphicsQueueFamilyIndex.Emplace( *NativeFamily );
            break;
        case RHI::ERHIQueueType::Compute:
            Config.ComputeQueueFamilyIndex.Emplace( *NativeFamily );
            break;
        case RHI::ERHIQueueType::Copy:
            Config.CopyQueueFamilyIndex.Emplace( *NativeFamily );
            break;
        default:
            return TUnexpected( RHI::MakeRHIInvalidQueue( "Unsupported ERHIQueueType value." ) );
        }

        AppendUniqueFamilyIndex( Config.UniqueQueueFamilyIndices, *NativeFamily );
    }

    /** NOTE: Always enable portability subset whenever the physical device exposes it (D2). */
    if ( InAdapter.bHasPortabilitySubset )
    {
        Config.EnabledExtensions.push_back( "VK_KHR_portability_subset" );
    }
    if ( InAdapter.bHasSwapchain )
    {
        Config.EnabledExtensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    }

    Config.Features13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    Config.Features13.dynamicRendering = VK_TRUE;
    Config.Features13.synchronization2 = VK_TRUE;

    Config.Features12.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Config.Features12.timelineSemaphore = VK_TRUE;
    if ( Supported.Supports( RHI::ERHICapabilityFlags::BindlessResources ) )
    {
        Config.Features12.descriptorIndexing                            = VK_TRUE;
        Config.Features12.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
        Config.Features12.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
        Config.Features12.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
        Config.Features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        Config.Features12.descriptorBindingPartiallyBound               = VK_TRUE;
        Config.Features12.descriptorBindingVariableDescriptorCount      = VK_TRUE;
        Config.Features12.runtimeDescriptorArray                        = VK_TRUE;
    }

    Config.Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    if ( Supported.Supports( RHI::ERHICapabilityFlags::MultiDrawIndirect ) )
    {
        Config.Features2.features.multiDrawIndirect = VK_TRUE;
    }
    if ( Supported.Supports( RHI::ERHICapabilityFlags::DepthClamp ) )
    {
        Config.Features2.features.depthClamp = VK_TRUE;
    }
    if ( Supported.Supports( RHI::ERHICapabilityFlags::AnisotropicFiltering ) )
    {
        Config.Features2.features.samplerAnisotropy = VK_TRUE;
    }
    if ( Supported.Supports( RHI::ERHICapabilityFlags::PipelineStatistics ) )
    {
        Config.Features2.features.pipelineStatisticsQuery = VK_TRUE;
    }

    return Config;
}
