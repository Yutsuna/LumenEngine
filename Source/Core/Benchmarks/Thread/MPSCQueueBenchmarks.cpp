/**
 * @file MPSCQueueBenchmarks.cpp
 * @brief Benchmarks for Lumen::Thread::TMPSCQueue with preserved storage.
 */

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Array.hpp>

#include <Thread/MPSCQueue.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    struct FBenchmarkMessage
    {
        UInt32 ProducerIndex;
        UInt32 Sequence;
    };

} // namespace

LUMEN_BENCHMARK( TMPSCQueue, PushPop )
{
    Thread::TMPSCQueue<UInt64> Queue;
    const auto Initialized = Queue.Initialize( 2ULL );

    if ( not Initialized.HasValue() ) [[unlikely]]
    {
        return;
    }

    State.SetItemsPerIteration( 2ULL );

    UInt64 Value       = 0ULL;
    UInt64 Accumulator = 0ULL;

    for ( auto _ : State )
    {
        const auto PushResult        = Queue.Push( Value );
        const TOptional<UInt64> Item = Queue.Pop();
        Accumulator += Item.ValueOr( 0ULL );
        Benchmarks::DoNotOptimize( PushResult );
        ++Value;
    }

    Benchmarks::DoNotOptimize( Accumulator );
}

LUMEN_BENCHMARK( TMPSCQueue, RejectWhenFull )
{
    Thread::TMPSCQueue<UInt64> Queue;
    const auto Initialized = Queue.Initialize( 1ULL );

    if ( not Initialized.HasValue() ) [[unlikely]]
    {
        return;
    }

    const auto InitialPush = Queue.Push( 1ULL );

    if ( not InitialPush.HasValue() ) [[unlikely]]
    {
        return;
    }

    State.SetItemsPerIteration( 1ULL );
    UInt64 Value = 2ULL;

    for ( auto _ : State )
    {
        const auto PushResult = Queue.Push( Value );
        Benchmarks::DoNotOptimize( PushResult );
        ++Value;
    }
}

LUMEN_BENCHMARK_ARGS( TMPSCQueue, FullBurstPushDrain, 8, 64, 512 )
{
    const USize Capacity = static_cast<USize>( State.GetArgument() );
    Thread::TMPSCQueue<UInt64> Queue;
    const auto Initialized = Queue.Initialize( Capacity );

    if ( not Initialized.HasValue() ) [[unlikely]]
    {
        return;
    }

    State.SetItemsPerIteration( static_cast<UInt64>( Capacity ) * 2ULL );

    UInt64 Value       = 0ULL;
    UInt64 Accumulator = 0ULL;
    USize FailureCount = 0ULL;

    for ( auto _ : State )
    {
        for ( USize Index = 0ULL; Index < Capacity; ++Index )
        {
            if ( not Queue.Push( Value + static_cast<UInt64>( Index ) ).HasValue() ) [[unlikely]]
            {
                ++FailureCount;
            }
        }

        for ( USize Index = 0ULL; Index < Capacity; ++Index )
        {
            const TOptional<UInt64> Item = Queue.Pop();

            if ( Item.HasValue() )
            {
                Accumulator += *Item;
            }
            else [[unlikely]]
            {
                ++FailureCount;
            }
        }

        Value += static_cast<UInt64>( Capacity );
    }

    Benchmarks::DoNotOptimize( Accumulator );
    Benchmarks::DoNotOptimize( FailureCount );
}

LUMEN_BENCHMARK( TMPSCQueue, ConcurrentPushPop )
{
    constexpr UInt32 ProducerCount       = 4U;
    constexpr UInt32 MessagesPerProducer = 250U;
    constexpr UInt32 TotalMessageCount   = ProducerCount * MessagesPerProducer;

    Thread::TMPSCQueue<FBenchmarkMessage> Queue;
    const auto Initialized = Queue.Initialize( 64ULL );

    if ( not Initialized.HasValue() ) [[unlikely]]
    {
        return;
    }

    State.SetItemsPerIteration( TotalMessageCount );

    TAtomic<UInt64> PublishedGeneration{ 0ULL };
    TAtomic<UInt32> CompletedProducers{ 0U };
    TAtomic<Bool> bStopProducers{ false };

    auto Producer = [&Queue, &PublishedGeneration, &CompletedProducers, &bStopProducers] ( const UInt32 InProducerIndex ) noexcept
    {
        UInt64 ObservedGeneration = 0ULL;

        while ( not bStopProducers.load( std::memory_order_acquire ) )
        {
            const UInt64 Generation = PublishedGeneration.load( std::memory_order_acquire );

            if ( Generation == ObservedGeneration )
            {
                FThread::Yield();
                continue;
            }

            for ( UInt32 Sequence = 0U; Sequence < MessagesPerProducer; ++Sequence )
            {
                while ( not Queue.Push( FBenchmarkMessage{ InProducerIndex, Sequence } ).HasValue() )
                {
                    FThread::Yield();
                }
            }

            ObservedGeneration = Generation;
            CompletedProducers.fetch_add( 1U, std::memory_order_release );
        }
    };

    TArray<FThread, ProducerCount> ProducerThreads = {
        FThread( Producer, 0U ),
        FThread( Producer, 1U ),
        FThread( Producer, 2U ),
        FThread( Producer, 3U ),
    };

    UInt64 Accumulator = 0ULL;

    for ( auto _ : State )
    {
        CompletedProducers.store( 0U, std::memory_order_relaxed );
        PublishedGeneration.fetch_add( 1ULL, std::memory_order_release );

        UInt32 ReceivedMessageCount = 0U;

        while ( ReceivedMessageCount < TotalMessageCount )
        {
            const TOptional<FBenchmarkMessage> Item = Queue.Pop();

            if ( Item.HasValue() )
            {
                Accumulator += Item->Sequence;
                ++ReceivedMessageCount;
            }
            else
            {
                FThread::Yield();
            }
        }

        while ( CompletedProducers.load( std::memory_order_acquire ) != ProducerCount )
        {
            FThread::Yield();
        }
    }

    bStopProducers.store( true, std::memory_order_release );

    for ( FThread &ProducerThread : ProducerThreads )
    {
        ProducerThread.Join();
    }

    Benchmarks::DoNotOptimize( Accumulator );
}

} // namespace Lumen
