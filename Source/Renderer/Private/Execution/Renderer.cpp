/**
 * @file Renderer.cpp
 * @brief Backend-neutral renderer facade implementation.
 */

#include <Execution/Renderer.hpp>

#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/RenderSnapshotView.hpp>

#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Device/RHIDeviceStatus.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Queue/RHISubmitDesc.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHICapabilities.hpp>

namespace
{

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

} // namespace

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
         InDesc.CommandCapacity == 0ULL )
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
    const TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> TextureResult = Device->CreateTexture( {
        .Extent         = Extent,
        .Format         = RHI::ERHIFormat::RGBA8Unorm,
        .Usage          = RHI::ERHIResourceUsage::RenderTarget,
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

    NextTimelineValue = 0ULL;
    bFirstFrame       = true;
    return {};
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

    RHI::IRHIQueue &QueueRef                = **Queue;
    RHI::IRHICommandAllocator &AllocatorRef = **Allocator;
    RHI::IRHICommandList &CommandListRef    = **CommandList;

    if ( not bFirstFrame and not QueueRef.Wait( { NextTimelineValue }, 0.0 ).HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::Dropped;
        return;
    }
    if ( not bFirstFrame and not AllocatorRef.Reset( QueueRef.GetCompletedValue() ).HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    const Float32 Intensity                      = static_cast<Float32>( View.Header.SnapshotSequence % 256ULL ) / 255.0F;
    RenderingDesc.ColorAttachments[0].ClearValue = RHI::FRHIColorClearValue{
        .Color = { Intensity, Intensity, Intensity, 1.0F },
    };

    /** INFO: The queue marks the list Pending during Submit; the renderer must hand it over in Executable state. */
    if ( not CommandListRef.Begin().HasValue() or not CommandListRef.BeginRendering( RenderingDesc ).HasValue() or
         not CommandListRef.EndRendering().HasValue() or not CommandListRef.End().HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    const UInt64 CompletionValue = NextTimelineValue + 1ULL;
    RHI::FRHISubmitDesc Submit;
    Submit.CommandLists = { *CommandList };
    Submit.Signals      = { { .Semaphore = *Timeline, .Value = CompletionValue } };
    Submit.Completion   = { CompletionValue };
    if ( not QueueRef.Submit( Submit ).HasValue() )
    {
        InOutStatus.Result = ERenderFrameResult::RHIError;
        return;
    }

    NextTimelineValue                 = CompletionValue;
    InOutStatus.LastSubmittedTimeline = CompletionValue;
    bFirstFrame                       = false;

    if ( QueueRef.Wait( { CompletionValue }, 0.0 ).HasValue() )
    {
        InOutStatus.LastCompletedTimeline = CompletionValue;
        InOutStatus.Result                = ERenderFrameResult::Completed;
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

    const TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> TextureResult = Device->CreateTexture( {
        .Extent         = InExtent,
        .Format         = RHI::ERHIFormat::RGBA8Unorm,
        .Usage          = RHI::ERHIResourceUsage::RenderTarget,
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
}
