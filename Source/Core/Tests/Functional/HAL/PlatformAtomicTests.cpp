/**
 * @file PlatformAtomicTests.cpp
 * @brief Functional tests for HAL::CpuRelax (platform atomic/yield hint).
 */

#include <HAL/PlatformAtomic.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>
#include <thread>

using namespace std::chrono_literals;

namespace Lumen
{

/**
 * basic behaviour
 */

LUMEN_TEST( CpuRelax_Basic, RepeatedCallsAreSafe )
{
    /**
     * INFO: CpuRelax is only a pipeline hint; repeated invocation must be
     * side-effect free, callable from any thread, and never trap at runtime.
     */
    for ( Int32 Index = 0; Index < 1000; ++Index )
    {
        HAL::CpuRelax();
    }

    LUMEN_CHECK_TRUE( true );
}

LUMEN_TEST( CpuRelax_Basic, CallableInTightLoop )
{
    UInt32 LoopCount = 0U;

    while ( LoopCount < 100U )
    {
        HAL::CpuRelax();
        ++LoopCount;
    }

    LUMEN_CHECK_EQ( LoopCount, 100U );
}

/**
 * memory ordering behaviour
 */

LUMEN_TEST( CpuRelax_MemoryOrdering, SpinWaitObservesOtherThreadStore )
{
    /**
     * INFO: CpuRelax compiles to pause/yield which is a memory barrier hint;
     * a producer/consumer handshake around the spin loop must not reorder
     * the observed store away.
     */
    TAtomic<Int32> State{ 0 };

    std::thread Producer( [&] ()
    {
        HAL::CpuRelax();
        State.store( 7, std::memory_order_release );
    } );

    while ( State.load( std::memory_order_acquire ) == 0 )
    {
        HAL::CpuRelax();
    }

    Producer.join();

    LUMEN_CHECK_EQ( State.load( std::memory_order_acquire ), 7 );
}

LUMEN_TEST( CpuRelax_MemoryOrdering, RelaxedTTASSpinLoopTerminates )
{
    /**
     * INFO: mirrors the TTAS usage pattern of FSpinLock: a relaxed loop on a
     * flag set from another thread terminates once the value is observed.
     */
    TAtomic<Bool> Done{ false };

    std::thread Finisher( [&] ()
    {
        std::this_thread::sleep_for( 10ms );
        Done.store( true, std::memory_order_release );
    } );

    while ( not Done.load( std::memory_order_acquire ) )
    {
        HAL::CpuRelax();
    }

    Finisher.join();

    LUMEN_CHECK_TRUE( Done.load( std::memory_order_acquire ) );
}

} // namespace Lumen
