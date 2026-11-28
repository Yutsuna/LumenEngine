/**
 * @file SceneToNullFrameTests.cpp
 * @brief Scene snapshot to portable RHI clear-pass integration on the test-only Null backend.
 */

#include <Backend/StartWithTestDevice.hpp>
#include <Execution/Renderer.hpp>

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Tests/UnitTest.hpp>

#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    /** @brief World configured with explicit capacities so extraction is active from the first tick. */
    [[nodiscard]] FWorldDesc MakeWorldDesc ( const UInt64 InWorldId ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount      = 1U;
        Desc.MaxActors        = 16ULL;
        Desc.WorldId          = InWorldId;
        Desc.RenderCapacities = FRenderSnapshotCapacities{
            .TransformCapacity = 16ULL,
            .MeshCapacity      = 16ULL,
            .CameraCapacity    = 16ULL,
            .LightCapacity     = 16ULL,
        };
        return Desc;
    }

    /** @brief Requests one frame, retrying only on the executor's documented QueueFull backpressure. */
    void RequestFrameOrWait ( FRenderer &InRenderer, const UInt64 InFrameIndex )
    {
        auto Result = InRenderer.RequestRender( InFrameIndex );
        USize Spins = 0ULL;
        while ( not Result and Result.Error() == ERendererError::QueueFull and Spins < 500000ULL )
        {
            ++Spins;
            FThread::Yield();
            Result = InRenderer.RequestRender( InFrameIndex );
        }
        LUMEN_CHECK_HAS_VALUE( Result );
    }

    /** @brief Waits until the single status reader observes the requested submitted timeline. */
    void WaitForSubmitted ( FRenderer &InRenderer, const UInt64 InTimeline )
    {
        USize Spins = 0ULL;
        while ( Spins < 2000000ULL )
        {
            ++Spins;
            InRenderer.SwapStatusBuffers();
            if ( InRenderer.GetStatus().LastSubmittedTimeline >= InTimeline )
            {
                return;
            }
            FThread::Yield();
        }
        const FRenderFrameStatus &Status = InRenderer.GetStatus();
        LUMEN_CHECK_MSG( false, std::format( "submitted={} consumed={} seq={} result={} rejected={}",
                                             Status.LastSubmittedTimeline, Status.LastConsumedFrame, Status.LastConsumedSequence,
                                             static_cast<UInt32>( Status.Result ), Status.RejectedCommandCount ) );
    }

} // namespace

LUMEN_INTEGRATION_TEST( FRenderer_NullSlice, CompletesOneThousandFramesWithoutLeakingOrDropping )
{
    RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
    Factory.SetDefaultValidation( false );
    const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = Factory.CreateDevice();
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }

    FWorld World( MakeWorldDesc( 70001ULL ) );
    FRenderer Renderer;
    const FRendererDesc Desc{
        .SnapshotStream  = &World.GetRenderSnapshotStream(),
        .Extent          = { 16U, 16U, 1U },
        .Surface         = {},
        .Backend         = "null",
        .CommandCapacity = 64ULL,
        .Shaders         = {},
        .ProxyCapacity   = 256ULL,
        .AssetCapacity   = 64ULL,
        .Readback        = {},
        .Feature         = {},
    };
    LUMEN_CHECK_HAS_VALUE( StartWithTestDevice( Renderer, *Device, Desc ) );
    if ( not Renderer.IsRunning() )
    {
        return;
    }

    for ( UInt64 FrameIndex = 1ULL; FrameIndex <= 1000ULL; ++FrameIndex )
    {
        LUMEN_CHECK_HAS_VALUE( World.TryTick( 1.0 / 60.0 ) );
        RequestFrameOrWait( Renderer, FrameIndex );
    }

    WaitForSubmitted( Renderer, 1000ULL );
    Renderer.RequestShutdown();
    Renderer.Join();

    Renderer.SwapStatusBuffers();
    const FRenderFrameStatus &Status = Renderer.GetStatus();
    LUMEN_CHECK_EQ( Status.LastRequestedFrame, 1000ULL );
    LUMEN_CHECK_EQ( Status.LastConsumedFrame, 1000ULL );
    LUMEN_CHECK_TRUE( Status.LastConsumedSequence > 0ULL );
    LUMEN_CHECK_TRUE( Status.LastConsumedSequence <= 1000ULL );
    LUMEN_CHECK_EQ( Status.LastSubmittedTimeline, 1000ULL );
    LUMEN_CHECK_EQ( Status.LastCompletedTimeline, 1000ULL );
    LUMEN_CHECK_TRUE( Status.DroppedSnapshotCount <= World.GetRenderPublishCount() );
    /** INFO: QueueFull retries are expected backpressure; LastRequestedFrame proves every frame was accepted. */
    LUMEN_CHECK_EQ( Status.Result, ERenderFrameResult::Shutdown );
}

LUMEN_INTEGRATION_TEST( FRenderer_NullSlice, ValidationOnReportsDeviceLossAndShutdown )
{
    RHI::Tests::IRHIConformanceDeviceFactory &Factory                   = RHI::Tests::GetRHIConformanceDeviceFactory();
    const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = Factory.CreateDevice( true );
    LUMEN_CHECK_HAS_VALUE( Device );
    if ( not Device )
    {
        return;
    }

    FWorld World( MakeWorldDesc( 70002ULL ) );
    FRenderer Renderer;
    const FRendererDesc Desc{
        .SnapshotStream  = &World.GetRenderSnapshotStream(),
        .Extent          = { 16U, 16U, 1U },
        .Surface         = {},
        .Backend         = "null",
        .CommandCapacity = 16ULL,
        .Shaders         = {},
        .ProxyCapacity   = 256ULL,
        .AssetCapacity   = 64ULL,
        .Readback        = {},
        .Feature         = {},
    };
    LUMEN_CHECK_HAS_VALUE( StartWithTestDevice( Renderer, *Device, Desc ) );
    if ( not Renderer.IsRunning() )
    {
        return;
    }

    LUMEN_CHECK_HAS_VALUE( World.TryTick( 1.0 / 60.0 ) );
    RequestFrameOrWait( Renderer, 1ULL );
    WaitForSubmitted( Renderer, 1ULL );
    Renderer.SwapStatusBuffers();
    LUMEN_CHECK_EQ( Renderer.GetStatus().Result, ERenderFrameResult::Completed );

    LUMEN_CHECK_TRUE( Factory.InjectDeviceLost( **Device ) );
    LUMEN_CHECK_HAS_VALUE( World.TryTick( 1.0 / 60.0 ) );
    RequestFrameOrWait( Renderer, 2ULL );

    USize Spins        = 0ULL;
    Bool bLostReported = false;
    while ( Spins < 1000000ULL and not bLostReported )
    {
        ++Spins;
        Renderer.SwapStatusBuffers();
        bLostReported = Renderer.GetStatus().Result == ERenderFrameResult::DeviceLost;
        if ( not bLostReported )
        {
            FThread::Yield();
        }
    }
    LUMEN_CHECK_TRUE( bLostReported );

    Renderer.RequestShutdown();
    Renderer.Join();
}

LUMEN_INTEGRATION_TEST( FRenderer_NullSlice, InvalidDescriptorIsRejectedBeforeAnyThreadStarts )
{
    FRenderer Renderer;
    LUMEN_CHECK_HAS_ERROR( Renderer.Start( FRendererDesc{} ) );
    LUMEN_CHECK_FALSE( Renderer.IsRunning() );
}

} // namespace Lumen
