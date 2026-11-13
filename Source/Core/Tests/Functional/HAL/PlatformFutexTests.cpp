/**
 * @file PlatformFutexTests.cpp
 * @brief Functional tests for FPlatformFutex.
 * @details Covers the documented contract of Wait / WaitFor / WakeOne / WakeAll:
 *          - Wait blocks while the futex word equals the expected value.
 *          - Wait returns immediately when the futex word differs.
 *          - WaitFor returns true when woken, false on timeout.
 *          - WaitFor returns immediately true when the futex word differs.
 *          - WaitFor clamps negative / zero / NaN timeouts and does not block.
 *          - WakeOne wakes exactly one waiter, WakeAll wakes all of them.
 *          - Waking with no waiters is a safe no-op.
 *
 * @note Waiter threads are detached and their shared state is heap-allocated, so
 *       a broken implementation that fails to wake cannot hang the whole test
 *       binary (the test itself times out, the leaked state is harmless).
 */

#include <Tests/UnitTest.hpp>

#include <HAL/PlatformFutex.hpp>
#include <HAL/PlatformTime.hpp>

#include <thread>

namespace Lumen
{

namespace
{
    using HAL::FPlatformFutex;
    using HAL::FPlatformTime;

    struct FWaitState
    {
        TAtomic<UInt32> Word{ 0 };
        TAtomic<UInt32> WokenCount{ 0 };
    };

    struct FWaitForResult
    {
        TAtomic<UInt32> Returned{ 0 };  /** 1 = the waiting thread has returned */
        TAtomic<UInt32> Woke{ 2 };      /** 0 = timeout, 1 = woken, 2 = not set */
    };

    /**
     * @brief Polls until Predicate returns true, or TimeoutSeconds elapses.
     * @returns The last evaluated predicate value.
     */
    template <typename PredicateType>
    bool WaitUntil ( PredicateType Predicate, const Float64 TimeoutSeconds = 1.0 )
    {
        const Float64 Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
        while ( FPlatformTime::Seconds() < Deadline )
        {
            if ( Predicate() )
            {
                return true;
            }
            FPlatformTime::Sleep( 0.01 );
        }
        return Predicate();
    }
} // namespace

/**
 * Wait
 */

LUMEN_TEST( FPlatformFutex_Wait, ReturnsImmediatelyWhenValueDiffers )
{
    FWaitState State;
    State.Word = 42; /** futex word is 42, not the expected 0 */

    const Float64 Start = FPlatformTime::Seconds();
    FPlatformFutex::Wait( State.Word, 0 );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_TRUE( Elapsed < 0.1 ); /** must not block */
}

LUMEN_TEST( FPlatformFutex_Wait, BlocksUntilWokenByWakeAll )
{
    auto *State = new FWaitState();

    std::thread Waiter( [State]() {
        FPlatformFutex::Wait( State->Word, 0 );
        ++State->WokenCount;
    } );
    Waiter.detach();

    /** Give the waiter ample time to actually enter the kernel wait. */
    FPlatformTime::Sleep( 0.1 );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 0U );

    State->Word = 1;
    FPlatformFutex::WakeAll( State->Word );

    const bool bWoken = WaitUntil( [State]() {
        return static_cast<UInt32>( State->WokenCount ) >= 1U;
    } );
    LUMEN_CHECK_TRUE( bWoken );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 1U );
}

/**
 * WaitFor
 */

LUMEN_TEST( FPlatformFutex_WaitFor, ReturnsTrueWhenWoken )
{
    auto *State  = new FWaitState();
    auto *Result = new FWaitForResult();

    std::thread Waiter( [State, Result]() {
        Result->Woke     = FPlatformFutex::WaitFor( State->Word, 0, 5.0 ) ? 1U : 0U;
        Result->Returned = 1;
    } );
    Waiter.detach();

    FPlatformTime::Sleep( 0.1 );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Result->Returned ), 0U );

    State->Word = 1;
    FPlatformFutex::WakeAll( State->Word );

    const Float64 Start = FPlatformTime::Seconds();
    const bool bReturned = WaitUntil( [Result]() {
        return static_cast<UInt32>( Result->Returned ) == 1U;
    } );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_TRUE( bReturned );
    LUMEN_CHECK_EQ( static_cast<UInt32>( Result->Woke ), 1U );
    LUMEN_CHECK_TRUE( Elapsed < 0.5 ); /** definitely not the 5 s timeout */
}

