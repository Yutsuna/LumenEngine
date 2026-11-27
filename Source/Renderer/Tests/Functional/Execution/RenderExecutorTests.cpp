/**
 * @file RenderExecutorTests.cpp
 * @brief Queue, stop/join and allocation contracts of the single render command executor.
 */

#include <Execution/RenderExecutor.hpp>

#include <Tests/UnitTest.hpp>

#include <Thread/Thread.hpp>

#include <Templates/Vector.hpp>

#include <AllocationTracking.hpp>

namespace Lumen
{

namespace
{

    /** @brief Test handler that records command frames and optionally holds the first command. */
    class FRecordingHandler final : public IRenderCommandHandler
    {
    public:

        explicit FRecordingHandler ( const Bool bInBlockFirst ) noexcept : bBlockFirst( bInBlockFirst )
        {
        }

        void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept override
        {
            InOutStatus.LastRequestedFrame = InCommand.Frame.FrameIndex;
            HandledFrames.push_back( InCommand.Frame.FrameIndex );
            HandlerThreads.push_back( FThread::GetCurrentId() );

            if ( bBlockFirst and not bHasBlocked )
            {
                bHasBlocked = true;
                bStarted.store( true, std::memory_order_release );
                USize Spins = 0ULL;
                while ( not bRelease.load( std::memory_order_acquire ) and Spins < 20000000ULL )
                {
                    ++Spins;
                    FThread::Yield();
                }
            }
        }

        void Release () noexcept
        {
            bRelease.store( true, std::memory_order_release );
        }

        void WaitUntilStarted () noexcept
        {
            USize Spins = 0ULL;
            while ( not bStarted.load( std::memory_order_acquire ) and Spins < 20000000ULL )
            {
                ++Spins;
                FThread::Yield();
            }
        }

        [[nodiscard]] Bool WasStarted () const noexcept
        {
            return bStarted.load( std::memory_order_acquire );
        }

        TVector<UInt64> HandledFrames;
        TVector<FThread::FId> HandlerThreads;

    private:

        Bool bBlockFirst = false;
        Bool bHasBlocked = false;
        TAtomic<Bool> bStarted{ false };
        TAtomic<Bool> bRelease{ false };
    };

    /** @brief Allocation-free handler used by the steady-state enqueue contract. */
    class FNoopHandler final : public IRenderCommandHandler
    {
    public:

        void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept override
        {
            InOutStatus.LastRequestedFrame = InCommand.Frame.FrameIndex;
        }
    };

    [[nodiscard]] FRenderCommand MakeFrame ( const UInt64 InFrameIndex ) noexcept
    {
        return FRenderCommand::MakeRender( FRenderFrameRequest{ .FrameIndex = InFrameIndex, .SnapshotSequence = InFrameIndex } );
    }

} // namespace

LUMEN_TEST( FRenderExecutor, InvalidCapacityIsRejected )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_ERROR( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 0ULL }, Handler ) );
    LUMEN_CHECK_FALSE( Executor.IsRunning() );
}

LUMEN_TEST( FRenderExecutor, StartRejectsSecondStart )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 4ULL }, Handler ) );
    const auto Second = Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 4ULL }, Handler );
    LUMEN_CHECK_HAS_ERROR( Second );
    LUMEN_CHECK_EQ( Second.Error(), ERenderExecutorError::AlreadyRunning );
    Executor.RequestStop();
    Executor.Join();
}

LUMEN_TEST( FRenderExecutor, DrainsQueuedCommandsBeforeStop )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 8ULL }, Handler ) );

    for ( UInt64 FrameIndex = 1ULL; FrameIndex <= 4ULL; ++FrameIndex )
    {
        LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( FrameIndex ) ) );
    }
    Executor.RequestStop();
    Executor.Join();

    LUMEN_CHECK_EQ( Handler.HandledFrames.size(), 4ULL );
    for ( USize Index = 0ULL; Index < 4ULL; ++Index )
    {
        LUMEN_CHECK_EQ( Handler.HandledFrames[Index], static_cast<UInt64>( Index + 1ULL ) );
    }
}

