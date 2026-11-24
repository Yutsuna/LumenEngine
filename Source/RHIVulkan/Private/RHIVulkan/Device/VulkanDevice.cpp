/**
 * @file VulkanDevice.cpp
 * @brief Implements Vulkan logical device creation, queue lookup, idle, and V2 factory stubs.
 */

#include <RHIVulkan/Device/VulkanDevice.hpp>

#include <RHI/Bindings/IRHIBindingLayout.hpp>
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Bindings/IRHIDescriptorAllocator.hpp>
#include <RHI/Caches/IRHIPipelineCache.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Memory/IRHIBufferPool.hpp>
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Shaders/IRHIShader.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIVulkan/Device/VulkanFeatureNegotiation.hpp>
#include <RHIVulkan/Module/VulkanInstance.hpp>
#include <RHIVulkan/Queue/VulkanQueue.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

/**
 * Public
 */

Lumen::RHIVulkan::FVulkanDevice::FVulkanDevice ( TSharedRef<FVulkanDeviceContext> InContext ) noexcept
    : RHI::FRHIDeviceBase( InContext->GetCapabilities(), InContext->GetLimits(), GetVulkanBackendTypeTag<FVulkanDevice>() ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIVulkan::FVulkanDevice::~FVulkanDevice () noexcept
{
    if ( GetStatus() != RHI::ERHIDeviceStatus::Lost )
    {
        ( void )vkDeviceWaitIdle( Context->GetDevice() );
    }
    SetStatus( RHI::ERHIDeviceStatus::Closed );
    Context->MarkClosed();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError> Lumen::RHIVulkan::FVulkanDevice::Create (
    TSharedRef<FVulkanInstance> InInstance, const FVulkanAdapter &InAdapter, const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Config, NegotiateVulkanDeviceFeatures( InAdapter, InDesc ) );
    Config.Features12.pNext = &Config.Features13;
    Config.Features2.pNext  = &Config.Features12;

    constexpr Float32 QueuePriority = 1.0F;
    TVector<VkDeviceQueueCreateInfo> QueueCreateInfos;
    QueueCreateInfos.reserve( Config.UniqueQueueFamilyIndices.size() );
    for ( const UInt32 FamilyIndex : Config.UniqueQueueFamilyIndices )
    {
        VkDeviceQueueCreateInfo QueueInfo{};
        QueueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueInfo.queueFamilyIndex = FamilyIndex;
        QueueInfo.queueCount       = 1U;
        QueueInfo.pQueuePriorities = &QueuePriority;
        QueueCreateInfos.push_back( QueueInfo );
    }

    VkDeviceCreateInfo DeviceCreateInfo{};
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pNext                   = &Config.Features2;
    DeviceCreateInfo.queueCreateInfoCount    = static_cast<UInt32>( QueueCreateInfos.size() );
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.data();
    DeviceCreateInfo.enabledExtensionCount   = static_cast<UInt32>( Config.EnabledExtensions.size() );
    DeviceCreateInfo.ppEnabledExtensionNames = Config.EnabledExtensions.empty() ? nullptr : Config.EnabledExtensions.data();

    FVulkanDeviceHandle NativeDevice;
    LUMEN_VK_TRY( vkCreateDevice( InAdapter.PhysicalDevice, &DeviceCreateInfo, nullptr, NativeDevice.ResetAndGetAddressOf() ),
                  RHI::ERHIErrorCondition::Device );

    TSharedRef<FVulkanDeviceContext> Context = MakeShared<FVulkanDeviceContext>(
        static_cast<TSharedRef<FVulkanInstance> &&>( InInstance ), InAdapter.PhysicalDevice,
        static_cast<FVulkanDeviceHandle &&>( NativeDevice ), Config.NegotiatedCapabilities, Config.NegotiatedLimits );

    TSharedRef<FVulkanDevice> Device = MakeShared<FVulkanDevice>( Context );

    const auto RegisterQueue = [&] ( RHI::ERHIQueueType InType, TOptional<UInt32> InFamilyIndex ) noexcept
    {
        if ( not InFamilyIndex.HasValue() )
        {
            return;
        }
        VkQueue NativeQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue( Context->GetDevice(), *InFamilyIndex, 0U, &NativeQueue );
        Context->SetQueue( InType, *InFamilyIndex, NativeQueue );

        TSharedRef<FVulkanQueue> Queue =
            MakeShared<FVulkanQueue>( Device->NextObjectId(), Context, InType, *InFamilyIndex, NativeQueue );
        Device->TrackObject( *Queue );
        Device->Queues.push_back( FQueueEntry{ .Type = InType, .Index = 0U, .Queue = Queue } );
    };

    RegisterQueue( RHI::ERHIQueueType::Graphics, Config.GraphicsQueueFamilyIndex );
    RegisterQueue( RHI::ERHIQueueType::Compute, Config.ComputeQueueFamilyIndex );
    RegisterQueue( RHI::ERHIQueueType::Copy, Config.CopyQueueFamilyIndex );

    return Device;
}

void Lumen::RHIVulkan::FVulkanDevice::InjectDeviceLost () noexcept
{
    SetStatus( RHI::ERHIDeviceStatus::Lost );
    Context->MarkLost();
}

const Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanDeviceContext> &Lumen::RHIVulkan::FVulkanDevice::GetContext () const noexcept
{
    return Context;
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanDevice::WaitIdle () noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "WaitIdle" ) );

    const VkResult Result = vkDeviceWaitIdle( Context->GetDevice() );
    if ( Result == VK_ERROR_DEVICE_LOST )
    {
        InjectDeviceLost();
    }

    LUMEN_VK_TRY( Result, RHI::ERHIErrorCondition::Device );
    return {};
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueue>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::GetQueue ( RHI::ERHIQueueType InType, UInt32 InIndex ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( "GetQueue" ) );

    if ( static_cast<USize>( InType ) >= static_cast<USize>( RHI::ERHIQueueType::Count ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidQueue( "Invalid ERHIQueueType value." ) );
    }

    for ( const FQueueEntry &Entry : Queues )
    {
        if ( Entry.Type == InType and Entry.Index == InIndex )
        {
            return Entry.Queue;
        }
    }

    return TUnexpected(
        RHI::MakeRHINotFound( RHI::ERHIErrorCondition::Queue, "Requested Vulkan queue was not created on this device." ) );
}

