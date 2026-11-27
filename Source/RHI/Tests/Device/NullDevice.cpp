/**
 * @file NullDevice.cpp
 * @brief Implements Null device creation, faults, and factory contracts.
 */

#include <Bindings/NullBindings.hpp>
#include <Caches/NullCaches.hpp>
#include <Commands/NullCommands.hpp>
#include <Device/NullDevice.hpp>
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
#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHICopyFootprint.hpp>
#include <Resources/NullResources.hpp>
#include <Shaders/NullShaders.hpp>
#include <Synchronization/NullSynchronization.hpp>
#include <Templates/Hash.hpp>

namespace
{

Lumen::RHI::FRHIExpected ValidateNullPlacement ( const Lumen::RHI::FRHIResourceRequirements &InRequirements,
                                                 const Lumen::RHI::ERHIMemoryClass InMemoryClass,
                                                 const Lumen::TSharedRef<Lumen::RHI::IRHIHeap> &InHeap,
                                                 const Lumen::RHI::FRHIAllocation &InAllocation,
                                                 const Lumen::RHI::FRHIObjectId InOwnerId ) noexcept
{
    using namespace Lumen;
    using namespace Lumen::RHI;

    if ( not InAllocation.IsValid() or InAllocation.Heap != InHeap.Get() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Placed resource allocation does not identify the supplied heap" ) );
    }
    const TOptional<Tests::FNullHeap *> NullHeap = TryRHIBackendCast<Tests::FNullHeap>( static_cast<IRHIObject &>( *InHeap ) );
    if ( not NullHeap or ( *NullHeap )->GetOwnerId() != InOwnerId )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Placed resource heap belongs to another device" ) );
    }
    const FRHIHeapDesc &HeapDesc = ( *NullHeap )->GetDesc();
    if ( InAllocation.MemoryClass != InMemoryClass or HeapDesc.MemoryClass != InMemoryClass )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Placed resource memory classes do not match" ) );
    }
    if ( InRequirements.Alignment == 0ULL or InAllocation.Alignment < InRequirements.Alignment or
         InAllocation.Offset % InRequirements.Alignment != 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Placed resource alignment is insufficient" ) );
    }
    if ( InAllocation.Size < InRequirements.Size or InAllocation.Offset > HeapDesc.Size or
         InAllocation.Size > HeapDesc.Size - InAllocation.Offset )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Placed resource range exceeds its heap or allocation" ) );
    }
    return {};
}

} // namespace

/**
 * Public
 */

Lumen::RHI::FRHILimits Lumen::RHI::Tests::MakeDefaultNullLimits () noexcept
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

Lumen::RHI::Tests::FNullDeviceDesc::FNullDeviceDesc () noexcept
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
    Adapter.Capabilities.ShaderFormats = { ERHIShaderFormat::Null };
    Adapter.QueueFamilies              = {
        { .Type = ERHIQueueType::Graphics, .Count = 2U, .bSupportsPresentation = true, .bSupportsTimestamps = true },
        { .Type = ERHIQueueType::Compute, .Count = 1U, .bSupportsTimestamps = true },
        { .Type = ERHIQueueType::Copy, .Count = 1U },
    };
    FaultPlan = MakeShared<FNullFaultPlan>();
}

Lumen::RHI::Tests::FNullDevice::FNullDevice ( FNullDeviceDesc InDesc ) noexcept
    : FRHIDeviceBase( InDesc.Adapter.Capabilities, InDesc.Adapter.Limits, FHash::TypeHash<FNullDevice>() ),
      Desc( std::move( InDesc ) ), DescriptorBackend( MakeUnique<FNullDescriptorPoolBackend>() )
{
    for ( const auto &Family : Desc.Adapter.QueueFamilies )
    {
        for ( UInt32 Index = 0U; Index < Family.Count; ++Index )
        {
            TSharedRef<FNullQueue> Queue = MakeShared<FNullQueue>( Family.Type, Index, Desc.FaultPlan, NextObjectId() );
            TrackObject( *Queue );
            Queues.push_back( std::move( Queue ) );
        }
    }
}

Lumen::RHI::Tests::FNullDevice::~FNullDevice () noexcept = default;