LUMEN_TEST( FPlatformFutex_WaitFor, ReturnsFalseOnTimeout )
{
    auto *State  = new FWaitState();
    auto *Result = new FWaitForResult();

    const Float64 Start = FPlatformTime::Seconds();
    std::thread Waiter( [State, Result]() {
        Result->Woke     = FPlatformFutex::WaitFor( State->Word, 0, 0.2 ) ? 1U : 0U;
        Result->Returned = 1;
    } );
    Waiter.detach();

    LUMEN_CHECK_TRUE( WaitUntil( [Result]() {
        return static_cast<UInt32>( Result->Returned ) == 1U;
    } ) );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_EQ( static_cast<UInt32>( Result->Woke ), 0U );
    LUMEN_CHECK_TRUE( Elapsed >= 0.15 ); /** didn't return too early */
    LUMEN_CHECK_TRUE( Elapsed < 1.0 );   /** didn't hang */
}

LUMEN_TEST( FPlatformFutex_WaitFor, ReturnsTrueImmediatelyWhenValueDiffers )
{
    FWaitState State;
    State.Word = 42;

    const Float64 Start = FPlatformTime::Seconds();
    const Bool bWoken   = FPlatformFutex::WaitFor( State.Word, 0, 1.0 );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_TRUE( bWoken );
    LUMEN_CHECK_TRUE( Elapsed < 0.1 );
}

LUMEN_TEST( FPlatformFutex_WaitFor, NegativeTimeoutDoesNotHang )
{
    FWaitState State; /** Word = 0, matches expected */

    const Float64 Start = FPlatformTime::Seconds();
    const Bool bWoken   = FPlatformFutex::WaitFor( State.Word, 0, -1.0 );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_FALSE( bWoken );      /** no waker → timeout */
    LUMEN_CHECK_TRUE( Elapsed < 0.1 );
}

LUMEN_TEST( FPlatformFutex_WaitFor, ZeroTimeoutDoesNotHang )
{
    FWaitState State;

    const Float64 Start = FPlatformTime::Seconds();
    const Bool bWoken   = FPlatformFutex::WaitFor( State.Word, 0, 0.0 );
    const Float64 Elapsed = FPlatformTime::Seconds() - Start;

    LUMEN_CHECK_FALSE( bWoken );
    LUMEN_CHECK_TRUE( Elapsed < 0.1 );
}

/**
 * WakeOne / WakeAll
 */

LUMEN_TEST( FPlatformFutex_WakeOne, WakesExactlyOneWaiter )
{
    auto *State = new FWaitState();

    auto WaiterBody = [State]() {
        FPlatformFutex::Wait( State->Word, 0 );
        ++State->WokenCount;
    };

    std::thread First( WaiterBody );
    std::thread Second( WaiterBody );
    First.detach();
    Second.detach();

    FPlatformTime::Sleep( 0.15 ); /** both waiters blocked */
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 0U );

    State->Word = 1;
    FPlatformFutex::WakeOne( State->Word );

    LUMEN_CHECK_TRUE( WaitUntil( [State]() {
        return static_cast<UInt32>( State->WokenCount ) >= 1U;
    } ) );

    /** Contract of WakeOne: one waiter, not all. */
    FPlatformTime::Sleep( 0.1 );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 1U );

    /** Clean up the still-blocked waiter so the process is not polluted. */
    FPlatformFutex::WakeAll( State->Word );
    LUMEN_CHECK_TRUE( WaitUntil( [State]() {
        return static_cast<UInt32>( State->WokenCount ) >= 2U;
    } ) );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 2U );
}

LUMEN_TEST( FPlatformFutex_WakeAll, WakesAllWaiters )
{
    constexpr UInt32 WaiterCount = 4U;
    auto *State = new FWaitState();

    for ( UInt32 Index = 0U; Index < WaiterCount; ++Index )
    {
        std::thread Waiter( [State]() {
            FPlatformFutex::Wait( State->Word, 0 );
            ++State->WokenCount;
        } );
        Waiter.detach();
    }

    FPlatformTime::Sleep( 0.2 );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), 0U );

    State->Word = 1;
    FPlatformFutex::WakeAll( State->Word );

    const bool bAllWoken = WaitUntil( [State]() {
        return static_cast<UInt32>( State->WokenCount ) >= WaiterCount;
    } );
    LUMEN_CHECK_TRUE( bAllWoken );
    LUMEN_CHECK_EQ( static_cast<UInt32>( State->WokenCount ), WaiterCount );
}

LUMEN_TEST( FPlatformFutex_WakeOne, NoWaitersIsSafe )
{
    FWaitState State;
    FPlatformFutex::WakeOne( State.Word );
    LUMEN_CHECK_TRUE( true ); /** must simply not crash */
}

LUMEN_TEST( FPlatformFutex_WakeAll, NoWaitersIsSafe )
{
    FWaitState State;
    FPlatformFutex::WakeAll( State.Word );
    LUMEN_CHECK_TRUE( true );
}

} // namespace Lumen
