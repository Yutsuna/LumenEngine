/**
 * @file RHIDeferredReleaseQueueStressTests.cpp
 * @brief Stresses concurrent retirement and polling.
 */

#include <Synchronization/RHIDeferredReleaseQueue.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDeferredReleaseQueueStress, ConcurrentProducersReleaseEveryCallbackExactlyOnce )
    {
        FRHIDeferredReleaseQueue Queue;
        TAtomic<UInt32> Released{ 0U };
        TVector<FThread> Producers;
        for ( UInt32 Producer = 0U; Producer < 4U; ++Producer )
        {
            Producers.emplace_back( [&Queue, &Released]
            {
                const auto IncrementReleased = [&Released]
                {
                    Released.fetch_add( 1U );
                };
                for ( UInt32 Index = 0U; Index < 128U; ++Index )
                {
                    const auto Enqueued = Queue.Enqueue( { 1ULL }, IncrementReleased );
                    if ( not Enqueued )
                    {
                        return;
                    }
                    Queue.Poll( { 1ULL } );
                }
            } );
        }
        for ( auto &Producer : Producers )
        {
            Producer.Join();
        }
        Queue.Drain();
        LUMEN_CHECK_EQ( Released.load(), 512U );
        LUMEN_CHECK_EQ( Queue.GetSize(), 0U );
    }

} // namespace
} // namespace Lumen
