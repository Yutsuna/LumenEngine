/**
 * @file VulkanAdapter.cpp
 * @brief Implements physical-device evaluation, limit translation, and adapter selection.
 */

#include <RHIVulkan/Module/VulkanAdapter.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <RHI/Types/RHIHash.hpp>

#include <Algo/Any.hpp>
#include <Algo/Find.hpp>

#include <cmath>
#include <cstring>

/**
 * Private
 */

namespace
{

constexpr Lumen::UInt32 MaxEnumerationRetries = 4U;

struct FVulkanAdapterIdentityKey
{
    Lumen::UInt8 DeviceUUID[VK_UUID_SIZE] = {};
    Lumen::UInt32 VendorID                = 0U;
    Lumen::UInt32 DeviceID                = 0U;
    Lumen::UInt32 DeviceType              = 0U;
    Lumen::UInt32 Reserved                = 0U;
};

template <typename ValueType> [[nodiscard]] constexpr ValueType MinValue ( ValueType InLeft, ValueType InRight ) noexcept
{
    return ( InLeft < InRight ) ? InLeft : InRight;
}

template <typename ValueType> [[nodiscard]] constexpr ValueType MaxValue ( ValueType InLeft, ValueType InRight ) noexcept
{
    return ( InLeft > InRight ) ? InLeft : InRight;
}

[[nodiscard]] Lumen::TExpected<Lumen::TVector<VkPhysicalDevice>, Lumen::RHI::FRHIError>
EnumeratePhysicalDevices ( VkInstance InInstance ) noexcept
{
    for ( Lumen::UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        Lumen::UInt32 Count  = 0U;
        VkResult CountResult = vkEnumeratePhysicalDevices( InInstance, &Count, nullptr );
        if ( CountResult != VK_SUCCESS )
        {
            return Lumen::TUnexpected( Lumen::RHIVulkan::MapVulkanResult( CountResult, Lumen::RHI::ERHIErrorCondition::Device,
                                                                          "vkEnumeratePhysicalDevices (count)" ) );
        }

        Lumen::TVector<VkPhysicalDevice> Devices;
        Devices.resize( Count );
        if ( Count == 0U )
        {
            return Devices;
        }

        const VkResult ListResult = vkEnumeratePhysicalDevices( InInstance, &Count, Devices.data() );
        if ( ListResult == VK_SUCCESS )
        {
            Devices.resize( Count );
            return Devices;
        }
        if ( ListResult != VK_INCOMPLETE )
        {
            return Lumen::TUnexpected( Lumen::RHIVulkan::MapVulkanResult( ListResult, Lumen::RHI::ERHIErrorCondition::Device,
                                                                          "vkEnumeratePhysicalDevices (list)" ) );
        }
    }

    return Lumen::TUnexpected( Lumen::RHI::FRHIError( Lumen::RHI::ERHIErrorCode::CorruptedData,
                                                      Lumen::RHI::ERHIErrorCondition::Device,
                                                      "vkEnumeratePhysicalDevices remained VK_INCOMPLETE." ) );
}

[[nodiscard]] Lumen::TExpected<Lumen::TVector<VkExtensionProperties>, Lumen::RHI::FRHIError>
EnumerateDeviceExtensions ( VkPhysicalDevice InDevice ) noexcept
{
    for ( Lumen::UInt32 Attempt = 0U; Attempt < MaxEnumerationRetries; ++Attempt )
    {
        Lumen::UInt32 Count  = 0U;
        VkResult CountResult = vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &Count, nullptr );
        if ( CountResult != VK_SUCCESS )
        {
            return Lumen::TUnexpected( Lumen::RHIVulkan::MapVulkanResult( CountResult, Lumen::RHI::ERHIErrorCondition::Device,
                                                                          "vkEnumerateDeviceExtensionProperties (count)" ) );
        }

        Lumen::TVector<VkExtensionProperties> Extensions;
        Extensions.resize( Count );
        if ( Count == 0U )
        {
            return Extensions;
        }

        const VkResult ListResult = vkEnumerateDeviceExtensionProperties( InDevice, nullptr, &Count, Extensions.data() );
        if ( ListResult == VK_SUCCESS )
        {
            Extensions.resize( Count );
            return Extensions;
        }
        if ( ListResult != VK_INCOMPLETE )
        {
            return Lumen::TUnexpected( Lumen::RHIVulkan::MapVulkanResult( ListResult, Lumen::RHI::ERHIErrorCondition::Device,
                                                                          "vkEnumerateDeviceExtensionProperties (list)" ) );
        }
    }