#define LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( OperationName, Condition )                                                           \
    do                                                                                                                           \
    {                                                                                                                            \
        LUMEN_TRY_OPTIONAL_ERROR( GetUnavailableError( OperationName ) );                                                        \
        return TUnexpected( RHI::MakeRHIUnsupported( Condition, OperationName " is not implemented in RHIVulkan V2 yet." ) );    \
    } while ( false )

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateBuffer ( const RHI::FRHIBufferDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateBuffer", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreatePlacedBuffer ( const RHI::FRHIBufferDesc &,
                                                      TSharedRef<RHI::IRHIHeap>,
                                                      const RHI::FRHIAllocation & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreatePlacedBuffer", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateTexture ( const RHI::FRHITextureDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateTexture", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreatePlacedTexture ( const RHI::FRHITextureDesc &,
                                                       TSharedRef<RHI::IRHIHeap>,
                                                       const RHI::FRHIAllocation & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreatePlacedTexture", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateTextureView ( const RHI::FRHITextureViewDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateTextureView", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISampler>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateSampler ( const RHI::FRHISamplerDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateSampler", RHI::ERHIErrorCondition::Resource );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIHeap>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateHeap ( const RHI::FRHIHeapDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateHeap", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBufferPool>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateBufferPool () noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateBufferPool", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateUploadAllocator ( UInt64 ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateUploadAllocator", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateReadbackAllocator ( UInt64 ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateReadbackAllocator", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateTransientResourceAllocator () noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateTransientResourceAllocator", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDefragmenter>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateDefragmenter () noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateDefragmenter", RHI::ERHIErrorCondition::Memory );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateBindingLayout ( const RHI::FRHIBindingLayoutDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateBindingLayout", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateBindingSet ( const TSharedRef<RHI::IRHIBindingLayout> & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateBindingSet", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateDescriptorAllocator ( const RHI::FRHIDescriptorAllocatorDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateDescriptorAllocator", RHI::ERHIErrorCondition::Binding );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateShader ( const RHI::FRHIShaderDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateShader", RHI::ERHIErrorCondition::Shader );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreatePipelineLayout ( const RHI::FRHIPipelineLayoutDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreatePipelineLayout", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateGraphicsPipeline ( const RHI::FRHIGraphicsPipelineDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateGraphicsPipeline", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateComputePipeline ( const RHI::FRHIComputePipelineDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateComputePipeline", RHI::ERHIErrorCondition::Pipeline );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreatePipelineCache ( const RHI::FRHIPipelineCacheDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreatePipelineCache", RHI::ERHIErrorCondition::Cache );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateCommandAllocator ( RHI::ERHIQueueType ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateCommandAllocator", RHI::ERHIErrorCondition::Command );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandList>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateCommandList ( const TSharedRef<RHI::IRHICommandAllocator> & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateCommandList", RHI::ERHIErrorCondition::Command );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISemaphore>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateSemaphore ( const RHI::FRHISemaphoreDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateSemaphore", RHI::ERHIErrorCondition::Synchronization );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateSwapchain ( const RHI::FRHISwapchainDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateSwapchain", RHI::ERHIErrorCondition::Presentation );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDevice::CreateQueryPool ( const RHI::FRHIQueryPoolDesc & ) noexcept
{
    LUMEN_VULKAN_UNIMPLEMENTED_FACTORY( "CreateQueryPool", RHI::ERHIErrorCondition::Query );
}

#undef LUMEN_VULKAN_UNIMPLEMENTED_FACTORY
