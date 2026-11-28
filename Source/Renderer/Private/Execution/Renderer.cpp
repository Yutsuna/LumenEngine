/**
 * @file Renderer.cpp
 * @brief Backend-neutral renderer facade implementation.
 */

#include <Execution/Renderer.hpp>

#include <Logging/Logger.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/RenderSnapshotView.hpp>

#include <Algo/ForEach.hpp>
#include <Core/EnumFlags.hpp>

#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHICopyDescs.hpp>
#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Commands/RHITransition.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Device/RHIDeviceStatus.hpp>
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIBlendState.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Pipelines/RHIVertexLayout.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Queue/RHISubmitDesc.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <RHI/Types/RHIShaderFormat.hpp>
#include <RHI/Types/RHIShaderStage.hpp>

#include <RenderGraph/RenderGraphContext.hpp>

#include <array>
#include <cstring>
#include <utility>

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogRenderer, "Renderer" );

/** @brief Converts an executor enqueue failure into the facade's error set. */
[[nodiscard]] Lumen::ERendererError ToRendererError ( const Lumen::ERenderExecutorError InError ) noexcept
{
    switch ( InError )
    {
    case Lumen::ERenderExecutorError::QueueFull:
        return Lumen::ERendererError::QueueFull;
    case Lumen::ERenderExecutorError::NotRunning:
    case Lumen::ERenderExecutorError::AlreadyRunning:
        return Lumen::ERendererError::NotRunning;
    default:
        return Lumen::ERendererError::ExecutorFailed;
    }
}

/** @brief Records the compiled barriers at their declared point relative to the single forward pass. */
[[nodiscard]] Lumen::Bool RecordGraphTransitions ( const Lumen::FRenderGraphContext &InContext,
                                                   Lumen::TSpan<const Lumen::ERenderTransitionRecordPoint> InRecordPoints,
                                                   const Lumen::ERenderTransitionRecordPoint InPoint,
                                                   Lumen::RHI::IRHICommandList &InList ) noexcept
{
    using namespace Lumen;
    for ( USize Index = 0U; Index < InRecordPoints.size(); ++Index )
    {
        if ( InRecordPoints[Index] != InPoint )
        {
            continue;
        }
        const auto Resolved = InContext.ResolveTransition( Index );
        if ( not Resolved )
        {
            return false;
        }
        const RHI::FRHITransition &Values = *Resolved;
        if ( not InList.Transition( TSpan<const RHI::FRHITransition>( &Values, 1U ) ) )
        {
            return false;
        }
    }
    return true;
}

} // namespace

Lumen::FRenderer::FRenderer () noexcept : Proxies( 256ULL ), Assets( 64ULL )
{
}

