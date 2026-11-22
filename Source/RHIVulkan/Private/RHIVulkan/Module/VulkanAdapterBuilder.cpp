/**
 * @file VulkanAdapterBuilder.cpp
 * @brief Implements Vulkan physical-device evaluation and metadata translation.
 */

#include <RHIVulkan/Module/VulkanAdapterBuilder.hpp>
#include <RHIVulkan/Module/VulkanExtensions.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>

#include <RHI/Types/RHIHash.hpp>

#include <cmath>
#include <cstring>
#include <utility>

/**
 * Private
 */

namespace
{

struct FVulkanAdapterIdentityKey
{
    Lumen::UInt8 DeviceUUID[VK_UUID_SIZE] = {};
    Lumen::UInt32 VendorID                = 0U;
    Lumen::UInt32 DeviceID                = 0U;
    Lumen::UInt32 DeviceType              = 0U;
    Lumen::UInt32 Reserved                = 0U;
};

struct FVulkanQueueFamilies
{
    Lumen::TVector<VkQueueFamilyProperties> Properties;
    Lumen::TOptional<Lumen::UInt32> GraphicsIndex;
    Lumen::TOptional<Lumen::UInt32> DedicatedComputeIndex;
    Lumen::TOptional<Lumen::UInt32> DedicatedCopyIndex;
};

template <typename ValueType> [[nodiscard]] constexpr ValueType MinValue ( ValueType InLeft, ValueType InRight ) noexcept
{
    return ( InLeft < InRight ) ? InLeft : InRight;
}

template <typename ValueType> [[nodiscard]] constexpr ValueType MaxValue ( ValueType InLeft, ValueType InRight ) noexcept
{
    return ( InLeft > InRight ) ? InLeft : InRight;
}

[[nodiscard]] Lumen::RHI::ERHIAdapterType MapAdapterType ( VkPhysicalDeviceType InType ) noexcept
{
    switch ( InType )
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return Lumen::RHI::ERHIAdapterType::Discrete;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return Lumen::RHI::ERHIAdapterType::Integrated;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return Lumen::RHI::ERHIAdapterType::Software;
    default:
        return Lumen::RHI::ERHIAdapterType::Unknown;
    }
}

