/**
 * @file PlatformSignalTests.cpp
 * @brief Functional tests for FPlatformSignal.
 */

#include <Tests/UnitTest.hpp>

#include <HAL/PlatformSignal.hpp>

#include <thread>

namespace Lumen
{

namespace
{

    UInt32 GHandlerACount                = 0;
    UInt32 GHandlerBCount                = 0;
    HAL::ESystemSignal::Type GLastSignal = HAL::ESystemSignal::None;
    std::thread::id GHandlerThreadId     = std::thread::id();

    void HandlerA ( HAL::ESystemSignal::Type InSignal )
    {
        ++GHandlerACount;
        GLastSignal = InSignal;
    }

    void HandlerB ( HAL::ESystemSignal::Type /*InSignal*/ )
    {
        ++GHandlerBCount;
    }

    void HandlerRecordThread ( HAL::ESystemSignal::Type /*InSignal*/ )
    {
        GHandlerThreadId = std::this_thread::get_id();
    }

    /**
     * @brief RAII teardown: restores OS defaults and clears observed counters.
     * @details Executed even when a LUMEN_CHECK throws mid-test, guaranteeing
     *          the next test starts from a clean signal state.
     */
    struct FSignalTestCleanup
    {
        ~FSignalTestCleanup () noexcept
        {
            HAL::FPlatformSignal::Reset();
            GHandlerACount   = 0;
            GHandlerBCount   = 0;
            GLastSignal      = HAL::ESystemSignal::None;
            GHandlerThreadId = std::thread::id();
        }
    };
} // namespace

LUMEN_TEST( FPlatformSignal_Bind, InvokesHandlerOnRaise )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_EQ( GHandlerACount, 1U );
    LUMEN_CHECK_TRUE( GLastSignal == HAL::ESystemSignal::Interrupt );
}

LUMEN_TEST( FPlatformSignal_Bind, InvokesAllHandlersBoundToSameSignal )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerB );
    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_EQ( GHandlerACount, 1U );
    LUMEN_CHECK_EQ( GHandlerBCount, 1U );
}

LUMEN_TEST( FPlatformSignal_Raise, DoesNotTouchOtherSignals )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Terminate, &HandlerB );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_EQ( GHandlerACount, 1U );
    LUMEN_CHECK_EQ( GHandlerBCount, 0U );
}

LUMEN_TEST( FPlatformSignal_Raise, SequentialRaisesDispatchEachTime )
{
    FSignalTestCleanup Cleanup;

    /** INFO: Raise == raise + Dispatch, so each call must consume the pending bit
     *        and fire the handler again on the next raise. */
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );
    LUMEN_CHECK_EQ( GHandlerACount, 1U );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );
    LUMEN_CHECK_EQ( GHandlerACount, 2U );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );
    LUMEN_CHECK_EQ( GHandlerACount, 3U );
}

LUMEN_TEST( FPlatformSignal_Unbind, RemovesPreviouslyBoundHandler )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Unbind( HAL::ESystemSignal::Interrupt );
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerB );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_EQ( GHandlerACount, 0U );
    LUMEN_CHECK_EQ( GHandlerBCount, 1U );
}

LUMEN_TEST( FPlatformSignal_Reset, ClearsAllBoundHandlers )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Reset();
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerB );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_EQ( GHandlerACount, 0U );
    LUMEN_CHECK_EQ( GHandlerBCount, 1U );
}

LUMEN_TEST( FPlatformSignal_Dispatch, NoPendingSignalsDoesNothing )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Dispatch();

    LUMEN_CHECK_EQ( GHandlerACount, 0U );
}

LUMEN_TEST( FPlatformSignal_Dispatch, ClearsPendingFlagAfterDispatch )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );

    /** Raise performs raise() + dispatch, so the pending bit is consumed once. */
    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );
    LUMEN_CHECK_EQ( GHandlerACount, 1U );

    /** A second dispatch must not invoke the handler again. */
    HAL::FPlatformSignal::Dispatch();
    LUMEN_CHECK_EQ( GHandlerACount, 1U );
}

LUMEN_TEST( FPlatformSignal_Dispatch, MultipleReceivesCoalesceToOneCall )
{
    FSignalTestCleanup Cleanup;

    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );

    /**
     * INFO: bypass HAL::FPlatformSignal::Raise to send the OS signal without dispatching,
     *        so we can observe the coalescing behaviour of PendingSignals (one bit
     *        per signal, regardless of how many times the OS delivered it).
     */
    const Int32 RawSignal = static_cast<Int32>( HAL::ESystemSignal::Interrupt );
    raise( RawSignal );
    raise( RawSignal );
    raise( RawSignal );

    HAL::FPlatformSignal::Dispatch();
    LUMEN_CHECK_EQ( GHandlerACount, 1U );

    HAL::FPlatformSignal::Dispatch(); /** flag cleared — must not re-run */
    LUMEN_CHECK_EQ( GHandlerACount, 1U );
}

LUMEN_TEST( FPlatformSignal_Dispatch, CoalescesAcrossDifferentSignals )
{
    FSignalTestCleanup Cleanup;

    /** INFO: each signal owns its own bit, so multiple different signals received
     *        without dispatch must each fire their own handler once. */
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Terminate, &HandlerB );

    const Int32 RawInterrupt = static_cast<Int32>( HAL::ESystemSignal::Interrupt );
    const Int32 RawTerminate = static_cast<Int32>( HAL::ESystemSignal::Terminate );
    raise( RawInterrupt );
    raise( RawInterrupt );
    raise( RawTerminate );
    raise( RawTerminate );

    HAL::FPlatformSignal::Dispatch();
    LUMEN_CHECK_EQ( GHandlerACount, 1U );
    LUMEN_CHECK_EQ( GHandlerBCount, 1U );
}

LUMEN_TEST( FPlatformSignal_Dispatch, HandlersRunOnCallerThread )
{
    FSignalTestCleanup Cleanup;

    /** INFO: HandleOSSignal is async-signal-safe (pending bit only); handlers run
     *        synchronously inside Dispatch() on the calling thread. */
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerRecordThread );
    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );

    LUMEN_CHECK_TRUE( GHandlerThreadId == std::this_thread::get_id() );
}

LUMEN_TEST( FPlatformSignal_Raise, UnregisteredSignalOnBoundSlotIsNoop )
{
    FSignalTestCleanup Cleanup;

    /**
     * INFO: Bind installs the async-signal-safe OS handler even when the handler
     *        list is empty after Unbind — so Raise() is safe and does not fall back
     *        to the process-killing default.
     */
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &HandlerA );
    HAL::FPlatformSignal::Unbind( HAL::ESystemSignal::Interrupt );

    HAL::FPlatformSignal::Raise( HAL::ESystemSignal::Interrupt );
    LUMEN_CHECK_EQ( GHandlerACount, 0U );
}

} // namespace Lumen
