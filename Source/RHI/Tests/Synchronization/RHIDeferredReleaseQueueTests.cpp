/**
 * @file RHIDeferredReleaseQueueTests.cpp
 * @brief Tests partial completion, FIFO drain, and shutdown.
 */

#include <Synchronization/RHIDeferredReleaseQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDeferredReleaseQueue, PollReleasesOnlyCompletedEntries )
    {
        FRHIDeferredReleaseQueue Queue;
        UInt32 First  = 0U;
        UInt32 Second = 0U;

        const auto IncrementFirst = [&First]
        {
            ++First;
        };
        const auto IncrementSecond = [&Second]
        {
            ++Second;
        };

        LUMEN_CHECK_TRUE( Queue.Enqueue( { 2ULL }, IncrementFirst ).HasValue() );
        LUMEN_CHECK_TRUE( Queue.Enqueue( { 4ULL }, IncrementSecond ).HasValue() );

        LUMEN_CHECK_EQ( Queue.Poll( { 1ULL } ), 0U );
        LUMEN_CHECK_EQ( Queue.Poll( { 2ULL } ), 1U );
        LUMEN_CHECK_EQ( First, 1U );
        LUMEN_CHECK_EQ( Second, 0U );
        LUMEN_CHECK_EQ( Queue.GetSize(), 1U );

        LUMEN_CHECK_EQ( Queue.Drain(), 1U );
        LUMEN_CHECK_EQ( Second, 1U );
        LUMEN_CHECK_EQ( Queue.GetSize(), 0U );
    }

    LUMEN_TEST( RHIDeferredReleaseQueue, ShutdownDrainsAndRejectsNewEntries )
    {
        FRHIDeferredReleaseQueue Queue;
        UInt32 Calls = 0U;

        const auto IncrementCalls = [&Calls]
        {
            ++Calls;
        };

        LUMEN_CHECK_HAS_ERROR( Queue.Enqueue( {}, {} ) );

        LUMEN_CHECK_TRUE( Queue.Enqueue( { 99ULL }, IncrementCalls ).HasValue() );
        Queue.Shutdown();
        LUMEN_CHECK_EQ( Calls, 1U );
        LUMEN_CHECK_TRUE( Queue.IsShutdown() );

        LUMEN_CHECK_HAS_ERROR( Queue.Enqueue( {}, IncrementCalls ) );
        LUMEN_CHECK_EQ( Calls, 1U );
    }

} // namespace
} // namespace Lumen
