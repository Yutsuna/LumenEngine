/**
 * @file OpenGLFenceCompletionTests.cpp
 * @brief Coverage for fence result classification and single-terminal completion.
 */

#include <Core/CoreTypes.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>
#include <HAL/PlatformTime.hpp>

#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIIdentifiers.hpp>

#include <RHIOpenGL/Device/OpenGLContext.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Device/OpenGLFenceStatus.hpp>
#include <RHIOpenGL/Queue/OpenGLQueue.hpp>
#include <RHIOpenGL/RHIOpenGLModule.hpp>
#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>

#include <Templates/ConditionVariable.hpp>
#include <Templates/Pair.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    static_assert( RHIOpenGL::GlSyncConditionSatisfied == 0x911CU );
    static_assert( RHIOpenGL::GlSyncAlreadySignaled == 0x911AU );
    static_assert( RHIOpenGL::GlSyncTimeoutExpired == 0x911BU );
    static_assert( RHIOpenGL::GlSyncWaitFailed == 0x911DU );

    struct FCompletionLatch
    {
        HAL::FPlatformMutex Mutex;
        FConditionVariable Done;
        Bool bOpen = false;
    };

    TSharedRef<RHIOpenGL::FOpenGLDeviceContext> MakeDeviceContext ()
    {
        const TExpected<TSharedRef<IRHIModule>, FRHIError> Module = RHIOpenGL::CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );
        LUMEN_CHECK_FALSE( Adapters->empty() );
        const TExpected<TSharedRef<RHIOpenGL::FOpenGLDeviceContext>, FRHIError> DeviceContext =
            RHIOpenGL::FOpenGLDeviceContext::Create( Adapters->front(), false, nullptr );
        LUMEN_CHECK_TRUE( DeviceContext.HasValue() );
        return *DeviceContext;
    }

    LUMEN_TEST( OpenGLFenceCompletion, ClassifiesFenceWaitStatuses )
    {
        using RHIOpenGL::EFenceWaitStatus;
        LUMEN_CHECK_EQ( RHIOpenGL::ClassifyFenceWaitStatus( RHIOpenGL::GlSyncConditionSatisfied ), EFenceWaitStatus::Complete );
        LUMEN_CHECK_EQ( RHIOpenGL::ClassifyFenceWaitStatus( RHIOpenGL::GlSyncAlreadySignaled ), EFenceWaitStatus::Complete );
        LUMEN_CHECK_EQ( RHIOpenGL::ClassifyFenceWaitStatus( RHIOpenGL::GlSyncTimeoutExpired ), EFenceWaitStatus::Timeout );
        LUMEN_CHECK_EQ( RHIOpenGL::ClassifyFenceWaitStatus( RHIOpenGL::GlSyncWaitFailed ), EFenceWaitStatus::Failed );
        LUMEN_CHECK_EQ( RHIOpenGL::ClassifyFenceWaitStatus( 0xDEADBEEFU ), EFenceWaitStatus::Failed );
    }

    LUMEN_TEST( OpenGLFenceCompletion, ReplayFailureTerminatesExactlyOnce )
    {
        const TSharedRef<RHIOpenGL::FOpenGLDeviceContext> DeviceContext = MakeDeviceContext();
        const TSharedRef<RHIOpenGL::FOpenGLContext> Executor            = DeviceContext->GetExecutor();

        const TExpected<TSharedRef<RHIOpenGL::FOpenGLSemaphore>, FRHIError> Semaphore = RHIOpenGL::FOpenGLSemaphore::Create(
            DeviceContext, FRHISemaphoreDesc{ .Type = ERHISemaphoreType::Timeline, .InitialValue = 0ULL, .DebugName = {} },
            FRHIObjectId{ 1ULL } );
        LUMEN_CHECK_TRUE( Semaphore.HasValue() );

        FCompletionLatch Latch;
        TAtomic<UInt32> Failures{ 0U };
        TAtomic<UInt32> Completions{ 0U };

        RHIOpenGL::FOpenGLSubmission Failed;
        Failed.Completion = { 1ULL };
        Failed.Replay     = [] () noexcept -> FRHIExpected
        {
            return TUnexpected( MakeRHIBackendFailure( ERHIErrorCondition::Command, "Injected replay failure." ) );
        };
        const TSharedPtr<IRHISemaphore> Retained = *Semaphore;
        Failed.Signals.push_back( TPair<TSharedPtr<IRHISemaphore>, UInt64>( Retained, 7ULL ) );
        Failed.OnComplete = [&Completions] () noexcept
        {
            Completions.fetch_add( 1U );
        };
        Failed.OnFailure = [&] ( const FRHIError & ) noexcept
        {
            Failures.fetch_add( 1U );
            {
                HAL::TLockGuard Lock( Latch.Mutex );
                Latch.bOpen = true;
            }
            Latch.Done.NotifyAll();
        };

        LUMEN_CHECK_TRUE( Executor->Submit( static_cast<RHIOpenGL::FOpenGLSubmission &&>( Failed ) ).HasValue() );
        {
            HAL::TUniqueLock Lock( Latch.Mutex );
            LUMEN_CHECK_TRUE( Latch.Done.WaitFor( Lock, 5.0, [&Latch] () noexcept -> Bool
            {
                return Latch.bOpen;
            } ) );
        }

        LUMEN_CHECK_EQ( Failures.load(), 1U );
        LUMEN_CHECK_EQ( Completions.load(), 0U );
        LUMEN_CHECK_EQ( ( *Semaphore )->GetValue(), 0ULL );
        LUMEN_CHECK_TRUE( Executor->GetCompletedTimeline().Value >= 1ULL );

        /* The executor must still accept and complete later work. */
        FCompletionLatch SuccessLatch;
        TAtomic<UInt32> SuccessCompletions{ 0U };
        RHIOpenGL::FOpenGLSubmission Succeeded;
        Succeeded.Completion = { 2ULL };
        Succeeded.Replay     = [] () noexcept -> FRHIExpected
        {
            return {};
        };
        Succeeded.OnComplete = [&] () noexcept
        {
            SuccessCompletions.fetch_add( 1U );
            {
                HAL::TLockGuard Lock( SuccessLatch.Mutex );
                SuccessLatch.bOpen = true;
            }
            SuccessLatch.Done.NotifyAll();
        };
        LUMEN_CHECK_TRUE( Executor->Submit( static_cast<RHIOpenGL::FOpenGLSubmission &&>( Succeeded ) ).HasValue() );
        {
            HAL::TUniqueLock Lock( SuccessLatch.Mutex );
            LUMEN_CHECK_TRUE( SuccessLatch.Done.WaitFor( Lock, 5.0, [&SuccessLatch] () noexcept -> Bool
            {
                return SuccessLatch.bOpen;
            } ) );
        }
        LUMEN_CHECK_EQ( SuccessCompletions.load(), 1U );
        LUMEN_CHECK_TRUE( Executor->GetCompletedTimeline().Value >= 2ULL );
    }

    /** @brief D9: a clean closure retires in-flight work with DeviceClosed, not DeviceLost. */
    LUMEN_TEST( OpenGLFenceCompletion, CloseFailsPendingSubmissionsWithDeviceClosed )
    {
        const TSharedRef<RHIOpenGL::FOpenGLDeviceContext> DeviceContext = MakeDeviceContext();
        const TSharedRef<RHIOpenGL::FOpenGLContext> Executor            = DeviceContext->GetExecutor();

        const TExpected<TSharedRef<RHIOpenGL::FOpenGLSemaphore>, FRHIError> Semaphore = RHIOpenGL::FOpenGLSemaphore::Create(
            DeviceContext, FRHISemaphoreDesc{ .Type = ERHISemaphoreType::Timeline, .InitialValue = 0ULL, .DebugName = {} },
            FRHIObjectId{ 1ULL } );
        LUMEN_CHECK_TRUE( Semaphore.HasValue() );
        const TSharedPtr<IRHISemaphore> Retained = *Semaphore;

        TAtomic<UInt32> Failures{ 0U };
        TAtomic<UInt32> Completions{ 0U };
        TAtomic<UInt32> FailureCode{ 0U };

        RHIOpenGL::FOpenGLSubmission Abandoned;
        Abandoned.Completion = { 1ULL };
        /* The never-signalled wait keeps the packet blocked until the close retires it. */
        Abandoned.Waits.push_back( TPair<TSharedPtr<IRHISemaphore>, UInt64>( Retained, 1ULL ) );
        Abandoned.Signals.push_back( TPair<TSharedPtr<IRHISemaphore>, UInt64>( Retained, 7ULL ) );
        Abandoned.OnComplete = [&Completions] () noexcept
        {
            Completions.fetch_add( 1U );
        };
        Abandoned.OnFailure = [&] ( const FRHIError &InError ) noexcept
        {
            FailureCode.store( static_cast<UInt32>( InError.Code ) );
            Failures.fetch_add( 1U );
        };
        LUMEN_CHECK_TRUE( Executor->Submit( static_cast<RHIOpenGL::FOpenGLSubmission &&>( Abandoned ) ).HasValue() );

        DeviceContext->MarkClosed();
        DeviceContext->MarkClosed(); /* Idempotent: no second retirement. */

        LUMEN_CHECK_EQ( Failures.load(), 1U );
        LUMEN_CHECK_EQ( Completions.load(), 0U );
        LUMEN_CHECK_EQ( FailureCode.load(), static_cast<UInt32>( ERHIErrorCode::DeviceClosed ) );
        LUMEN_CHECK_EQ( ( *Semaphore )->GetValue(), 0ULL );
        const TOptional<FRHIError> Unavailable = DeviceContext->GetUnavailableError( "test" );
        LUMEN_CHECK_TRUE( Unavailable.HasValue() );
        LUMEN_CHECK_EQ( Unavailable->Code, ERHIErrorCode::DeviceClosed );
    }

    /** @brief D9: waiters are released only after terminal callbacks have run (D7 ordering). */
    LUMEN_TEST( OpenGLFenceCompletion, CloseWakesWaitersAfterTerminalCallbacks )
    {
        const TSharedRef<RHIOpenGL::FOpenGLDeviceContext> DeviceContext               = MakeDeviceContext();
        const TExpected<TSharedRef<RHIOpenGL::FOpenGLSemaphore>, FRHIError> Semaphore = RHIOpenGL::FOpenGLSemaphore::Create(
            DeviceContext, FRHISemaphoreDesc{ .Type = ERHISemaphoreType::Timeline, .InitialValue = 0ULL, .DebugName = {} },
            FRHIObjectId{ 1ULL } );
        LUMEN_CHECK_TRUE( Semaphore.HasValue() );

        TAtomic<Bool> bTerminal{ false };
        RHIOpenGL::FOpenGLSubmission Blocked;
        Blocked.Completion = { 1ULL };
        Blocked.Waits.push_back( TPair<TSharedPtr<IRHISemaphore>, UInt64>( TSharedPtr<IRHISemaphore>( *Semaphore ), 1ULL ) );
        Blocked.OnFailure = [&bTerminal] ( const FRHIError & ) noexcept
        {
            bTerminal.store( true );
        };
        LUMEN_CHECK_TRUE(
            DeviceContext->GetExecutor()->Submit( static_cast<RHIOpenGL::FOpenGLSubmission &&>( Blocked ) ).HasValue() );

        TAtomic<Bool> bSawTerminal{ false };
        FThread Waiter( [&] () noexcept
        {
            RHIOpenGL::FOpenGLWaitState *State = DeviceContext->GetWaitState();
            HAL::TUniqueLock Lock( State->Mutex );
            State->Condition.Wait( Lock, [State] () noexcept -> Bool
            {
                return State->bUnavailable;
            } );
            bSawTerminal.store( bTerminal.load() );
        } );

        HAL::FPlatformTime::Sleep( 0.01 );
        DeviceContext->MarkClosed();
        Waiter.Join();
        LUMEN_CHECK_TRUE( bSawTerminal.load() );
    }

    /** @brief D9: all queue operations report DeviceClosed after a clean closure. */
    LUMEN_TEST( OpenGLFenceCompletion, QueueReportsDeviceClosedAfterClose )
    {
        const TSharedRef<RHIOpenGL::FOpenGLDeviceContext> DeviceContext = MakeDeviceContext();
        const TSharedRef<RHIOpenGL::FOpenGLQueue> Queue =
            RHIOpenGL::FOpenGLQueue::Create( FRHIObjectId{ 2ULL }, DeviceContext, ERHIQueueType::Graphics, 0U );

        const FRHIExpected Before = Queue->Wait( { 1ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( Before );
        LUMEN_CHECK_EQ( Before.Error().Code, ERHIErrorCode::Timeout );

        DeviceContext->MarkClosed();

        const FRHIExpected After = Queue->Wait( { 1ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( After );
        LUMEN_CHECK_EQ( After.Error().Code, ERHIErrorCode::DeviceClosed );
        const FRHIExpected Idle = Queue->WaitIdle();
        LUMEN_CHECK_HAS_ERROR( Idle );
        LUMEN_CHECK_EQ( Idle.Error().Code, ERHIErrorCode::DeviceClosed );

        FRHISubmitDesc Submit;
        Submit.Completion           = { 1ULL };
        const FRHIExpected Rejected = Queue->Submit( Submit );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Rejected.Error().Code, ERHIErrorCode::DeviceClosed );
    }

} // namespace

} // namespace Lumen
