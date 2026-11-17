/**
 * @file NullDevice.cpp
 * @brief Implements Null device creation, faults, and factory contracts.
 */

#include <Bindings/NullBindings.hpp>
#include <Caches/NullCaches.hpp>
#include <Commands/NullCommands.hpp>
#include <Device/NullDevice.hpp>
#include <Device/RHIBackendCast.hpp>
#include <Memory/NullMemory.hpp>
#include <Memory/RHIBufferPool.hpp>
#include <Memory/RHIDefragmenter.hpp>
#include <Memory/RHIReadbackAllocator.hpp>
#include <Memory/RHITransientResourceAllocator.hpp>
#include <Memory/RHIUploadAllocator.hpp>
#include <Pipelines/NullPipelines.hpp>
#include <Presentation/NullPresentation.hpp>
#include <Queries/NullQueries.hpp>
#include <Queue/NullQueue.hpp>
#include <Resources/NullResources.hpp>
#include <Shaders/NullShaders.hpp>
#include <Synchronization/NullSynchronization.hpp>
#include <Templates/Hash.hpp>
#include <Types/RHICopyFootprint.hpp>

/**
 * Public
 */

Lumen::FRHILimits Lumen::Tests::MakeDefaultNullLimits () noexcept
{
    return FRHILimits{
        .MaxBufferSize           = 1ULL << 30U,
        .MaxTextureDimension1D   = 16384U,
        .MaxTextureDimension2D   = 16384U,
        .MaxTextureDimension3D   = 2048U,
        .MaxTextureArrayLayers   = 2048U,
        .MaxColorAttachments     = 8U,
        .MaxBindingSets          = 8U,
        .MaxDescriptorsPerSet    = 4096U,
        .MaxInlineConstantBytes  = 256U,
        .MaxVertexAttributes     = 32U,
        .MaxVertexBuffers        = 16U,
        .MaxComputeWorkgroupSize = 1024U,
        .MinBufferAlignment      = 16ULL,
        .MinTextureRowAlignment  = 256ULL,
        .TimestampPeriod         = 1ULL,
    };
}

Lumen::Tests::FNullDeviceDesc::FNullDeviceDesc () noexcept
{
    Adapter.AdapterId             = 1ULL;
    Adapter.Name                  = "Null Adapter";
    Adapter.BackendName           = "Null";
    Adapter.Type                  = ERHIAdapterType::Software;
    Adapter.Limits                = MakeDefaultNullLimits();
    Adapter.Capabilities.Features = ERHICapabilityFlags::DynamicRendering | ERHICapabilityFlags::TimelineSemaphores |
                                    ERHICapabilityFlags::IndirectDraw | ERHICapabilityFlags::MultiDrawIndirect |
                                    ERHICapabilityFlags::TimestampQueries | ERHICapabilityFlags::OcclusionQueries |
                                    ERHICapabilityFlags::PipelineStatistics | ERHICapabilityFlags::AsyncCompute |
                                    ERHICapabilityFlags::Presentation | ERHICapabilityFlags::AnisotropicFiltering;
    Adapter.QueueFamilies = {
        { .Type = ERHIQueueType::Graphics, .Count = 2U, .bSupportsPresentation = true, .bSupportsTimestamps = true },
        { .Type = ERHIQueueType::Compute, .Count = 1U, .bSupportsTimestamps = true },
        { .Type = ERHIQueueType::Copy, .Count = 1U },
    };
    FaultPlan = MakeShared<FNullFaultPlan>();
}

Lumen::Tests::FNullDevice::FNullDevice ( FNullDeviceDesc InDesc ) noexcept
    : FRHIDeviceBase( InDesc.Adapter.Capabilities, InDesc.Adapter.Limits, FHash::TypeHash<FNullDevice>() ),
      Desc( std::move( InDesc ) ), DescriptorBackend( MakeUnique<FNullDescriptorPoolBackend>() )
{
    for ( const auto &Family : Desc.Adapter.QueueFamilies )
    {
        for ( UInt32 Index = 0U; Index < Family.Count; ++Index )
        {
            auto Queue = MakeShared<FNullQueue>( Family.Type, Index, Desc.FaultPlan, NextObjectId() );
            TrackObject( *Queue );
            Queues.push_back( std::move( Queue ) );
        }
    }
}

Lumen::Tests::FNullDevice::~FNullDevice () noexcept = default;

