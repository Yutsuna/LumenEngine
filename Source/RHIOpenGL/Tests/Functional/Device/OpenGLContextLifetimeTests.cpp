/**
 * @file OpenGLContextLifetimeTests.cpp
 * @brief Regression coverage for executor shutdown, deferred retirement and task invocations.
 */

#include <Core/CoreTypes.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>

#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>

#include <RHIOpenGL/Device/OpenGLContext.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/RHIOpenGLModule.hpp>

#include <HAL/PlatformTime.hpp>
#include <Templates/ConditionVariable.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    /** @brief Creates a standalone executor without going through an IRHIDevice. */
    TSharedRef<RHIOpenGL::FOpenGLContext> MakeExecutor ()
    {
        const TExpected<TSharedRef<IRHIModule>, FRHIError> Module = RHIOpenGL::CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );
        LUMEN_CHECK_FALSE( Adapters->empty() );
        const TExpected<TSharedRef<RHIOpenGL::FOpenGLDeviceContext>, FRHIError> DeviceContext =
            RHIOpenGL::FOpenGLDeviceContext::Create( Adapters->front(), false, nullptr );
        LUMEN_CHECK_TRUE( DeviceContext.HasValue() );
        return ( *DeviceContext )->GetExecutor();
    }

    LUMEN_TEST( OpenGLContextLifetime, DeferReleaseRunsSynchronouslyWhenTimelineComplete )
    {
        TSharedRef<RHIOpenGL::FOpenGLContext> Executor = MakeExecutor();

        struct FGate
        {
            HAL::FPlatformMutex Mutex;
            FConditionVariable Condition;
            Bool bOpen = false;
        } Gate;

        TAtomic<Bool> bGateEntered{ false };
        RHIOpenGL::FOpenGLSubmission Blocking;
        Blocking.Completion = { 1ULL };
        Blocking.Replay     = [&Gate, &bGateEntered] () noexcept -> FRHIExpected
        {
            bGateEntered.store( true );
            HAL::TUniqueLock Lock( Gate.Mutex );
            Gate.Condition.Wait( Lock, [&Gate] () noexcept -> Bool
            {
                return Gate.bOpen;
            } );
            return {};
        };
        LUMEN_CHECK_TRUE( Executor->Submit( static_cast<RHIOpenGL::FOpenGLSubmission &&>( Blocking ) ).HasValue() );

        for ( UInt32 Attempt = 0U; Attempt < 200U and not bGateEntered.load(); ++Attempt )
        {
            HAL::FPlatformTime::Sleep( 0.001 );
        }
        LUMEN_CHECK_TRUE( bGateEntered.load() );

        TAtomic<Bool> bReleaseCalled{ false };
        TAtomic<Bool> bCallerReturned{ false };
        FRHIExpected ReleaseResult;
        FThread Caller( [&] () noexcept
        {
            ReleaseResult = Executor->DeferRelease( { 0ULL }, [&bReleaseCalled] () noexcept
            {
                bReleaseCalled.store( true );
            } );
            bCallerReturned.store( true );
        } );

        /* The executor is blocked inside Replay: only the immediate retirement path can
         * make DeferRelease's caller wait for the callback before it returns. */
        HAL::FPlatformTime::Sleep( 0.02 );
        const Bool bWaitedForExecutor = not bCallerReturned.load();
        {
            HAL::TLockGuard Lock( Gate.Mutex );
            Gate.bOpen = true;
        }
        Gate.Condition.NotifyAll();
        Caller.Join();

        LUMEN_CHECK_TRUE( bGateEntered.load() );
        LUMEN_CHECK_TRUE( ReleaseResult.HasValue() );
        LUMEN_CHECK_TRUE( bWaitedForExecutor );
        LUMEN_CHECK_TRUE( bReleaseCalled.load() );
    }

    LUMEN_TEST( OpenGLContextLifetime, DeferredReleasesDrainOnExecutorAtShutdown )
    {
        TAtomic<UInt32> ExecutionThread{ 0U };
        {
            TSharedRef<RHIOpenGL::FOpenGLContext> Executor = MakeExecutor();
            FThread::FId ExecutorId{};
            LUMEN_CHECK_TRUE( Executor
                                  ->ExecuteOnGLThread( [&ExecutorId] () noexcept
            {
                ExecutorId = FThread::GetCurrentId();
            } ).HasValue() );

            LUMEN_CHECK_TRUE( Executor
                                  ->DeferRelease( { 1000ULL }, [&ExecutionThread, ExecutorId] () noexcept
            {
                ExecutionThread.store( FThread::GetCurrentId() == ExecutorId ? 1U : 2U );
            } ).HasValue() );
        }
        LUMEN_CHECK_EQ( ExecutionThread.load(), 1U );
    }

    LUMEN_TEST( OpenGLContextLifetime, InvokesFailFastAfterShutdown )
    {
        TSharedRef<RHIOpenGL::FOpenGLContext> Executor = MakeExecutor();
        TAtomic<UInt32> Succeeded{ 0U };
        TAtomic<UInt32> Rejected{ 0U };
        TAtomic<Bool> bKeepRunning{ true };
        FThread Caller( [&] () noexcept
        {
            while ( bKeepRunning.load() )
            {
                if ( Executor
                         ->ExecuteOnGLThread( [] () noexcept
                {
                } ).HasValue() )
                {
                    Succeeded.fetch_add( 1U );
                }
                else
                {
                    Rejected.fetch_add( 1U );
                }
            }
        } );

        HAL::FPlatformTime::Sleep( 0.001 );
        Executor->Shutdown();
        bKeepRunning.store( false );
        Caller.Join();

        LUMEN_CHECK_TRUE( Succeeded.load() > 0U );
        LUMEN_CHECK_TRUE( Rejected.load() > 0U );
        const FRHIExpected After = Executor->ExecuteOnGLThread( [] () noexcept
        {
        } );
        LUMEN_CHECK_HAS_ERROR( After );
        LUMEN_CHECK_EQ( After.Error().Code, ERHIErrorCode::DeviceLost );
    }

} // namespace

} // namespace Lumen