    return Lumen::TUnexpected( Lumen::RHI::FRHIError( Lumen::RHI::ERHIErrorCode::CorruptedData,
                                                      Lumen::RHI::ERHIErrorCondition::Device,
                                                      "vkEnumerateDeviceExtensionProperties remained VK_INCOMPLETE." ) );
}

[[nodiscard]] Lumen::Bool HasDeviceExtension ( const Lumen::TVector<VkExtensionProperties> &InExtensions,
                                               const Lumen::AnsiChar *InName ) noexcept
{
    return Lumen::Algo::AnyOf( InExtensions, [InName] ( const VkExtensionProperties &Ext ) noexcept -> Lumen::Bool
    {
        return std::strcmp( Ext.extensionName, InName ) == 0;
    } );
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

[[nodiscard]] Lumen::UInt32 GetAdapterPreferenceRank ( Lumen::RHI::ERHIAdapterType InType ) noexcept
{
    switch ( InType )
    {
    case Lumen::RHI::ERHIAdapterType::Discrete:
        return 0U;
    case Lumen::RHI::ERHIAdapterType::Integrated:
        return 1U;
    case Lumen::RHI::ERHIAdapterType::Unknown:
        return 2U;
    case Lumen::RHI::ERHIAdapterType::Software:
        return 3U;
    default:
        return 4U;
    }
}

[[nodiscard]] Lumen::TOptional<Lumen::RHIVulkan::FVulkanAdapter>
TryBuildBaselineAdapter ( const Lumen::RHIVulkan::FVulkanInstance &InInstance, VkPhysicalDevice InPhysicalDevice ) noexcept
{
    VkPhysicalDeviceProperties BaseProps{};
    vkGetPhysicalDeviceProperties( InPhysicalDevice, &BaseProps );
    if ( BaseProps.apiVersion < VK_API_VERSION_1_3 )
    {
        return {};
    }

    VkPhysicalDeviceIDProperties IdProps{};
    IdProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

    VkPhysicalDeviceVulkan13Properties Props13{};
    Props13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    Props13.pNext = &IdProps;

    VkPhysicalDeviceProperties2 Props2{};
    Props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    Props2.pNext = &Props13;
    vkGetPhysicalDeviceProperties2( InPhysicalDevice, &Props2 );

    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceVulkan12Features Features12{};
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Features12.pNext = &Features13;

    VkPhysicalDeviceFeatures2 Features2{};
    Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    Features2.pNext = &Features12;
    vkGetPhysicalDeviceFeatures2( InPhysicalDevice, &Features2 );

    // Mandatory Vulkan 1.3 baseline (D2).
    if ( Features13.dynamicRendering != VK_TRUE or Features13.synchronization2 != VK_TRUE or
         Features12.timelineSemaphore != VK_TRUE )
    {
        return {};
    }

    const auto ExtensionsResult = EnumerateDeviceExtensions( InPhysicalDevice );
    if ( not ExtensionsResult )
    {
        return {};
    }

    Lumen::UInt32 QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties( InPhysicalDevice, &QueueFamilyCount, nullptr );
    if ( QueueFamilyCount == 0U )
    {
        return {};
    }

    Lumen::TVector<VkQueueFamilyProperties> QueueFamilies;
    QueueFamilies.resize( QueueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( InPhysicalDevice, &QueueFamilyCount, QueueFamilies.data() );

    Lumen::TOptional<Lumen::UInt32> GraphicsIndex;
    Lumen::TOptional<Lumen::UInt32> DedicatedComputeIndex;
    Lumen::TOptional<Lumen::UInt32> DedicatedCopyIndex;

    for ( Lumen::UInt32 Index = 0U; Index < QueueFamilyCount; ++Index )
    {
        const VkQueueFamilyProperties &Family = QueueFamilies[Index];
        if ( Family.queueCount == 0U )
        {
            continue;
        }

        const VkQueueFlags Flags = Family.queueFlags;
        if ( not GraphicsIndex and ( Flags & VK_QUEUE_GRAPHICS_BIT ) != 0U )
        {
            GraphicsIndex = Index;
        }
        else if ( not DedicatedComputeIndex and ( Flags & VK_QUEUE_COMPUTE_BIT ) != 0U and
                  ( Flags & VK_QUEUE_GRAPHICS_BIT ) == 0U )
        {
            DedicatedComputeIndex = Index;
        }
        else if ( not DedicatedCopyIndex and ( Flags & VK_QUEUE_TRANSFER_BIT ) != 0U and
                  ( Flags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ) == 0U )
        {
            DedicatedCopyIndex = Index;
        }
    }

    if ( not GraphicsIndex )
    {
        return {};
    }

    Lumen::RHIVulkan::FVulkanAdapter Adapter{};
    Adapter.PhysicalDevice           = InPhysicalDevice;
    Adapter.GraphicsQueueFamilyIndex = *GraphicsIndex;
    Adapter.ComputeQueueFamilyIndex  = DedicatedComputeIndex;
    Adapter.CopyQueueFamilyIndex     = DedicatedCopyIndex;
    Adapter.bHasPortabilitySubset    = HasDeviceExtension( *ExtensionsResult, "VK_KHR_portability_subset" );
    Adapter.bHasSwapchain            = HasDeviceExtension( *ExtensionsResult, VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    Adapter.bHasBindlessIndexing =
        ( Features12.descriptorIndexing == VK_TRUE ) and ( Features12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE ) and
        ( Features12.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE ) and
        ( Features12.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE ) and
        ( Features12.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE ) and
        ( Features12.descriptorBindingPartiallyBound == VK_TRUE ) and
        ( Features12.descriptorBindingVariableDescriptorCount == VK_TRUE ) and ( Features12.runtimeDescriptorArray == VK_TRUE );

    FVulkanAdapterIdentityKey IdentityKey{};
    std::memcpy( IdentityKey.DeviceUUID, IdProps.deviceUUID, VK_UUID_SIZE );
    IdentityKey.VendorID   = Props2.properties.vendorID;
    IdentityKey.DeviceID   = Props2.properties.deviceID;
    IdentityKey.DeviceType = static_cast<Lumen::UInt32>( Props2.properties.deviceType );

    Lumen::UInt64 AdapterId = Lumen::RHI::RHIHashValue( IdentityKey );
    if ( AdapterId == 0ULL )
    {
        AdapterId = 1ULL;
    }

    Adapter.Info.AdapterId   = AdapterId;
    Adapter.Info.Name        = Props2.properties.deviceName;
    Adapter.Info.BackendName = "Vulkan";
    Adapter.Info.Type        = MapAdapterType( Props2.properties.deviceType );

    const VkPhysicalDeviceLimits &VkLimits = Props2.properties.limits;

    // Exact integer nanosecond representability check per §13 Open Question 2.
    const Lumen::Bool bExactTimestampPeriod =
        ( VkLimits.timestampPeriod >= 1.0F ) and std::isfinite( VkLimits.timestampPeriod ) and
        ( VkLimits.timestampPeriod <= static_cast<float>( UINT64_MAX ) ) and
        ( static_cast<float>( static_cast<Lumen::UInt64>( VkLimits.timestampPeriod ) ) == VkLimits.timestampPeriod );

    const Lumen::UInt64 TimestampPeriod = bExactTimestampPeriod ? static_cast<Lumen::UInt64>( VkLimits.timestampPeriod ) : 1ULL;

    const Lumen::Bool bGraphicsSupportsTimestamps = QueueFamilies[Adapter.GraphicsQueueFamilyIndex].timestampValidBits > 0U;

    Lumen::RHI::ERHICapabilityFlags FeatureFlags =
        Lumen::RHI::ERHICapabilityFlags::DynamicRendering | Lumen::RHI::ERHICapabilityFlags::TimelineSemaphores |
        Lumen::RHI::ERHICapabilityFlags::IndirectDraw | Lumen::RHI::ERHICapabilityFlags::OcclusionQueries;

    if ( Adapter.bHasBindlessIndexing )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::BindlessResources;
    }
    if ( Features2.features.multiDrawIndirect == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::MultiDrawIndirect;
    }
    if ( bExactTimestampPeriod and bGraphicsSupportsTimestamps )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::TimestampQueries;
    }
    if ( Features2.features.pipelineStatisticsQuery == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::PipelineStatistics;
    }
    if ( Features2.features.depthClamp == VK_TRUE )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::DepthClamp;
    }
    if ( Features2.features.samplerAnisotropy == VK_TRUE and VkLimits.maxSamplerAnisotropy > 1.0F )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::AnisotropicFiltering;
    }
    if ( Adapter.ComputeQueueFamilyIndex )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::AsyncCompute;
    }
    if ( Adapter.bHasSwapchain and InInstance.SupportsSurface() )
    {
        FeatureFlags |= Lumen::RHI::ERHICapabilityFlags::Presentation;
    }

    Adapter.Info.Capabilities.Features = FeatureFlags;
    // Left at 0.0 until M0-R2 documents ShaderModelMajor/Minor mapping (§13 Open Question 1).
    Adapter.Info.Capabilities.ShaderModelMajor = 0U;
    Adapter.Info.Capabilities.ShaderModelMinor = 0U;

    const Lumen::UInt32 SafePerSetDescriptors =
        MinValue( MinValue( VkLimits.maxDescriptorSetUniformBuffers, VkLimits.maxDescriptorSetStorageBuffers ),
                  MinValue( MinValue( VkLimits.maxDescriptorSetSampledImages, VkLimits.maxDescriptorSetStorageImages ),
                            VkLimits.maxDescriptorSetSamplers ) );

    const Lumen::UInt64 MinBufferAlign = MaxValue( MaxValue<Lumen::UInt64>( 1ULL, VkLimits.minUniformBufferOffsetAlignment ),
                                                   static_cast<Lumen::UInt64>( VkLimits.minStorageBufferOffsetAlignment ) );

    Adapter.Info.Limits.MaxBufferSize           = Props13.maxBufferSize;
    Adapter.Info.Limits.MaxTextureDimension1D   = VkLimits.maxImageDimension1D;
    Adapter.Info.Limits.MaxTextureDimension2D   = VkLimits.maxImageDimension2D;
    Adapter.Info.Limits.MaxTextureDimension3D   = VkLimits.maxImageDimension3D;
    Adapter.Info.Limits.MaxTextureArrayLayers   = VkLimits.maxImageArrayLayers;
    Adapter.Info.Limits.MaxColorAttachments     = VkLimits.maxColorAttachments;
    Adapter.Info.Limits.MaxBindingSets          = VkLimits.maxBoundDescriptorSets;
    Adapter.Info.Limits.MaxDescriptorsPerSet    = MaxValue( 1U, SafePerSetDescriptors );
    Adapter.Info.Limits.MaxInlineConstantBytes  = VkLimits.maxPushConstantsSize;
    Adapter.Info.Limits.MaxVertexAttributes     = VkLimits.maxVertexInputAttributes;
    Adapter.Info.Limits.MaxVertexBuffers        = VkLimits.maxVertexInputBindings;
    Adapter.Info.Limits.MaxComputeWorkgroupSize = VkLimits.maxComputeWorkGroupInvocations;
    Adapter.Info.Limits.MinBufferAlignment      = MinBufferAlign;
    Adapter.Info.Limits.MinTextureRowAlignment  = MaxValue<Lumen::UInt64>( 1ULL, VkLimits.optimalBufferCopyRowPitchAlignment );
    Adapter.Info.Limits.TimestampPeriod         = TimestampPeriod;

    Lumen::RHI::FRHIQueueFamilyInfo GraphicsInfo{};
    GraphicsInfo.Type                  = Lumen::RHI::ERHIQueueType::Graphics;
    GraphicsInfo.Count                 = 1U;
    GraphicsInfo.bSupportsPresentation = Adapter.bHasSwapchain and InInstance.SupportsSurface();
    GraphicsInfo.bSupportsTimestamps   = bExactTimestampPeriod and bGraphicsSupportsTimestamps;
    Adapter.Info.QueueFamilies.push_back( GraphicsInfo );

    if ( Adapter.ComputeQueueFamilyIndex )
    {
        Lumen::RHI::FRHIQueueFamilyInfo ComputeInfo{};
        ComputeInfo.Type                  = Lumen::RHI::ERHIQueueType::Compute;
        ComputeInfo.Count                 = 1U;
        ComputeInfo.bSupportsPresentation = false;
        ComputeInfo.bSupportsTimestamps =
            bExactTimestampPeriod and QueueFamilies[*Adapter.ComputeQueueFamilyIndex].timestampValidBits > 0U;
        Adapter.Info.QueueFamilies.push_back( ComputeInfo );
    }

    if ( Adapter.CopyQueueFamilyIndex )
    {
        Lumen::RHI::FRHIQueueFamilyInfo CopyInfo{};
        CopyInfo.Type                  = Lumen::RHI::ERHIQueueType::Copy;
        CopyInfo.Count                 = 1U;
        CopyInfo.bSupportsPresentation = false;
        CopyInfo.bSupportsTimestamps =
            bExactTimestampPeriod and QueueFamilies[*Adapter.CopyQueueFamilyIndex].timestampValidBits > 0U;
        Adapter.Info.QueueFamilies.push_back( CopyInfo );
    }

    if ( not Adapter.Info.Validate() )
    {
        return {};
    }

    return Lumen::TOptional<Lumen::RHIVulkan::FVulkanAdapter>( Adapter );
}
} // namespace

