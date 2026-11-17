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

    LUMEN_TEST( RHIDeferredReleaseQueueStress, MixedTimelinesNeverReleaseEarly )
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
                    const UInt64 Required = static_cast<UInt64>( Index % 64U ) + 1ULL;
                    if ( not Queue.Enqueue( { Required }, IncrementReleased ) )
                    {
                        return;
                    }
                }
            } );
        }
        for ( auto &Producer : Producers )
        {
            Producer.Join();
        }
        LUMEN_CHECK_EQ( Queue.Poll( {} ), 0U );
        LUMEN_CHECK_EQ( Released.load(), 0U );
        UInt32 PreviousReleased = 0U;
        for ( UInt64 Completed = 1ULL; Completed <= 64ULL; ++Completed )
        {
            Queue.Poll( { Completed } );
            const UInt32 CurrentReleased = Released.load();
            LUMEN_CHECK_TRUE( CurrentReleased >= PreviousReleased );
            LUMEN_CHECK_TRUE( CurrentReleased <= 512U );
            PreviousReleased = CurrentReleased;
        }
        LUMEN_CHECK_EQ( Released.load(), 512U );
        LUMEN_CHECK_EQ( Queue.GetSize(), 0U );
    }

} // namespace
} // namespace Lumen