[[nodiscard]] Lumen::TOptional<FVulkanQueueFamilies> DiscoverQueueFamilies ( VkPhysicalDevice InPhysicalDevice ) noexcept
{
    Lumen::UInt32 QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties( InPhysicalDevice, &QueueFamilyCount, nullptr );
    if ( QueueFamilyCount == 0U )
    {
        return {};
    }

    FVulkanQueueFamilies QueueFamilies;
    QueueFamilies.Properties.resize( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( InPhysicalDevice, &QueueFamilyCount, QueueFamilies.Properties.data() );

    for ( Lumen::UInt32 Index = 0U; Index < QueueFamilyCount; ++Index )
    {
        const VkQueueFamilyProperties &Family = QueueFamilies.Properties[Index];
        if ( Family.queueCount == 0U )
        {
            continue;
        }

        const VkQueueFlags Flags = Family.queueFlags;
        if ( not QueueFamilies.GraphicsIndex and ( Flags & VK_QUEUE_GRAPHICS_BIT ) != 0U )
        {
            QueueFamilies.GraphicsIndex.Emplace( Index );
        }
        else if ( not QueueFamilies.DedicatedComputeIndex and ( Flags & VK_QUEUE_COMPUTE_BIT ) != 0U and
                  ( Flags & VK_QUEUE_GRAPHICS_BIT ) == 0U )
        {
            QueueFamilies.DedicatedComputeIndex.Emplace( Index );
        }
        else if ( not QueueFamilies.DedicatedCopyIndex and ( Flags & VK_QUEUE_TRANSFER_BIT ) != 0U and
                  ( Flags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) == 0U )
        {
            QueueFamilies.DedicatedCopyIndex.Emplace( Index );
        }
    }

    if ( not QueueFamilies.GraphicsIndex )
    {
        return {};
    }
    return Lumen::TOptional<FVulkanQueueFamilies>( std::move( QueueFamilies ) );
}

[[nodiscard]] Lumen::Bool HasBindlessIndexing ( const VkPhysicalDeviceVulkan12Features &InFeatures ) noexcept
{
    return ( InFeatures.descriptorIndexing == VK_TRUE ) and
           ( InFeatures.shaderSampledImageArrayNonUniformIndexing == VK_TRUE ) and
           ( InFeatures.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE ) and
           ( InFeatures.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE ) and
           ( InFeatures.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE ) and
           ( InFeatures.descriptorBindingPartiallyBound == VK_TRUE ) and
           ( InFeatures.descriptorBindingVariableDescriptorCount == VK_TRUE ) and
           ( InFeatures.runtimeDescriptorArray == VK_TRUE );
}

[[nodiscard]] Lumen::UInt64 MakeAdapterId ( const VkPhysicalDeviceProperties &InProperties,
                                            const VkPhysicalDeviceIDProperties &InIdProperties ) noexcept
{
    FVulkanAdapterIdentityKey IdentityKey{};
    std::memcpy( IdentityKey.DeviceUUID, InIdProperties.deviceUUID, VK_UUID_SIZE );
    IdentityKey.VendorID   = InProperties.vendorID;
    IdentityKey.DeviceID   = InProperties.deviceID;
    IdentityKey.DeviceType = static_cast<Lumen::UInt32>( InProperties.deviceType );

    const Lumen::UInt64 AdapterId = Lumen::RHI::RHIHashValue( IdentityKey );
    return AdapterId == 0ULL ? 1ULL : AdapterId;
}

void PopulateCapabilities ( Lumen::RHIVulkan::FVulkanAdapter &InAdapter,
                            const VkPhysicalDeviceFeatures2 &InFeatures,
                            const VkPhysicalDeviceLimits &InLimits,
                            const Lumen::Bool bExactTimestampPeriod,
                            const Lumen::Bool bGraphicsSupportsTimestamps,
                            const Lumen::Bool bSupportsPresentation ) noexcept
{
    Lumen::RHI::ERHICapabilityFlags FeatureFlags =
        Lumen::RHI::ERHICapabilityFlags::DynamicRendering | Lumen::RHI::ERHICapabilityFlags::TimelineSemaphores |
        Lumen::RHI::ERHICapabilityFlags::IndirectDraw | Lumen::RHI::ERHICapabilityFlags::OcclusionQueries;

    if ( InAdapter.bHasBindlessIndexing )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::BindlessResources;
    }
    if ( InFeatures.features.multiDrawIndirect == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::MultiDrawIndirect;
    }
    if ( bExactTimestampPeriod and bGraphicsSupportsTimestamps )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::TimestampQueries;
    }
    if ( InFeatures.features.pipelineStatisticsQuery == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::PipelineStatistics;
    }
    if ( InFeatures.features.depthClamp == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::DepthClamp;
    }
    if ( InFeatures.features.samplerAnisotropy == VK_TRUE and InLimits.maxSamplerAnisotropy > 1.0F )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::AnisotropicFiltering;
    }
    if ( InAdapter.ComputeQueueFamilyIndex )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::AsyncCompute;
    }
    if ( bSupportsPresentation )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::Presentation;
    }

    InAdapter.Info.Capabilities.Features         = FeatureFlags;
    InAdapter.Info.Capabilities.ShaderModelMajor = 0U;
    InAdapter.Info.Capabilities.ShaderModelMinor = 0U;
}