LUMEN_TEST( FRenderExecutor, OneThreadConsumesEveryCommand )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 32ULL }, Handler ) );

    for ( UInt64 FrameIndex = 1ULL; FrameIndex <= 32ULL; ++FrameIndex )
    {
        LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( FrameIndex ) ) );
    }
    Executor.RequestStop();
    Executor.Join();

    LUMEN_CHECK_FALSE( Handler.HandlerThreads.empty() );
    const FThread::FId FirstThread = Handler.HandlerThreads[0];
    for ( const FThread::FId ThreadId : Handler.HandlerThreads )
    {
        LUMEN_CHECK_EQ( ThreadId, FirstThread );
    }
}

LUMEN_TEST( FRenderExecutor, QueueFullLeavesAcceptedCommandsUntouched )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( true );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 1ULL }, Handler ) );
    LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( 1ULL ) ) );
    Handler.WaitUntilStarted();
    LUMEN_CHECK_TRUE( Handler.WasStarted() );

    /** INFO: Popping the first command frees exactly one slot; that slot is accepted before saturation. */
    LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( 2ULL ) ) );
    const auto Saturated = Executor.Enqueue( MakeFrame( 3ULL ) );
    LUMEN_CHECK_HAS_ERROR( Saturated );
    LUMEN_CHECK_EQ( Saturated.Error(), ERenderExecutorError::QueueFull );

    Handler.Release();
    Executor.RequestStop();
    Executor.Join();

    LUMEN_CHECK_EQ( Handler.HandledFrames.size(), 2ULL );
    LUMEN_CHECK_EQ( Handler.HandledFrames[0], 1ULL );
    LUMEN_CHECK_EQ( Handler.HandledFrames[1], 2ULL );
}

LUMEN_TEST( FRenderExecutor, EnqueueAfterJoinIsNotRunning )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 2ULL }, Handler ) );
    Executor.RequestStop();
    Executor.Join();

    const auto Result = Executor.Enqueue( MakeFrame( 1ULL ) );
    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error(), ERenderExecutorError::NotRunning );
}

LUMEN_TEST( FRenderExecutor, RequestStopRejectsLaterCommands )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 4ULL }, Handler ) );
    LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( 1ULL ) ) );

    Executor.RequestStop();
    LUMEN_CHECK_FALSE( Executor.IsRunning() );
    const auto Rejected = Executor.Enqueue( MakeFrame( 2ULL ) );
    LUMEN_CHECK_HAS_ERROR( Rejected );
    LUMEN_CHECK_EQ( Rejected.Error(), ERenderExecutorError::NotRunning );

    Executor.Join();
    LUMEN_CHECK_EQ( Handler.HandledFrames.size(), 1ULL );
    LUMEN_CHECK_EQ( Handler.HandledFrames[0], 1ULL );
    Executor.SwapStatusBuffers();
    LUMEN_CHECK_EQ( Executor.GetStatus().RejectedCommandCount, 1ULL );
}

LUMEN_TEST( FRenderExecutor, JoinWithoutRequestStopDrainsAndTerminates )
{
    FRenderExecutor Executor;
    FRecordingHandler Handler( false );
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 4ULL }, Handler ) );

    for ( UInt64 FrameIndex = 1ULL; FrameIndex <= 3ULL; ++FrameIndex )
    {
        LUMEN_CHECK_HAS_VALUE( Executor.Enqueue( MakeFrame( FrameIndex ) ) );
    }
    Executor.Join();

    LUMEN_CHECK_FALSE( Executor.IsRunning() );
    LUMEN_CHECK_EQ( Handler.HandledFrames.size(), 3ULL );
    for ( USize Index = 0ULL; Index < 3ULL; ++Index )
    {
        LUMEN_CHECK_EQ( Handler.HandledFrames[Index], static_cast<UInt64>( Index + 1ULL ) );
    }
}

LUMEN_TEST( FRenderExecutor, EnqueueAfterInitializationDoesNotAllocate )
{
    FRenderExecutor Executor;
    FNoopHandler Handler;
    LUMEN_CHECK_HAS_VALUE( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 1024ULL }, Handler ) );

    {
        const FAllocationTrackingScope Tracking;
        for ( UInt64 FrameIndex = 1ULL; FrameIndex <= 1024ULL; ++FrameIndex )
        {
            const auto Result = Executor.Enqueue( MakeFrame( FrameIndex ) );
            LUMEN_CHECK_TRUE( Result.HasValue() or Result.Error() == ERenderExecutorError::QueueFull );
        }
        LUMEN_CHECK_EQ( Tracking.GetCount(), 0ULL );
    }

    Executor.RequestStop();
    Executor.Join();
}

} // namespace Lumen
