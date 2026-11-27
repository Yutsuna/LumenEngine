/**
 * @file VulkanFeatureNegotiationTests.cpp
 * @brief Tests Vulkan feature chains and native queue negotiation without a driver.
 */

#include <RHIVulkan/Device/VulkanFeatureNegotiation.hpp>
#include <RHIVulkan/Module/VulkanAdapter.hpp>
#include <RHIVulkan/Module/VulkanAdapterBuilder.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    namespace
    {

        FVulkanAdapter MakeNegotiationAdapter ()
        {
            FVulkanAdapter Adapter;
            Adapter.Info.AdapterId   = 1ULL;
            Adapter.Info.Name        = "NegotiationAdapter";
            Adapter.Info.BackendName = "Vulkan";
            Adapter.Info.Type        = RHI::ERHIAdapterType::Discrete;
            Adapter.Info.Capabilities.Features =
                RHI::ERHICapabilityFlags::DynamicRendering | RHI::ERHICapabilityFlags::TimelineSemaphores |
                RHI::ERHICapabilityFlags::BindlessResources | RHI::ERHICapabilityFlags::IndirectDraw |
                RHI::ERHICapabilityFlags::OcclusionQueries | RHI::ERHICapabilityFlags::MultiDrawIndirect |
                RHI::ERHICapabilityFlags::DepthClamp | RHI::ERHICapabilityFlags::AnisotropicFiltering |
                RHI::ERHICapabilityFlags::PipelineStatistics;
            Adapter.Info.Capabilities.ShaderFormats = { RHI::ERHIShaderFormat::PortableBytecode };
            Adapter.Info.Limits                     = {
                                    .MaxBufferSize           = 1ULL << 20U,
                                    .MaxTextureDimension1D   = 1024U,
                                    .MaxTextureDimension2D   = 1024U,
                                    .MaxTextureDimension3D   = 256U,
                                    .MaxTextureArrayLayers   = 256U,
                                    .MaxColorAttachments     = 8U,
                                    .MaxBindingSets          = 4U,
                                    .MaxDescriptorsPerSet    = 64U,
                                    .MaxInlineConstantBytes  = 128U,
                                    .MaxVertexAttributes     = 32U,
                                    .MaxVertexBuffers        = 8U,
                                    .MaxComputeWorkgroupSize = 1024U,
                                    .MinBufferAlignment      = 16ULL,
                                    .MinTextureRowAlignment  = 16ULL,
                                    .TimestampPeriod         = 1ULL,
            };
            Adapter.Info.QueueFamilies = {
                { .Type = RHI::ERHIQueueType::Graphics, .Count = 1U },
                { .Type = RHI::ERHIQueueType::Compute, .Count = 1U },
                { .Type = RHI::ERHIQueueType::Copy, .Count = 1U },
            };
            Adapter.GraphicsQueueFamilyIndex.Emplace( 0U );
            Adapter.ComputeQueueFamilyIndex.Emplace( 1U );
            Adapter.CopyQueueFamilyIndex.Emplace( 0U );
            Adapter.bHasPortabilitySubset = true;
            Adapter.bHasSwapchain         = true;
            Adapter.bHasBindlessIndexing  = true;
            return Adapter;
        }

    } // namespace

    LUMEN_TEST( VulkanFeatureNegotiation, BuildsFeatureChainAndDeduplicatesNativeFamilies )
    {
        const FVulkanAdapter Adapter = MakeNegotiationAdapter();
        RHI::FRHIDeviceDesc Desc;
        Desc.Queues = {
            { .Type = RHI::ERHIQueueType::Graphics, .Count = 1U },
            { .Type = RHI::ERHIQueueType::Compute, .Count = 1U },
            { .Type = RHI::ERHIQueueType::Copy, .Count = 1U },
        };

        const TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError> Config = NegotiateVulkanDeviceFeatures( Adapter, Desc );
        LUMEN_CHECK_TRUE( Config.HasValue() );
        LUMEN_CHECK_TRUE( Config->GraphicsQueueFamilyIndex.HasValue() );
        LUMEN_CHECK_EQ( *Config->GraphicsQueueFamilyIndex, 0U );
        LUMEN_CHECK_EQ( *Config->ComputeQueueFamilyIndex, 1U );
        LUMEN_CHECK_EQ( *Config->CopyQueueFamilyIndex, 0U );
        LUMEN_CHECK_EQ( Config->UniqueQueueFamilyIndices.size(), 2U );
        LUMEN_CHECK_EQ( Config->EnabledExtensions.size(), 2U );
        LUMEN_CHECK_EQ( Config->NegotiatedCapabilities.Features, Adapter.Info.Capabilities.Features );
        LUMEN_CHECK_EQ( Config->NegotiatedLimits.MaxBufferSize, Adapter.Info.Limits.MaxBufferSize );

        LUMEN_CHECK_EQ( Config->Features13.dynamicRendering, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features13.synchronization2, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features12.timelineSemaphore, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features12.descriptorIndexing, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features12.descriptorBindingPartiallyBound, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features12.descriptorBindingVariableDescriptorCount, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features12.runtimeDescriptorArray, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features2.features.multiDrawIndirect, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features2.features.depthClamp, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features2.features.samplerAnisotropy, VK_TRUE );
        LUMEN_CHECK_EQ( Config->Features2.features.pipelineStatisticsQuery, VK_TRUE );
    }

    LUMEN_TEST( VulkanFeatureNegotiation, PublishesOnlyVerifiedNativeFeatures )
    {
        FVulkanAdapter Adapter = MakeNegotiationAdapter();
        Adapter.ComputeQueueFamilyIndex.Reset();
        Adapter.bHasBindlessIndexing = false;
        VkPhysicalDeviceFeatures2 Features{};
        VkPhysicalDeviceLimits Limits{};
        Limits.maxSamplerAnisotropy = 1.0F;

        const RHI::ERHICapabilityFlags OptionalFlags =
            RHI::ERHICapabilityFlags::BindlessResources | RHI::ERHICapabilityFlags::MultiDrawIndirect |
            RHI::ERHICapabilityFlags::TimestampQueries | RHI::ERHICapabilityFlags::PipelineStatistics |
            RHI::ERHICapabilityFlags::DepthClamp | RHI::ERHICapabilityFlags::AnisotropicFiltering |
            RHI::ERHICapabilityFlags::AsyncCompute | RHI::ERHICapabilityFlags::Presentation;
        const RHI::ERHICapabilityFlags CoreFlags =
            RHI::ERHICapabilityFlags::DynamicRendering | RHI::ERHICapabilityFlags::TimelineSemaphores |
            RHI::ERHICapabilityFlags::IndirectDraw | RHI::ERHICapabilityFlags::OcclusionQueries;
        const RHI::ERHICapabilityFlags Baseline = BuildVulkanFeatureFlags( Adapter, Features, Limits, false, false, false );
        LUMEN_CHECK_TRUE( ( Baseline & CoreFlags ) == CoreFlags );
        LUMEN_CHECK_TRUE( ( Baseline & OptionalFlags ) == RHI::ERHICapabilityFlags::None );

        Features.features.multiDrawIndirect       = VK_TRUE;
        Features.features.pipelineStatisticsQuery = VK_TRUE;
        Features.features.depthClamp              = VK_TRUE;
        Features.features.samplerAnisotropy       = VK_TRUE;
        Limits.maxSamplerAnisotropy               = 4.0F;
        Adapter.ComputeQueueFamilyIndex.Emplace( 1U );
        Adapter.bHasBindlessIndexing              = true;
        const RHI::ERHICapabilityFlags Advertised = BuildVulkanFeatureFlags( Adapter, Features, Limits, true, true, true );
        LUMEN_CHECK_TRUE( ( Advertised & OptionalFlags ) == OptionalFlags );
    }

    LUMEN_TEST( VulkanFeatureNegotiation, RejectsUnsupportedFeaturesAndShaderFormats )
    {
        FVulkanAdapter Adapter = MakeNegotiationAdapter();
        Adapter.Info.Capabilities.Features =
            RHI::ERHICapabilityFlags::DynamicRendering | RHI::ERHICapabilityFlags::TimelineSemaphores;

        RHI::FRHIDeviceDesc FeatureDesc;
        FeatureDesc.RequiredCapabilities.Features = RHI::ERHICapabilityFlags::RayTracing;
        const TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError> FeatureResult =
            NegotiateVulkanDeviceFeatures( Adapter, FeatureDesc );
        LUMEN_CHECK_HAS_ERROR( FeatureResult );
        LUMEN_CHECK_EQ( FeatureResult.Error().Code, RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( FeatureResult.Error().Condition, RHI::ERHIErrorCondition::Capability );

        RHI::FRHIDeviceDesc ShaderDesc;
        ShaderDesc.RequiredCapabilities.ShaderFormats = { RHI::ERHIShaderFormat::Source };
        const TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError> ShaderResult =
            NegotiateVulkanDeviceFeatures( Adapter, ShaderDesc );
        LUMEN_CHECK_HAS_ERROR( ShaderResult );
        LUMEN_CHECK_EQ( ShaderResult.Error().Code, RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( ShaderResult.Error().Condition, RHI::ERHIErrorCondition::Shader );
    }

    LUMEN_TEST( VulkanFeatureNegotiation, RejectsInvalidRequestsAndMissingNativeFamilies )
    {
        FVulkanAdapter Adapter = MakeNegotiationAdapter();
        RHI::FRHIDeviceDesc InvalidDesc;
        InvalidDesc.Queues = { { .Type = RHI::ERHIQueueType::Graphics, .Count = 0U } };
        const TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError> InvalidResult =
            NegotiateVulkanDeviceFeatures( Adapter, InvalidDesc );
        LUMEN_CHECK_HAS_ERROR( InvalidResult );
        LUMEN_CHECK_EQ( InvalidResult.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( InvalidResult.Error().Condition, RHI::ERHIErrorCondition::Queue );

        Adapter.ComputeQueueFamilyIndex.Reset();
        RHI::FRHIDeviceDesc ComputeDesc;
        ComputeDesc.Queues = { { .Type = RHI::ERHIQueueType::Compute, .Count = 1U } };
        const TExpected<FVulkanNegotiatedDeviceConfig, RHI::FRHIError> ComputeResult =
            NegotiateVulkanDeviceFeatures( Adapter, ComputeDesc );
        LUMEN_CHECK_HAS_ERROR( ComputeResult );
        LUMEN_CHECK_EQ( ComputeResult.Error().Code, RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( ComputeResult.Error().Condition, RHI::ERHIErrorCondition::Queue );
    }

} // namespace RHIVulkan

} // namespace Lumen