void PopulateLimits ( Lumen::RHIVulkan::FVulkanAdapter &InAdapter,
                      const VkPhysicalDeviceLimits &InLimits,
                      const VkPhysicalDeviceVulkan13Properties &InProperties13,
                      const Lumen::UInt64 InTimestampPeriod ) noexcept
{
    const Lumen::UInt32 SafePerSetDescriptors =
        MinValue( MinValue( InLimits.maxDescriptorSetUniformBuffers, InLimits.maxDescriptorSetStorageBuffers ),
                  MinValue( MinValue( InLimits.maxDescriptorSetSampledImages, InLimits.maxDescriptorSetStorageImages ),
                            InLimits.maxDescriptorSetSamplers ) );

    const Lumen::UInt64 MinBufferAlign = MaxValue( MaxValue<Lumen::UInt64>( 1ULL, InLimits.minUniformBufferOffsetAlignment ),
                                                   static_cast<Lumen::UInt64>( InLimits.minStorageBufferOffsetAlignment ) );

    InAdapter.Info.Limits.MaxBufferSize           = InProperties13.maxBufferSize;
    InAdapter.Info.Limits.MaxTextureDimension1D   = InLimits.maxImageDimension1D;
    InAdapter.Info.Limits.MaxTextureDimension2D   = InLimits.maxImageDimension2D;
    InAdapter.Info.Limits.MaxTextureDimension3D   = InLimits.maxImageDimension3D;
    InAdapter.Info.Limits.MaxTextureArrayLayers   = InLimits.maxImageArrayLayers;
    InAdapter.Info.Limits.MaxColorAttachments     = InLimits.maxColorAttachments;
    InAdapter.Info.Limits.MaxBindingSets          = InLimits.maxBoundDescriptorSets;
    InAdapter.Info.Limits.MaxDescriptorsPerSet    = MaxValue( 1U, SafePerSetDescriptors );
    InAdapter.Info.Limits.MaxInlineConstantBytes  = InLimits.maxPushConstantsSize;
    InAdapter.Info.Limits.MaxVertexAttributes     = InLimits.maxVertexInputAttributes;
    InAdapter.Info.Limits.MaxVertexBuffers        = InLimits.maxVertexInputBindings;
    InAdapter.Info.Limits.MaxComputeWorkgroupSize = InLimits.maxComputeWorkGroupInvocations;
    InAdapter.Info.Limits.MinBufferAlignment      = MinBufferAlign;
    InAdapter.Info.Limits.MinTextureRowAlignment  = MaxValue<Lumen::UInt64>( 1ULL, InLimits.optimalBufferCopyRowPitchAlignment );
    InAdapter.Info.Limits.TimestampPeriod         = InTimestampPeriod;
}