Lumen::TExpected<Lumen::TVector<Lumen::RHIVulkan::FVulkanAdapter>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::EnumerateVulkanAdapters ( const FVulkanInstance &InInstance ) noexcept
{
    LUMEN_EXPECT( Devices, EnumeratePhysicalDevices( InInstance.GetHandle() ) );

    TVector<FVulkanAdapter> Result;
    for ( VkPhysicalDevice PhysicalDevice : Devices )
    {
        if ( TOptional<FVulkanAdapter> Built = TryBuildBaselineAdapter( InInstance, PhysicalDevice ) )
        {
            Result.push_back( *Built );
        }
    }

    return Result;
}

Lumen::TExpected<Lumen::RHIVulkan::FVulkanAdapter, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::SelectVulkanAdapter ( const FVulkanInstance &InInstance, TOptional<UInt64> InAdapterId ) noexcept
{
    LUMEN_EXPECT( Adapters, EnumerateVulkanAdapters( InInstance ) );

    if ( Adapters.empty() )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Device,
                                                     "No Vulkan physical device satisfies the Vulkan 1.3 baseline." ) );
    }

    if ( InAdapterId )
    {
        if ( const FVulkanAdapter *Result = Lumen::Algo::FindIf( Adapters, [&] ( const FVulkanAdapter &Adapter ) noexcept -> Bool
        {
            return Adapter.Info.AdapterId == *InAdapterId;
        } ) )
        {
            return *Result;
        }
        return TUnexpected(
            RHI::MakeRHINotFound( RHI::ERHIErrorCondition::Device, "Requested Vulkan AdapterId was not found." ) );
    }

    const FVulkanAdapter *Best = &Adapters.front();
    for ( USize Index = 1U; Index < Adapters.size(); ++Index )
    {
        const FVulkanAdapter &Candidate = Adapters.at( Index );
        if ( GetAdapterPreferenceRank( Candidate.Info.Type ) < GetAdapterPreferenceRank( Best->Info.Type ) )
        {
            Best = &Candidate;
        }
    }

    return *Best;
}