Lumen::TOptional<Lumen::FRHIError> Lumen::Tests::FNullDevice::CheckOperation ( const FStringView InOperation ) noexcept
{
    if ( TOptional<FRHIError> Error = GetUnavailableError( InOperation ) )
    {
        return Error;
    }
    return Desc.FaultPlan ? Desc.FaultPlan->ShouldFail( InOperation ) : TOptional<FRHIError>{};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullDevice::WaitIdle () noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "WaitIdle" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    for ( const auto &Queue : Queues )
    {
        const auto Result = Queue->WaitIdle();
        if ( not Result )
        {
            return Result;
        }
    }
    return {};
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIQueue>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::GetQueue ( const ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    if ( auto Error = CheckOperation( "GetQueue" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    UInt32 CurrentIndex = 0U;
    for ( const auto &Queue : Queues )
    {
        if ( Queue->GetType() == InType )
        {
            if ( CurrentIndex == InIndex )
            {
                return StaticCastSharedRef<IRHIQueue>( Queue );
            }
            ++CurrentIndex;
        }
    }
    return TUnexpected<FRHIError>( MakeRHIUnsupported( NotFound, ERHIErrorCondition::Queue, "Null queue was not requested" ) );
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBuffer>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateBuffer" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Size > GetLimits().MaxBufferSize )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Limit, "Buffer exceeds the published size limit" ) );
    }
    auto Result = MakeShared<FNullBuffer>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITexture>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateTexture ( const FRHITextureDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateTexture" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Extent.Width > GetLimits().MaxTextureDimension2D or InDesc.Extent.Height > GetLimits().MaxTextureDimension2D or
         InDesc.Extent.Depth > GetLimits().MaxTextureDimension3D or InDesc.LayerCount > GetLimits().MaxTextureArrayLayers )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Limit, "Texture exceeds a published dimension limit" ) );
    }
    const auto Footprint = ComputeRHICopyFootprint( InDesc.Format, InDesc.Extent, 0ULL, GetLimits().MinTextureRowAlignment );
    if ( not Footprint )
    {
        return TUnexpected<FRHIError>( Footprint.Error() );
    }
    FRHIResourceRequirements Requirements{
        .Size = Footprint->TotalSize, .Alignment = GetLimits().MinTextureRowAlignment, .PreferredMemoryClass = {} };
    Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );
    auto Result = MakeShared<FNullTexture>( InDesc, Requirements, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITextureView>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateTextureView" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullTextureView>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISampler>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateSampler" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullSampler>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIHeap>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateHeap" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullHeap>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBufferPool>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateBufferPool () noexcept
{
    if ( auto Error = CheckOperation( "CreateBufferPool" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    auto Result = MakeShared<FRHIBufferPool>( *this, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIUploadAllocator>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    if ( auto Error = CheckOperation( "CreateUploadAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InCapacity == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Upload capacity is zero" ) );
    }
    auto Result = MakeShared<FRHIUploadAllocator>( InCapacity, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIReadbackAllocator>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    if ( auto Error = CheckOperation( "CreateReadbackAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InCapacity == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback capacity is zero" ) );
    }
    auto Result = MakeShared<FRHIReadbackAllocator>( InCapacity, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHITransientResourceAllocator>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateTransientResourceAllocator () noexcept
{
    if ( auto Error = CheckOperation( "CreateTransientResourceAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    auto Result = MakeShared<FRHITransientResourceAllocator>( NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDefragmenter>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateDefragmenter () noexcept
{
    if ( auto Error = CheckOperation( "CreateDefragmenter" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    auto Result = MakeShared<FRHIDefragmenter>( NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingLayout>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateBindingLayout" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    for ( const FRHIBindingLayoutEntry &Entry : InDesc.Entries )
    {
        if ( Entry.bBindless and not GetCapabilities().Supports( ERHICapabilityFlags::BindlessResources ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Capability, "Bindless resources were not negotiated" ) );
        }
    }
    auto Result = MakeShared<FNullBindingLayout>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBindingSet>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept
{
    if ( auto Error = CheckOperation( "CreateBindingSet" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    auto Result = MakeShared<FNullBindingSet>( InLayout, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDescriptorAllocator>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateDescriptorAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullDescriptorAllocator>( InDesc, *DescriptorBackend, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIShader>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateShader ( const FRHIShaderDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateShader" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullShader>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipelineLayout>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreatePipelineLayout" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullPipelineLayout>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipeline>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateGraphicsPipeline" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    FRHIPipelineCompatibility Compatibility{ .ColorFormats       = InDesc.ColorFormats,
                                             .DepthStencilFormat = InDesc.DepthStencilFormat,
                                             .SampleCount        = InDesc.Multisample.Count,
                                             .VertexLayoutHash   = InDesc.VertexLayout.GetHash() };
    for ( const auto &Layout : InDesc.Layout->GetDesc().BindingLayouts )
    {
        Compatibility.LayoutHash = RHIHashCombine( Compatibility.LayoutHash, Layout->GetDesc().GetHash() );
    }
    auto Result = MakeShared<FNullPipeline>( ERHIPipelineType::Graphics, std::move( Compatibility ), NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipeline>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateComputePipeline" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    FRHIPipelineCompatibility Compatibility;
    for ( const auto &Layout : InDesc.Layout->GetDesc().BindingLayouts )
    {
        Compatibility.LayoutHash = RHIHashCombine( Compatibility.LayoutHash, Layout->GetDesc().GetHash() );
    }
    auto Result = MakeShared<FNullPipeline>( ERHIPipelineType::Compute, Compatibility, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIPipelineCache>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreatePipelineCache" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullPipelineCache>( InDesc, Desc.FaultPlan, NextObjectId() );
    if ( const auto Initialized = Result->Initialize(); not Initialized )
    {
        return TUnexpected<FRHIError>( Initialized.Error() );
    }
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHICommandAllocator>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateCommandAllocator ( const ERHIQueueType InQueueType ) noexcept
{
    if ( auto Error = CheckOperation( "CreateCommandAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InQueueType >= ERHIQueueType::Count )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid command queue type" ) );
    }
    auto Result = MakeShared<FNullCommandAllocator>( InQueueType, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHICommandList>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept
{
    if ( auto Error = CheckOperation( "CreateCommandList" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    auto Allocator = TryRHIBackendCast<FNullCommandAllocator>( StaticCastSharedRef<IRHIObject>( InAllocator ) );
    if ( not Allocator )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Allocator belongs to another backend" ) );
    }
    auto Result = MakeShared<FNullCommandList>( *Allocator, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISemaphore>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateSemaphore" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Type == ERHISemaphoreType::Timeline and
         not GetCapabilities().Supports( ERHICapabilityFlags::TimelineSemaphores ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Capability, "Timeline semaphores were not negotiated" ) );
    }
    auto Result = MakeShared<FNullSemaphore>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISwapchain>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateSwapchain" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( not GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Presentation, "Presentation was not negotiated" ) );
    }
    const auto SurfaceCapabilities = MakeNullSwapchainCapabilities();
    if ( const auto Valid = InDesc.Validate( SurfaceCapabilities ); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    auto Result = MakeShared<FNullSwapchain>( InDesc, SurfaceCapabilities, *this, Desc.FaultPlan, NextObjectId() );
    if ( const auto Initialized = Result->Initialize(); not Initialized )
    {
        return TUnexpected<FRHIError>( Initialized.Error() );
    }
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIQueryPool>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept
{
    if ( auto Error = CheckOperation( "CreateQueryPool" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    ERHICapabilityFlags Required = ERHICapabilityFlags::None;
    if ( InDesc.Type == ERHIQueryType::Occlusion )
    {
        Required = ERHICapabilityFlags::OcclusionQueries;
    }
    else if ( InDesc.Type == ERHIQueryType::Timestamp )
    {
        Required = ERHICapabilityFlags::TimestampQueries;
    }
    else if ( InDesc.Type == ERHIQueryType::PipelineStatistics )
    {
        Required = ERHICapabilityFlags::PipelineStatistics;
    }
    if ( not GetCapabilities().Supports( Required ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( Unsupported, ERHIErrorCondition::Query, "Query type was not negotiated" ) );
    }
    auto Result = MakeShared<FNullQueryPool>( InDesc, Desc.FaultPlan, NextObjectId() );
    TrackObject( *Result );
    return Result;
}

void Lumen::Tests::FNullDevice::MarkLost () noexcept
{
    SetStatus( ERHIDeviceStatus::Lost );
    for ( const auto &Queue : Queues )
    {
        Queue->MarkLost();
    }
}

Lumen::TSharedPtr<Lumen::Tests::FNullFaultPlan> Lumen::Tests::FNullDevice::GetFaultPlan () const noexcept
{
    return Desc.FaultPlan;
}