Lumen::FRenderer::~FRenderer () noexcept
{
    RequestShutdown();
    Join();
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::Start ( const FRendererDesc &InDesc ) noexcept
{
    if ( Executor.IsRunning() )
    {
        return MakeUnexpected( ERendererError::AlreadyRunning );
    }
    if ( not InDesc.Device.IsValid() or InDesc.SnapshotStream == nullptr or not InDesc.Extent.IsValid() or
         InDesc.CommandCapacity == 0ULL or InDesc.ProxyCapacity == 0ULL or InDesc.AssetCapacity == 0ULL )
    {
        return MakeUnexpected( ERendererError::InvalidDesc );
    }
    if ( not InDesc.Device->GetCapabilities().Supports( RHI::ERHICapabilityFlags::DynamicRendering ) )
    {
        return MakeUnexpected( ERendererError::CapabilityUnsupported );
    }

    Device         = InDesc.Device;
    SnapshotStream = InDesc.SnapshotStream;
    Extent         = InDesc.Extent;
    Swapchain      = InDesc.Swapchain;
    Shaders        = InDesc.Shaders;
    ProxyCapacity  = InDesc.ProxyCapacity;
    AssetCapacity  = InDesc.AssetCapacity;
    ReadbackDesc   = InDesc.Readback;
    FrameExtension = InDesc.FrameExtension;

    Proxies = FRenderProxyRegistry( ProxyCapacity );
    Assets  = FRenderAssetRegistry( AssetCapacity );
    PendingGeometry.reserve( AssetCapacity );

    LUMEN_EXPECT( _, CreateResources() );

    const TExpected<void, ERenderExecutorError> Started =
        Executor.Start( FRenderExecutorDesc{ .CommandCapacity = InDesc.CommandCapacity }, *this );
    if ( not Started ) [[unlikely]]
    {
        return MakeUnexpected( ToRendererError( Started.Error() ) );
    }

    return {};
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::RequestRender ( const UInt64 InFrameIndex ) noexcept
{
    if ( not Executor.IsRunning() or SnapshotStream == nullptr )
    {
        return MakeUnexpected( ERendererError::NotRunning );
    }

    const FRenderFrameRequest Request{ .FrameIndex       = InFrameIndex,
                                       .SnapshotSequence = SnapshotStream->GetLastPublishedSequence() };
    const TExpected<void, ERenderExecutorError> Result = Executor.Enqueue( FRenderCommand::MakeRender( Request ) );
    if ( not Result ) [[unlikely]]
    {
        return MakeUnexpected( ToRendererError( Result.Error() ) );
    }
    return {};
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::RequestResize ( const RHI::FRHIExtent3D &InExtent ) noexcept
{
    if ( not Executor.IsRunning() or not InExtent.IsValid() )
    {
        return MakeUnexpected( ERendererError::NotRunning );
    }

    const TExpected<void, ERenderExecutorError> Result = Executor.Enqueue( FRenderCommand::MakeResize( InExtent ) );
    if ( not Result ) [[unlikely]]
    {
        return MakeUnexpected( ToRendererError( Result.Error() ) );
    }
    return {};
}

void Lumen::FRenderer::RequestShutdown () noexcept
{
    if ( Executor.IsRunning() )
    {
        /** INFO: A saturated queue can reject the marker; RequestStop still drains what was accepted and the executor
         *        counts and publishes the rejection when it joins. */
        static_cast<void>( Executor.Enqueue( FRenderCommand::MakeSimple( ERenderCommandType::Shutdown ) ) );
    }
    Executor.RequestStop();
}

void Lumen::FRenderer::Join () noexcept
{
    Executor.Join();
}

Lumen::Bool Lumen::FRenderer::IsRunning () const noexcept
{
    return Executor.IsRunning();
}

Lumen::TExpected<void, Lumen::ERendererError>
Lumen::FRenderer::RegisterGeometry ( const FRenderGeometryAssetDesc &InDesc ) noexcept
{
    if ( Executor.IsRunning() )
    {
        return MakeUnexpected( ERendererError::InvalidDesc );
    }
    if ( not InDesc.Id.IsValid() or InDesc.Positions.size() < 3U or InDesc.Indices.size() < 3U or
         PendingGeometry.size() >= AssetCapacity )
    {
        return MakeUnexpected( ERendererError::InvalidDesc );
    }

    FPendingGeometry Geometry;
    Geometry.Id = InDesc.Id;
    Geometry.Positions.assign( InDesc.Positions.begin(), InDesc.Positions.end() );
    Geometry.Indices.assign( InDesc.Indices.begin(), InDesc.Indices.end() );
    PendingGeometry.push_back( std::move( Geometry ) );
    return {};
}

Lumen::TOptional<Lumen::TSpan<const Lumen::Byte>> Lumen::FRenderer::GetReadbackData () const noexcept
{
    return ReadbackData;
}

void Lumen::FRenderer::SwapStatusBuffers () noexcept
{
    Executor.SwapStatusBuffers();
}

const Lumen::FRenderFrameStatus &Lumen::FRenderer::GetStatus () const noexcept
{
    return Executor.GetStatus();
}

void Lumen::FRenderer::HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept
{
    switch ( InCommand.Type )
    {
    case ERenderCommandType::RenderLatest:
        RenderLatest( InCommand.Frame, InOutStatus );
        break;
    case ERenderCommandType::Resize:
        ApplyResize( InCommand.Extent, InOutStatus );
        break;
    case ERenderCommandType::RecreateDevice:
        InOutStatus.Result = ERenderFrameResult::InvalidState;
        break;
    case ERenderCommandType::Shutdown:
        if ( not bFirstFrame and ( **Queue ).Wait( { NextTimelineValue }, 1.0 ).HasValue() )
        {
            InOutStatus.LastCompletedTimeline = NextTimelineValue;
            UpdateReadback( InOutStatus );
            PresentTarget( InOutStatus );
        }
        InOutStatus.Result = ERenderFrameResult::Shutdown;
        break;
    }
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::CreateResources () noexcept
{
    const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> QueueResult =
        Device->GetQueue( RHI::ERHIQueueType::Graphics, 0U );
    if ( not QueueResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    const TExpected<TSharedRef<RHI::IRHICommandAllocator>, RHI::FRHIError> AllocatorResult =
        Device->CreateCommandAllocator( RHI::ERHIQueueType::Graphics );
    if ( not AllocatorResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    const TExpected<TSharedRef<RHI::IRHICommandList>, RHI::FRHIError> ListResult = Device->CreateCommandList( *AllocatorResult );
    if ( not ListResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    RHI::ERHIResourceUsage TargetUsage = RHI::ERHIResourceUsage::RenderTarget;
    bReadbackEnabled = ReadbackDesc.Capacity > 0ULL and not Swapchain.IsValid() and
                       ReadbackDesc.Capacity >= static_cast<UInt64>( Extent.Width ) * static_cast<UInt64>( Extent.Height ) * 4ULL;
    if ( bReadbackEnabled )
    {
        TargetUsage |= RHI::ERHIResourceUsage::CopySource;
    }

    const TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> TextureResult = Device->CreateTexture( {
        .Extent         = Extent,
        .Format         = RHI::ERHIFormat::RGBA8Unorm,
        .Usage          = TargetUsage,
        .OptimizedClear = RHI::FRHIColorClearValue{},
    } );
    if ( not TextureResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    RHI::FRHITextureViewDesc ViewDesc;
    ViewDesc.Texture                                                             = *TextureResult;
    const TExpected<TSharedRef<RHI::IRHITextureView>, RHI::FRHIError> ViewResult = Device->CreateTextureView( ViewDesc );
    if ( not ViewResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    const TExpected<TSharedRef<RHI::IRHISemaphore>, RHI::FRHIError> SemaphoreResult = Device->CreateSemaphore( {
        .Type         = RHI::ERHISemaphoreType::Timeline,
        .InitialValue = 0ULL,
        .DebugName    = {},
    } );
    if ( not SemaphoreResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    Queue.Emplace( *QueueResult );
    Allocator.Emplace( *AllocatorResult );
    CommandList.Emplace( *ListResult );
    TargetTexture.Emplace( *TextureResult );
    TargetView.Emplace( *ViewResult );
    Timeline.Emplace( *SemaphoreResult );

    RenderingDesc      = RHI::FRHIRenderingDesc{};
    RenderingDesc.Area = RHI::FRHIRect2D{ 0, 0, Extent.Width, Extent.Height };
    RHI::FRHIRenderingAttachment Attachment;
    Attachment.View       = *TargetView;
    Attachment.LoadOp     = RHI::ERHIRenderingLoadOp::Clear;
    Attachment.StoreOp    = RHI::ERHIRenderingStoreOp::Store;
    Attachment.ClearValue = RHI::FRHIColorClearValue{};
    RenderingDesc.ColorAttachments.push_back( Attachment );

    for ( const FPendingGeometry &Geometry : PendingGeometry )
    {
        const FRenderGeometryAssetDesc Desc{
            .Id        = Geometry.Id,
            .Positions = TSpan<const Maths::FVec3f>( Geometry.Positions.data(), Geometry.Positions.size() ),
            .Indices   = TSpan<const UInt32>( Geometry.Indices.data(), Geometry.Indices.size() ) };
        const auto AssetResult = Assets.Register( *Device, **Queue, Desc );
        if ( not AssetResult ) [[unlikely]]
        {
            return MakeUnexpected( ERendererError::GeometryFailed );
        }
    }

    const Bool bHasShaders = not Shaders.VertexBytecode.empty() and not Shaders.PixelBytecode.empty();
    if ( bHasShaders )
    {
        LUMEN_EXPECT( _, CreateForwardResources() );
    }
    if ( Swapchain.IsValid() )
    {
        LUMEN_EXPECT( _, CreatePresentationResources() );
    }
    if ( bReadbackEnabled )
    {
        const auto AllocatorResultRHI = Device->CreateReadbackAllocator( ReadbackDesc.Capacity );
        if ( not AllocatorResultRHI )
        {
            return MakeUnexpected( ERendererError::RHICreationFailed );
        }
        const auto AllocationResult =
            ( *AllocatorResultRHI )
                ->Allocate(
                    { .Size = ReadbackDesc.Capacity, .Alignment = 16ULL, .MemoryClass = RHI::ERHIMemoryClass::Readback } );
        if ( not AllocationResult )
        {
            return MakeUnexpected( ERendererError::RHICreationFailed );
        }
        ReadbackAllocator.Emplace( *AllocatorResultRHI );
        ReadbackAllocation.Emplace( *AllocationResult );
    }

    /** INFO: Asset uploads are control-time submissions on this queue; the frame timeline must continue past them. */
    NextTimelineValue = Queue->Get()->GetCompletedValue().Value;
    bFirstFrame       = true;
    if ( FrameExtension.IsValid() and not FrameExtension->Initialize( *Device, Extent ) )
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    return {};
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::CreateForwardResources () noexcept
{
    if ( Shaders.InlineConstantBytes < 128U )
    {
        return MakeUnexpected( ERendererError::InvalidDesc );
    }

    RHI::FRHIShaderDesc VertexDesc;
    VertexDesc.Stage                          = RHI::ERHIShaderStage::Vertex;
    VertexDesc.Format                         = Shaders.Format;
    VertexDesc.EntryPoint                     = Shaders.VertexEntryPoint;
    VertexDesc.Bytecode                       = TSpan<const Byte>( Shaders.VertexBytecode.data(), Shaders.VertexBytecode.size() );
    VertexDesc.Reflection.InlineConstantBytes = Shaders.InlineConstantBytes;
    const auto VertexResult                   = Device->CreateShader( VertexDesc );
    if ( not VertexResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    RHI::FRHIShaderDesc PixelDesc;
    PixelDesc.Stage                          = RHI::ERHIShaderStage::Pixel;
    PixelDesc.Format                         = Shaders.Format;
    PixelDesc.EntryPoint                     = Shaders.PixelEntryPoint;
    PixelDesc.Bytecode                       = TSpan<const Byte>( Shaders.PixelBytecode.data(), Shaders.PixelBytecode.size() );
    PixelDesc.Reflection.InlineConstantBytes = Shaders.InlineConstantBytes;
    const auto PixelResult                   = Device->CreateShader( PixelDesc );
    if ( not PixelResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    RHI::FRHIPipelineLayoutDesc LayoutDesc;
    LayoutDesc.InlineConstants.push_back(
        { .Offset = 0U, .Size = Shaders.InlineConstantBytes, .Stages = RHI::ERHIShaderStage::AllGraphics } );
    const auto LayoutResult = Device->CreatePipelineLayout( LayoutDesc );
    if ( not LayoutResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    RHI::FRHIGraphicsPipelineDesc PipelineDesc;
    PipelineDesc.VertexShader = *VertexResult;
    PipelineDesc.PixelShader  = *PixelResult;
    PipelineDesc.Layout       = *LayoutResult;
    PipelineDesc.VertexLayout.Buffers.push_back(
        { .Slot = 0U, .Stride = static_cast<UInt32>( sizeof( Maths::FVec3f ) ), .Rate = RHI::ERHIVertexInputRate::Vertex } );
    PipelineDesc.VertexLayout.Attributes.push_back(
        { .Location = 0U, .BufferSlot = 0U, .Format = RHI::ERHIFormat::RGB32Float, .Offset = 0U } );
    PipelineDesc.Topology = RHI::ERHIPrimitiveTopology::TriangleList;
    PipelineDesc.ColorFormats.push_back( RHI::ERHIFormat::RGBA8Unorm );
    const auto PipelineResult = Device->CreateGraphicsPipeline( PipelineDesc );
    if ( not PipelineResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    PipelineLayout = *LayoutResult;
    Pipeline.Emplace( *PipelineResult );

    const FRenderGraphCapacities Capacities{
        .PassCount = 1U, .ResourceCount = 1U, .WriteCountPerPass = 1U, .TransitionCount = 4U };
    GraphBuilder.Emplace( Capacities );
    bForwardEnabled = true;
    return {};
}

Lumen::TExpected<void, Lumen::ERendererError> Lumen::FRenderer::CreatePresentationResources () noexcept
{
    const auto RenderCompleteResult = Device->CreateSemaphore( { .Type = RHI::ERHISemaphoreType::Binary, .DebugName = {} } );
    if ( not RenderCompleteResult ) [[unlikely]]
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }
    RenderComplete.Emplace( *RenderCompleteResult );

    if ( not RecreateSwapchainViews() )
    {
        return MakeUnexpected( ERendererError::RHICreationFailed );
    }

    bPresentation = true;
    return {};
}

Lumen::Bool Lumen::FRenderer::RecreateSwapchainViews ( FRenderFrameStatus *InOutStatus ) noexcept
{
    TVector<TSharedRef<RHI::IRHITextureView>> Replacement;
    Replacement.reserve( Swapchain->GetImageCount() );
    for ( UInt32 Index = 0U; Index < Swapchain->GetImageCount(); ++Index )
    {
        const auto ImageResult = Swapchain->GetImage( Index );
        if ( not ImageResult )
        {
            if ( InOutStatus != nullptr )
            {
                InOutStatus->Result = ERenderFrameResult::RHIError;
            }
            return false;
        }
        RHI::FRHITextureViewDesc ViewDesc;
        ViewDesc.Texture      = *ImageResult;
        const auto ViewResult = Device->CreateTextureView( ViewDesc );
        if ( not ViewResult )
        {
            if ( InOutStatus != nullptr )
            {
                InOutStatus->Result = ERenderFrameResult::RHIError;
            }
            return false;
        }
        Replacement.push_back( *ViewResult );
    }
    SwapchainViews = std::move( Replacement );
    return true;
}

Lumen::Bool Lumen::FRenderer::AcquireTarget ( FRenderFrameStatus &InOutStatus ) noexcept
{
    if ( not bPresentation )
    {
        RenderingDesc.ColorAttachments[0].View = *TargetView;
        return true;
    }

    auto AcquiredResult = Swapchain->AcquireNextImage();
    if ( not AcquiredResult and AcquiredResult.Error().Code == RHI::ERHIErrorCode::OutOfDate )
    {
        if ( not Swapchain->Resize( Extent ).HasValue() or not RecreateSwapchainViews( &InOutStatus ) )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return false;
        }
        AcquiredResult = Swapchain->AcquireNextImage();
    }
    if ( not AcquiredResult )
    {
        InOutStatus.Result = AcquiredResult.Error().Code == RHI::ERHIErrorCode::OutOfDate ? ERenderFrameResult::SwapchainOutOfDate
                                                                                          : ERenderFrameResult::RHIError;
        return false;
    }

    AcquiredImage     = *AcquiredResult;
    bHasAcquiredImage = true;
    if ( AcquiredImage.bSuboptimal )
    {
        InOutStatus.Result = ERenderFrameResult::SwapchainSuboptimal;
    }
    if ( AcquiredImage.ImageIndex >= SwapchainViews.size() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }
    /** INFO: FRHIAcquiredImage carries a non-owning shared pointer; the graph import needs a TSharedRef. */
    const auto ImageResult = Swapchain->GetImage( AcquiredImage.ImageIndex );
    if ( not ImageResult )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }
    AcquiredTarget.Emplace( *ImageResult );

    RenderingDesc.Area                     = RHI::FRHIRect2D{ 0, 0, Extent.Width, Extent.Height };
    RenderingDesc.ColorAttachments[0].View = SwapchainViews[AcquiredImage.ImageIndex];
    return true;
}

void Lumen::FRenderer::PresentTarget ( FRenderFrameStatus &InOutStatus ) noexcept
{
    if ( not bHasAcquiredImage )
    {
        return;
    }
    const RHI::FRHIExpected Presented =
        Swapchain->Present( **Queue, AcquiredImage, bPresentation ? RenderComplete->Get() : nullptr );
    if ( not Presented )
    {
        InOutStatus.Result = Presented.Error().Code == RHI::ERHIErrorCode::OutOfDate ? ERenderFrameResult::SwapchainOutOfDate
                                                                                     : ERenderFrameResult::RHIError;
    }
    else
    {
        ++InOutStatus.PresentCount;
        if ( InOutStatus.Result == ERenderFrameResult::SwapchainSuboptimal or
             InOutStatus.Result == ERenderFrameResult::Enqueued or InOutStatus.Result == ERenderFrameResult::Completed )
        {
            InOutStatus.Result = ERenderFrameResult::Presented;
        }
    }
    bHasAcquiredImage = false;
}

void Lumen::FRenderer::UpdateReadback ( FRenderFrameStatus &InOutStatus ) noexcept
{
    if ( not bReadbackEnabled or not ReadbackAllocator or not ReadbackAllocation )
    {
        return;
    }
    const auto Mapped = ( *ReadbackAllocator )->Map( *ReadbackAllocation );
    if ( not Mapped )
    {
        return;
    }
    /** INFO: The span points into the readback ring and is valid until the next consumed frame reuses that range. */
    ReadbackData.Emplace( Mapped->MappedData );
    InOutStatus.ReadbackByteCount = static_cast<UInt64>( Mapped->MappedData.size() );
    if ( InOutStatus.Result != ERenderFrameResult::RHIError )
    {
        InOutStatus.Result = ERenderFrameResult::ReadbackReady;
    }
}

Lumen::Bool Lumen::FRenderer::RecordForwardPass ( const FRenderSnapshotView &InView,
                                                  RHI::IRHICommandList &InCommandList,
                                                  FRenderFrameStatus &InOutStatus ) noexcept
{
    if ( not bForwardEnabled )
    {
        return true;
    }

    FRenderProxyStats ProxyStats;
    const auto SyncResult = Proxies.Sync( InView.Meshes, InView.Transforms, InView.Header.SnapshotSequence, ProxyStats );
    if ( not SyncResult )
    {
        InOutStatus.Result = ERenderFrameResult::CapacityExceeded;
        return false;
    }
    InOutStatus.ProxyCount = static_cast<UInt64>( Proxies.GetSize() );

    GraphBuilder->Reset();
    const TSharedRef<RHI::IRHITexture> Target = bPresentation ? *AcquiredTarget : *TargetTexture;
    const auto Resource                       = GraphBuilder->Import( Target, { .InitialState = RHI::ERHIResourceState::Common,
                                                                                .FinalState   = RHI::ERHIResourceState::Common,
                                                                                .InitialQueue = RHI::ERHIQueueType::Graphics,
                                                                                .FinalQueue   = RHI::ERHIQueueType::Graphics } );
    const auto Pass                           = GraphBuilder->AddPass( { .bHasSideEffects = true } );
    if ( not Resource or not Pass )
    {
        InOutStatus.Result = ERenderFrameResult::GraphError;
        return false;
    }
    if ( not GraphBuilder->Write( *Pass, *Resource, RHI::ERHIResourceState::RenderTarget ) or
         not GraphBuilder->Export( *Resource ) )
    {
        InOutStatus.Result = ERenderFrameResult::GraphError;
        return false;
    }
    auto CompiledResult = std::move( *GraphBuilder ).Compile();
    if ( not CompiledResult )
    {
        InOutStatus.Result = ERenderFrameResult::GraphError;
        return false;
    }

    std::array<ERenderTransitionRecordPoint, 4U> RecordPoints{};
    USize TransitionCount = 0U;
    for ( const FRenderTransition &Transition : CompiledResult->GetTransitions() )
    {
        if ( TransitionCount < RecordPoints.size() )
        {
            RecordPoints[TransitionCount] = Transition.RecordPoint;
        }
        ++TransitionCount;
    }
    if ( TransitionCount > RecordPoints.size() )
    {
        InOutStatus.Result = ERenderFrameResult::GraphError;
        return false;
    }

    FRenderGraphContext Context( std::move( *CompiledResult ) );
    if ( not RecordGraphTransitions( Context, TSpan<const ERenderTransitionRecordPoint>( RecordPoints.data(), TransitionCount ),
                                     ERenderTransitionRecordPoint::BeforePass, InCommandList ) )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }

    const RHI::FRHIExpected BeginResult    = InCommandList.BeginRendering( RenderingDesc );
    const RHI::FRHIExpected PipelineResult = BeginResult ? InCommandList.SetPipeline( *Pipeline ) : BeginResult;
    if ( not PipelineResult )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }

    std::array<Byte, 128U> Constants{};
    static_assert( sizeof( Maths::FMat4x4f ) == 64U, "The forward pass inline-constant layout requires 64-byte matrices." );
    const FRenderCameraRecord *Camera = nullptr;
    for ( const FRenderCameraRecord &Candidate : InView.Cameras )
    {
        if ( not HasAnyFlags( Candidate.Flags, ERenderCameraFlags::Active ) )
        {
            continue;
        }
        if ( Camera == nullptr or Candidate.ProxyId < Camera->ProxyId )
        {
            Camera = &Candidate;
        }
    }
    const Maths::FMat4x4f ViewMatrix = Camera != nullptr ? Camera->View : Maths::FMat4x4f::Identity();
    const Maths::FMat4x4f Projection = Camera != nullptr ? Camera->Projection : Maths::FMat4x4f::Identity();
    std::memcpy( Constants.data(), &ViewMatrix, sizeof( ViewMatrix ) );
    std::memcpy( Constants.data() + sizeof( ViewMatrix ), &Projection, sizeof( Projection ) );

    if ( not InCommandList.SetInlineConstants( RHI::ERHIShaderStage::AllGraphics, 0U, Constants ).HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }

    for ( const FRenderProxyEntry &Proxy : Proxies.GetEntries() )
    {
        if ( not HasAnyFlags( Proxy.Flags, ERenderMeshFlags::Visible ) )
        {
            continue;
        }
        const FRenderGeometryAsset *Asset = Assets.Find( Proxy.MeshAssetId );
        if ( Asset == nullptr )
        {
            continue;
        }
        const RHI::FRHIExpected BoundVertex = InCommandList.SetVertexBuffer( 0U, Asset->VertexBuffer, 0U );
        const RHI::FRHIExpected BoundIndex =
            BoundVertex ? InCommandList.SetIndexBuffer( Asset->IndexBuffer, 0U, Asset->IndexFormat ) : BoundVertex;
        const RHI::FRHIExpected Drawn =
            BoundIndex ? InCommandList.DrawIndexed( { .IndexCount = Asset->IndexCount } ) : BoundIndex;
        if ( not Drawn )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return false;
        }
        ++InOutStatus.DrawCallCount;
    }

    const RHI::FRHIExpected EndRenderingResult = InCommandList.EndRendering();
    if ( not EndRenderingResult or
         not RecordGraphTransitions( Context, TSpan<const ERenderTransitionRecordPoint>( RecordPoints.data(), TransitionCount ),
                                     ERenderTransitionRecordPoint::AfterPass, InCommandList ) )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return false;
    }
    return true;
}

void Lumen::FRenderer::RenderLatest ( const FRenderFrameRequest &InFrame, FRenderFrameStatus &InOutStatus ) noexcept
{
    InOutStatus.LastRequestedFrame           = InFrame.FrameIndex;
    const RHI::ERHIDeviceStatus DeviceStatus = Device->GetStatus();
    if ( DeviceStatus != RHI::ERHIDeviceStatus::Ready )
    {
        InOutStatus.Result =
            DeviceStatus == RHI::ERHIDeviceStatus::Lost ? ERenderFrameResult::DeviceLost : ERenderFrameResult::DeviceClosed;
        return;
    }

    RHI::IRHIQueue &QueueRef                = **Queue;
    RHI::IRHICommandAllocator &AllocatorRef = **Allocator;
    RHI::IRHICommandList &CommandListRef    = **CommandList;

    if ( not bFirstFrame )
    {
        if ( not QueueRef.Wait( { NextTimelineValue }, 0.0 ).HasValue() )
        {
            InOutStatus.Result = bHasAcquiredImage ? ERenderFrameResult::Enqueued : ERenderFrameResult::Dropped;
            return;
        }
        InOutStatus.LastCompletedTimeline = NextTimelineValue;
        if ( bHasAcquiredImage )
        {
            PresentTarget( InOutStatus );
            if ( InOutStatus.Result == ERenderFrameResult::RHIError )
            {
                return;
            }
        }
        else
        {
            UpdateReadback( InOutStatus );
        }
        if ( not AllocatorRef.Reset( QueueRef.GetCompletedValue() ).HasValue() )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
    }

    /** INFO: The render executor is the only reader allowed to swap this stream. Spans are copied into plain values here. */
    static_cast<void>( SnapshotStream->SwapReadBuffers() );
    const FRenderSnapshotView View   = SnapshotStream->GetReadSnapshot();
    InOutStatus.LastConsumedFrame    = InFrame.FrameIndex;
    InOutStatus.LastConsumedSequence = View.Header.SnapshotSequence;
    InOutStatus.DroppedSnapshotCount = SnapshotStream->GetDroppedSnapshotCount();
    /** INFO: Snapshot sequences start at one; a zero WorldId is the only "no publication yet" marker. */
    if ( View.Header.WorldId == 0ULL )
    {
        InOutStatus.Result = ERenderFrameResult::Idle;
        return;
    }

    if ( bForwardEnabled )
    {
        FRenderProxyStats ProxyStats;
        const auto SyncResult = Proxies.Sync( View.Meshes, View.Transforms, View.Header.SnapshotSequence, ProxyStats );
        if ( not SyncResult )
        {
            InOutStatus.Result = ERenderFrameResult::CapacityExceeded;
            return;
        }
        InOutStatus.ProxyCount = static_cast<UInt64>( Proxies.GetSize() );
    }

    if ( not AcquireTarget( InOutStatus ) )
    {
        return;
    }

    const Float32 Intensity                      = static_cast<Float32>( View.Header.SnapshotSequence % 256ULL ) / 255.0F;
    RenderingDesc.ColorAttachments[0].ClearValue = RHI::FRHIColorClearValue{
        .Color = { Intensity, Intensity, Intensity, 1.0F },
    };

    if ( not CommandListRef.Begin().HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    if ( FrameExtension.IsValid() )
    {
        const TSharedRef<RHI::IRHITexture> Target = bPresentation ? *AcquiredTarget : *TargetTexture;
        const TSharedRef<RHI::IRHITextureView> FrameTargetView =
            bPresentation ? SwapchainViews[AcquiredImage.ImageIndex] : *TargetView;
        const FRenderFrameExtensionContext ExtensionContext{ .Snapshot      = View,
                                                             .CommandList   = CommandListRef,
                                                             .TargetTexture = Target,
                                                             .TargetView    = FrameTargetView,
                                                             .Extent        = Extent,
                                                             .Status        = InOutStatus };
        const RHI::FRHIExpected Recorded = FrameExtension->Record( ExtensionContext );
        if ( not Recorded )
        {
            LUMEN_LOG_ERROR( LogRenderer, "Frame extension record failed: {}", Recorded.Error().Message );
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
    }
    else
    {
        if ( bForwardEnabled and not RecordForwardPass( View, CommandListRef, InOutStatus ) )
        {
            return;
        }
        if ( not bForwardEnabled and
             ( not CommandListRef.BeginRendering( RenderingDesc ).HasValue() or not CommandListRef.EndRendering().HasValue() ) )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
    }

    if ( bReadbackEnabled and ReadbackAllocator and ReadbackAllocation )
    {
        const auto ReadbackBufferResult = ( *ReadbackAllocator )->GetBuffer( *ReadbackAllocation );
        if ( not ReadbackBufferResult or
             not CommandListRef.CopyTextureToBuffer( *TargetTexture, *ReadbackBufferResult,
                                                     { .BufferOffset = 0ULL,
                                                       .RowPitch     = static_cast<UInt64>( Extent.Width ) * 4ULL,
                                                       .SlicePitch   = static_cast<UInt64>( Extent.Width ) * Extent.Height * 4ULL,
                                                       .TextureOffset = {},
                                                       .Extent        = Extent,
                                                       .Mip           = 0U,
                                                       .Layer         = 0U,
                                                       .Aspect        = RHI::ERHIFormatAspect::Color } ) )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
    }

    if ( not CommandListRef.End().HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    const UInt64 QueueCompleted  = QueueRef.GetCompletedValue().Value;
    const UInt64 CompletionValue = ( QueueCompleted > NextTimelineValue ? QueueCompleted : NextTimelineValue ) + 1ULL;
    RHI::FRHISubmitDesc Submit;
    Submit.CommandLists = { *CommandList };
    Submit.Signals      = { { .Semaphore = *Timeline, .Value = CompletionValue } };
    if ( bHasAcquiredImage and AcquiredImage.AcquireSemaphore )
    {
        Submit.Waits.push_back( { .Semaphore = AcquiredImage.AcquireSemaphore, .Value = 1ULL } );
    }
    if ( bPresentation )
    {
        Submit.Signals.push_back( { .Semaphore = *RenderComplete, .Value = 1ULL } );
    }
    Submit.Completion = { CompletionValue };
    if ( not QueueRef.Submit( Submit ).HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    NextTimelineValue                 = CompletionValue;
    InOutStatus.LastSubmittedTimeline = CompletionValue;
    bFirstFrame                       = false;
    if ( FrameExtension.IsValid() )
    {
        FrameExtension->Commit( CompletionValue, InOutStatus );
    }

    if ( QueueRef.Wait( { CompletionValue }, 0.0 ).HasValue() )
    {
        InOutStatus.LastCompletedTimeline = CompletionValue;
        if ( InOutStatus.Result != ERenderFrameResult::SwapchainSuboptimal )
        {
            InOutStatus.Result = ERenderFrameResult::Completed;
        }
        UpdateReadback( InOutStatus );
        PresentTarget( InOutStatus );
    }
    else
    {
        InOutStatus.Result = ERenderFrameResult::Enqueued;
    }
}

void Lumen::FRenderer::ApplyResize ( const RHI::FRHIExtent3D &InExtent, FRenderFrameStatus &InOutStatus ) noexcept
{
    if ( InExtent == Extent )
    {
        return;
    }

    if ( not bFirstFrame )
    {
        if ( not( **Queue ).Wait( { NextTimelineValue }, 1.0 ).HasValue() )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
        InOutStatus.LastCompletedTimeline = NextTimelineValue;
        if ( bHasAcquiredImage )
        {
            PresentTarget( InOutStatus );
        }
        else
        {
            UpdateReadback( InOutStatus );
        }
        if ( InOutStatus.Result == ERenderFrameResult::RHIError or
             not( **Allocator ).Reset( ( **Queue ).GetCompletedValue() ).HasValue() )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
    }

    if ( bPresentation )
    {
        if ( not Swapchain->Resize( InExtent ).HasValue() )
        {
            InOutStatus.Result = ERenderFrameResult::RHIError;
            return;
        }
        if ( not RecreateSwapchainViews( &InOutStatus ) )
        {
            return;
        }
    }

    RHI::ERHIResourceUsage TargetUsage = RHI::ERHIResourceUsage::RenderTarget;
    if ( bReadbackEnabled )
    {
        TargetUsage |= RHI::ERHIResourceUsage::CopySource;
    }
    const TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> TextureResult = Device->CreateTexture( {
        .Extent         = InExtent,
        .Format         = RHI::ERHIFormat::RGBA8Unorm,
        .Usage          = TargetUsage,
        .OptimizedClear = RHI::FRHIColorClearValue{},
    } );
    if ( not TextureResult ) [[unlikely]]
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    RHI::FRHITextureViewDesc ViewDesc;
    ViewDesc.Texture                                                             = *TextureResult;
    const TExpected<TSharedRef<RHI::IRHITextureView>, RHI::FRHIError> ViewResult = Device->CreateTextureView( ViewDesc );
    if ( not ViewResult ) [[unlikely]]
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    Extent = InExtent;
    TargetTexture.Emplace( *TextureResult );
    TargetView.Emplace( *ViewResult );
    RenderingDesc.Area                     = RHI::FRHIRect2D{ 0, 0, Extent.Width, Extent.Height };
    RenderingDesc.ColorAttachments[0].View = *TargetView;
    if ( FrameExtension.IsValid() and not FrameExtension->Resize( Extent ) )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
    }
}