Lumen::TOptional<Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullDevice::CheckOperation ( const FStringView InOperation ) noexcept
{
    if ( TOptional<FRHIError> Error = GetUnavailableError( InOperation ) )
    {
        return Error;
    }
    return Desc.FaultPlan ? Desc.FaultPlan->ShouldFail( InOperation ) : TOptional<FRHIError>{};
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullDevice::WaitIdle () noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "WaitIdle" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    for ( const auto &Queue : Queues )
    {
        const FRHIExpected Result = Queue->WaitIdle();
        if ( not Result )
        {
            return Result;
        }
    }
    return {};
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueue>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::GetQueue ( const ERHIQueueType InType, const UInt32 InIndex ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "GetQueue" ) )
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
    return TUnexpected<FRHIError>( MakeRHINotFound( ERHIErrorCondition::Queue, "Null queue was not requested" ) );
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateBuffer ( const FRHIBufferDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateBuffer" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Size > GetLimits().MaxBufferSize )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Limit, "Buffer exceeds the published size limit" ) );
    }
    TSharedRef<FNullBuffer> Result = MakeShared<FNullBuffer>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreatePlacedBuffer ( const FRHIBufferDesc &InDesc,
                                                     TSharedRef<IRHIHeap> InHeap,
                                                     const FRHIAllocation &InAllocation ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreatePlacedBuffer" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Size > GetLimits().MaxBufferSize )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Limit, "Buffer exceeds the published size limit" ) );
    }
    const FRHIResourceRequirements Requirements{
        .Size = InDesc.Size, .Alignment = 16ULL, .PreferredMemoryClass = TOptional<ERHIMemoryClass>( InDesc.MemoryClass ) };
    const FRHIExpected Placement = ValidateNullPlacement( Requirements, InDesc.MemoryClass, InHeap, InAllocation, GetId() );
    if ( not Placement )
    {
        return TUnexpected<FRHIError>( Placement.Error() );
    }
    TSharedRef<FNullBuffer> Result = MakeShared<FNullBuffer>( InDesc, NextObjectId(), InHeap, InAllocation );
    TrackObject( *Result );
    return Result;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateTexture ( const FRHITextureDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateTexture" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Extent.Width > GetLimits().MaxTextureDimension2D or InDesc.Extent.Height > GetLimits().MaxTextureDimension2D or
         InDesc.Extent.Depth > GetLimits().MaxTextureDimension3D or InDesc.LayerCount > GetLimits().MaxTextureArrayLayers )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Limit, "Texture exceeds a published dimension limit" ) );
    }
    const TExpected<FRHICopyFootprint, FRHIError> Footprint =
        ComputeRHICopyFootprint( InDesc.Format, InDesc.Extent, 0ULL, GetLimits().MinTextureRowAlignment );
    if ( not Footprint )
    {
        return TUnexpected<FRHIError>( Footprint.Error() );
    }
    FRHIResourceRequirements Requirements{
        .Size = Footprint->TotalSize, .Alignment = GetLimits().MinTextureRowAlignment, .PreferredMemoryClass = {} };
    Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );
    TSharedRef<FNullTexture> Result = MakeShared<FNullTexture>( InDesc, Requirements, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreatePlacedTexture ( const FRHITextureDesc &InDesc,
                                                      TSharedRef<IRHIHeap> InHeap,
                                                      const FRHIAllocation &InAllocation ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreatePlacedTexture" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Extent.Width > GetLimits().MaxTextureDimension2D or InDesc.Extent.Height > GetLimits().MaxTextureDimension2D or
         InDesc.Extent.Depth > GetLimits().MaxTextureDimension3D or InDesc.LayerCount > GetLimits().MaxTextureArrayLayers )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Limit, "Texture exceeds a published dimension limit" ) );
    }
    const TExpected<FRHICopyFootprint, FRHIError> Footprint =
        ComputeRHICopyFootprint( InDesc.Format, InDesc.Extent, 0ULL, GetLimits().MinTextureRowAlignment );
    if ( not Footprint )
    {
        return TUnexpected<FRHIError>( Footprint.Error() );
    }
    FRHIResourceRequirements Requirements{
        .Size = Footprint->TotalSize, .Alignment = GetLimits().MinTextureRowAlignment, .PreferredMemoryClass = {} };
    Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );
    const FRHIExpected Placement = ValidateNullPlacement( Requirements, InDesc.MemoryClass, InHeap, InAllocation, GetId() );
    if ( not Placement )
    {
        return TUnexpected<FRHIError>( Placement.Error() );
    }
    TSharedRef<FNullTexture> Result = MakeShared<FNullTexture>( InDesc, Requirements, NextObjectId(), InHeap, InAllocation );
    TrackObject( *Result );
    return Result;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateTextureView ( const FRHITextureViewDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateTextureView" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullTextureView> Result = MakeShared<FNullTextureView>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISampler>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateSampler ( const FRHISamplerDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateSampler" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullSampler> Result = MakeShared<FNullSampler>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIHeap>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateHeap ( const FRHIHeapDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateHeap" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullHeap> Result = MakeShared<FNullHeap>( InDesc, NextObjectId(), GetId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBufferPool>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateBufferPool () noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateBufferPool" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    TSharedRef<FRHIBufferPool> Result = MakeShared<FRHIBufferPool>( *this, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateUploadAllocator ( const UInt64 InCapacity ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateUploadAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InCapacity == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Upload capacity is zero" ) );
    }
    const FRHIBufferDesc BufferDesc{
        .Size = InCapacity, .Usage = ERHIResourceUsage::CopySource, .MemoryClass = ERHIMemoryClass::Upload };
    TSharedRef<FNullBuffer> Buffer = MakeShared<FNullBuffer>( BufferDesc, NextObjectId() );
    TrackObject( *Buffer );
    TSharedRef<FRHIUploadAllocator> Result =
        MakeShared<FRHIUploadAllocator>( StaticCastSharedRef<IRHIBuffer>( Buffer ), NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateReadbackAllocator ( const UInt64 InCapacity ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateReadbackAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InCapacity == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback capacity is zero" ) );
    }
    const FRHIBufferDesc BufferDesc{
        .Size = InCapacity, .Usage = ERHIResourceUsage::CopyDestination, .MemoryClass = ERHIMemoryClass::Readback };
    TSharedRef<FNullBuffer> Buffer = MakeShared<FNullBuffer>( BufferDesc, NextObjectId() );
    TrackObject( *Buffer );
    TSharedRef<FRHIReadbackAllocator> Result =
        MakeShared<FRHIReadbackAllocator>( StaticCastSharedRef<IRHIBuffer>( Buffer ), NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateTransientResourceAllocator () noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateTransientResourceAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    TSharedRef<FRHITransientResourceAllocator> Result = MakeShared<FRHITransientResourceAllocator>( NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDefragmenter>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateDefragmenter () noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateDefragmenter" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    TSharedRef<FRHIDefragmenter> Result = MakeShared<FRHIDefragmenter>( NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateBindingLayout ( const FRHIBindingLayoutDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateBindingLayout" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    for ( const FRHIBindingLayoutEntry &Entry : InDesc.Entries )
    {
        if ( Entry.bBindless and not GetCapabilities().Supports( ERHICapabilityFlags::BindlessResources ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIUnsupported( ERHIErrorCondition::Capability, "Bindless resources were not negotiated" ) );
        }
    }
    TSharedRef<FNullBindingLayout> Result = MakeShared<FNullBindingLayout>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBindingSet>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateBindingSet ( const TSharedRef<IRHIBindingLayout> &InLayout ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateBindingSet" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    TSharedRef<FNullBindingSet> Result = MakeShared<FNullBindingSet>( InLayout, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDescriptorAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateDescriptorAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullDescriptorAllocator> Result =
        MakeShared<FNullDescriptorAllocator>( InDesc, *DescriptorBackend, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateShader ( const FRHIShaderDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateShader" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( not GetCapabilities().Supports( InDesc.Format ) )
    {
        return TUnexpected<FRHIError>( MakeRHIUnsupported( ERHIErrorCondition::Shader, "Shader format was not negotiated" ) );
    }
    TSharedRef<FNullShader> Result = MakeShared<FNullShader>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreatePipelineLayout ( const FRHIPipelineLayoutDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreatePipelineLayout" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullPipelineLayout> Result = MakeShared<FNullPipelineLayout>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateGraphicsPipeline ( const FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateGraphicsPipeline" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
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
    TSharedRef<FNullPipeline> Result =
        MakeShared<FNullPipeline>( ERHIPipelineType::Graphics, std::move( Compatibility ), NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateComputePipeline ( const FRHIComputePipelineDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateComputePipeline" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    FRHIPipelineCompatibility Compatibility;
    for ( const auto &Layout : InDesc.Layout->GetDesc().BindingLayouts )
    {
        Compatibility.LayoutHash = RHIHashCombine( Compatibility.LayoutHash, Layout->GetDesc().GetHash() );
    }
    TSharedRef<FNullPipeline> Result = MakeShared<FNullPipeline>( ERHIPipelineType::Compute, Compatibility, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreatePipelineCache ( const FRHIPipelineCacheDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreatePipelineCache" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullPipelineCache> Result = MakeShared<FNullPipelineCache>( InDesc, Desc.FaultPlan, NextObjectId() );
    if ( const FRHIExpected Initialized = Result->Initialize(); not Initialized )
    {
        return TUnexpected<FRHIError>( Initialized.Error() );
    }
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandAllocator>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateCommandAllocator ( const ERHIQueueType InQueueType ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateCommandAllocator" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( InQueueType >= ERHIQueueType::Count )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid command queue type" ) );
    }
    TSharedRef<FNullCommandAllocator> Result = MakeShared<FNullCommandAllocator>( InQueueType, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHICommandList>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateCommandList ( const TSharedRef<IRHICommandAllocator> &InAllocator ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateCommandList" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    TOptional<TSharedPtr<FNullCommandAllocator>> Allocator =
        TryRHIBackendCast<FNullCommandAllocator>( StaticCastSharedRef<IRHIObject>( InAllocator ) );
    if ( not Allocator )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Allocator belongs to another backend" ) );
    }
    TSharedRef<FNullCommandList> Result = MakeShared<FNullCommandList>( *Allocator, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISemaphore>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateSemaphore ( const FRHISemaphoreDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateSemaphore" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Type == ERHISemaphoreType::Timeline and
         not GetCapabilities().Supports( ERHICapabilityFlags::TimelineSemaphores ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Capability, "Timeline semaphores were not negotiated" ) );
    }
    TSharedRef<FNullSemaphore> Result = MakeShared<FNullSemaphore>( InDesc, NextObjectId() );
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateSwapchain ( const FRHISwapchainDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateSwapchain" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( not GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Presentation, "Presentation was not negotiated" ) );
    }
    const FRHISwapchainCapabilities SurfaceCapabilities = MakeNullSwapchainCapabilities();
    if ( const FRHIExpected Valid = InDesc.Validate( SurfaceCapabilities ); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    TSharedRef<FNullSwapchain> Result =
        MakeShared<FNullSwapchain>( InDesc, SurfaceCapabilities, *this, Desc.FaultPlan, NextObjectId() );
    if ( const FRHIExpected Initialized = Result->Initialize(); not Initialized )
    {
        return TUnexpected<FRHIError>( Initialized.Error() );
    }
    TrackObject( *Result );
    return Result;
}
Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIQueryPool>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullDevice::CreateQueryPool ( const FRHIQueryPoolDesc &InDesc ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( "CreateQueryPool" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    if ( const FRHIExpected Valid = InDesc.Validate(); not Valid )
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
        return TUnexpected<FRHIError>( MakeRHIUnsupported( ERHIErrorCondition::Query, "Query type was not negotiated" ) );
    }
    TSharedRef<FNullQueryPool> Result = MakeShared<FNullQueryPool>( InDesc, Desc.FaultPlan, NextObjectId() );
    TrackObject( *Result );
    return Result;
}

void Lumen::RHI::Tests::FNullDevice::MarkLost () noexcept
{
    SetStatus( ERHIDeviceStatus::Lost );
    for ( const auto &Queue : Queues )
    {
        Queue->MarkLost();
    }
}

Lumen::TSharedPtr<Lumen::RHI::Tests::FNullFaultPlan> Lumen::RHI::Tests::FNullDevice::GetFaultPlan () const noexcept
{
    return Desc.FaultPlan;
}