void PopulateQueueFamilies ( Lumen::RHIVulkan::FVulkanAdapter &InAdapter,
                             const FVulkanQueueFamilies &InQueueFamilies,
                             const Lumen::Bool bExactTimestampPeriod,
                             const Lumen::Bool bSupportsPresentation ) noexcept
{
    Lumen::RHI::FRHIQueueFamilyInfo GraphicsInfo{};
    GraphicsInfo.Type                  = Lumen::RHI::ERHIQueueType::Graphics;
    GraphicsInfo.Count                 = 1U;
    GraphicsInfo.bSupportsPresentation = bSupportsPresentation;
    GraphicsInfo.bSupportsTimestamps =
        bExactTimestampPeriod and InQueueFamilies.Properties[*InAdapter.GraphicsQueueFamilyIndex].timestampValidBits > 0U;
    InAdapter.Info.QueueFamilies.push_back( GraphicsInfo );

    if ( InAdapter.ComputeQueueFamilyIndex )
    {
        Lumen::RHI::FRHIQueueFamilyInfo ComputeInfo{};
        ComputeInfo.Type                  = Lumen::RHI::ERHIQueueType::Compute;
        ComputeInfo.Count                 = 1U;
        ComputeInfo.bSupportsPresentation = false;
        ComputeInfo.bSupportsTimestamps =
            bExactTimestampPeriod and InQueueFamilies.Properties[*InAdapter.ComputeQueueFamilyIndex].timestampValidBits > 0U;
        InAdapter.Info.QueueFamilies.push_back( ComputeInfo );
    }

    if ( InAdapter.CopyQueueFamilyIndex )
    {
        Lumen::RHI::FRHIQueueFamilyInfo CopyInfo{};
        CopyInfo.Type                  = Lumen::RHI::ERHIQueueType::Copy;
        CopyInfo.Count                 = 1U;
        CopyInfo.bSupportsPresentation = false;
        CopyInfo.bSupportsTimestamps =
            bExactTimestampPeriod and InQueueFamilies.Properties[*InAdapter.CopyQueueFamilyIndex].timestampValidBits > 0U;
        InAdapter.Info.QueueFamilies.push_back( CopyInfo );
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TOptional<Lumen::RHIVulkan::FVulkanAdapter>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::BuildVulkanAdapter ( const FVulkanInstance &InInstance, VkPhysicalDevice InPhysicalDevice ) noexcept
{
    VkPhysicalDeviceProperties BaseProperties{};
    vkGetPhysicalDeviceProperties( InPhysicalDevice, &BaseProperties );
    if ( BaseProperties.apiVersion < VK_API_VERSION_1_3 )
    {
        return TOptional<FVulkanAdapter>{};
    }

    VkPhysicalDeviceIDProperties IdProperties{};
    IdProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

    VkPhysicalDeviceVulkan13Properties Properties13{};
    Properties13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    Properties13.pNext = &IdProperties;

    VkPhysicalDeviceProperties2 Properties2{};
    Properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    Properties2.pNext = &Properties13;
    vkGetPhysicalDeviceProperties2( InPhysicalDevice, &Properties2 );

    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceVulkan12Features Features12{};
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Features12.pNext = &Features13;

    VkPhysicalDeviceFeatures2 Features2{};
    Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    Features2.pNext = &Features12;
    vkGetPhysicalDeviceFeatures2( InPhysicalDevice, &Features2 );

    if ( Features13.dynamicRendering != VK_TRUE or Features13.synchronization2 != VK_TRUE or
         Features12.timelineSemaphore != VK_TRUE )
    {
        return TOptional<FVulkanAdapter>{};
    }

    LUMEN_EXPECT( Extensions, FVulkanExtensions::Enumerate( InPhysicalDevice ) );

    TOptional<FVulkanQueueFamilies> QueueFamilies = DiscoverQueueFamilies( InPhysicalDevice );
    if ( not QueueFamilies )
    {
        return TOptional<FVulkanAdapter>{};
    }

    FVulkanAdapter Adapter{};
    Adapter.PhysicalDevice = InPhysicalDevice;
    Adapter.GraphicsQueueFamilyIndex.Emplace( *QueueFamilies->GraphicsIndex );
    if ( QueueFamilies->DedicatedComputeIndex )
    {
        Adapter.ComputeQueueFamilyIndex.Emplace( *QueueFamilies->DedicatedComputeIndex );
    }
    if ( QueueFamilies->DedicatedCopyIndex )
    {
        Adapter.CopyQueueFamilyIndex.Emplace( *QueueFamilies->DedicatedCopyIndex );
    }

    Adapter.bHasPortabilitySubset = Extensions.HasExtension( "VK_KHR_portability_subset" );
    Adapter.bHasSwapchain         = Extensions.HasExtension( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    Adapter.bHasBindlessIndexing  = HasBindlessIndexing( Features12 );

    Adapter.Info.AdapterId   = MakeAdapterId( Properties2.properties, IdProperties );
    Adapter.Info.Name        = Properties2.properties.deviceName;
    Adapter.Info.BackendName = "Vulkan";
    Adapter.Info.Type        = MapAdapterType( Properties2.properties.deviceType );

    const VkPhysicalDeviceLimits &Limits = Properties2.properties.limits;
    const Lumen::Bool bExactTimestampPeriod =
        ( Limits.timestampPeriod >= 1.0F ) and std::isfinite( Limits.timestampPeriod ) and
        ( Limits.timestampPeriod <= static_cast<float>( UINT64_MAX ) ) and
        ( static_cast<float>( static_cast<UInt64>( Limits.timestampPeriod ) ) == Limits.timestampPeriod );
    const UInt64 TimestampPeriod = bExactTimestampPeriod ? static_cast<UInt64>( Limits.timestampPeriod ) : 1ULL;
    const Lumen::Bool bGraphicsSupportsTimestamps =
        QueueFamilies->Properties[*Adapter.GraphicsQueueFamilyIndex].timestampValidBits > 0U;
    const Lumen::Bool bSupportsPresentation = Adapter.bHasSwapchain and InInstance.SupportsSurface();

    PopulateCapabilities( Adapter, Features2, Limits, bExactTimestampPeriod, bGraphicsSupportsTimestamps, bSupportsPresentation );
    PopulateLimits( Adapter, Limits, Properties13, TimestampPeriod );
    PopulateQueueFamilies( Adapter, *QueueFamilies, bExactTimestampPeriod, bSupportsPresentation );

    LUMEN_EXPECT( Validation, Adapter.Info.Validate() );
    return TOptional<FVulkanAdapter>( std::move( Adapter ) );
}
