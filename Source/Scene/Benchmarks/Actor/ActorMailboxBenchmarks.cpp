/// @file ActorMailboxBenchmarks.cpp
/// @brief Raw transport benchmarks for the actor MPSC mailbox.
///
/// This suite measures the layer below AActor: node publication, message transfer, and node recycling. Batch variants
/// expose the difference between the 256-node pool steady state and the allocation fallback used for deeper mailboxes.

#include <Benchmarks/Benchmarks.hpp>

#include <Actor/ActorMailbox.hpp>

#include "ActorBenchmarkHelpers.hpp"

#include <algorithm>

namespace Lumen
{

///
/// @brief Steady-state round trip with a warm, preallocated node pool.
///
/// One iteration sends and receives exactly one 128-byte message, so the reported time is the full enqueue/dequeue
/// latency of the mailbox without any actor or registry logic.
///
LUMEN_BENCHMARK( ActorMailbox, PushPop_SteadyState )
{
    FMailBox Mailbox( 0ULL, FMailBox::PoolCapacity );
    const FMessage Message = MakeActorBenchmarkMessage();

    for ( auto _ : State )
    {
        const EEnqueueResult Result = Mailbox.Push( Message );
        Benchmarks::DoNotOptimize( Result );

        const TOptional<FMessage> Popped = Mailbox.Pop();
        Benchmarks::DoNotOptimize( Popped );
    }

    Mailbox.Close();
}

///
/// @brief Enqueue throughput for bounded batches of varying depth.
///
/// Each state iteration is one Push. The mailbox is drained inside a paused region, so the timer only covers node
/// publication. Batches above FMailBox::PoolCapacity exercise the allocation fallback because the free list cannot
/// retain the whole batch, which is the behaviour an unbounded production mailbox eventually hits under a burst.
///
LUMEN_BENCHMARK_ARGS( ActorMailbox, Push_Block, 16, 64, 256, 1024, 4096 )
{
    const USize Batch        = static_cast<USize>( State.GetArgument() );
    const USize Preallocated = std::min( Batch, FMailBox::PoolCapacity );

    FMailBox Mailbox( Batch, Preallocated );
    const FMessage Message = MakeActorBenchmarkMessage();

    USize InBatch = 0ULL;
    for ( auto _ : State )
    {
        const EEnqueueResult Result = Mailbox.Push( Message );
        Benchmarks::DoNotOptimize( Result );

        ++InBatch;
        if ( InBatch == Batch )
        {
            State.PauseTiming();
            DrainMailbox( Mailbox );
            InBatch = 0ULL;
            State.ResumeTiming();
        }
    }

    Mailbox.Close();
}

///
/// @brief Dequeue throughput for bounded batches of varying depth.
///
/// Each state iteration is one Pop. Refilling the mailbox is paused, so the timer isolates the consumer path. With a
/// batch deeper than the pool, Pop also has to release nodes that no longer fit in the free list.
///
LUMEN_BENCHMARK_ARGS( ActorMailbox, Pop_Block, 16, 64, 256, 1024, 4096 )
{
    const USize Batch        = static_cast<USize>( State.GetArgument() );
    const USize Preallocated = std::min( Batch, FMailBox::PoolCapacity );

    FMailBox Mailbox( 0ULL, Preallocated );
    const FMessage Message = MakeActorBenchmarkMessage();

    for ( USize Index = 0ULL; Index < Batch; ++Index )
    {
        const EEnqueueResult Result = Mailbox.Push( Message );
        Benchmarks::DoNotOptimize( Result );
    }

    USize Remaining = Batch;
    for ( auto _ : State )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();
        Benchmarks::DoNotOptimize( Popped );

        --Remaining;
        if ( Remaining == 0ULL )
        {
            State.PauseTiming();
            for ( USize Index = 0ULL; Index < Batch; ++Index )
            {
                const EEnqueueResult Result = Mailbox.Push( Message );
                Benchmarks::DoNotOptimize( Result );
            }
            Remaining = Batch;
            State.ResumeTiming();
        }
    }

    Mailbox.Close();
}

///
/// @brief Cost of polling an empty mailbox, the common consumer back-off path.
///
LUMEN_BENCHMARK( ActorMailbox, EmptyPop )
{
    FMailBox Mailbox( 0ULL, FMailBox::PoolCapacity );

    for ( auto _ : State )
    {
        const TOptional<FMessage> Popped = Mailbox.Pop();
        Benchmarks::DoNotOptimize( Popped );
    }

    Mailbox.Close();
}

///
/// @brief Cost of the lock-free visibility check used before polling.
///
LUMEN_BENCHMARK( ActorMailbox, EmptyCheck )
{
    FMailBox Mailbox( 0ULL, FMailBox::PoolCapacity );

    for ( auto _ : State )
    {
        const Bool bIsEmpty = Mailbox.IsEmpty();
        Benchmarks::DoNotOptimize( bIsEmpty );
    }

    Mailbox.Close();
}

} // namespace Lumen
