/// @file ActorDispatchBenchmarks.cpp
/// @brief End-to-end delivery benchmarks for AActor and FActorRef.
///
/// The same workload is measured through three layers:
///   - mailbox only (ActorMailbox suite),
///   - direct AActor::EnqueueMessage + ProcessMailbox (no registry lookup),
///   - FActorRef::EnqueueMessage, which adds the registry lookup and sender lifetime pin.
///
/// Comparing the three exposes the cost of the safe reference path and of processing-budget granularity.

#include <Benchmarks/Benchmarks.hpp>

#include <Actor/Actor.hpp>

#include "ActorBenchmarkHelpers.hpp"

#include <algorithm>

namespace Lumen
{

///
/// @brief One send and one receive through a raw actor pointer.
///
LUMEN_BENCHMARK( ActorDispatch, Direct_RoundTrip_NoOp )
{
    FNoOpBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FMessage Message = MakeActorBenchmarkMessage();
    for ( auto _ : State )
    {
        const EEnqueueResult Result = Actor.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        const UInt32 Processed = Actor.ProcessMailbox( 1U );
        Benchmarks::DoNotOptimize( Processed );
    }

    Actor.Shutdown();
}

///
/// @brief Same as Direct_RoundTrip_NoOp, with the handler decoding a payload.
///
LUMEN_BENCHMARK( ActorDispatch, Direct_RoundTrip_Payload )
{
    FPayloadBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FMessage Message = MakeActorBenchmarkMessage( 42ULL );
    for ( auto _ : State )
    {
        const EEnqueueResult Result = Actor.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        const UInt32 Processed = Actor.ProcessMailbox( 1U );
        Benchmarks::DoNotOptimize( Processed );
    }

    Benchmarks::DoNotOptimize( Actor.ReceivedSequenceSum );
    Actor.Shutdown();
}

///
/// @brief One send and one receive through FActorRef, including generation validation and the sender pin.
///
LUMEN_BENCHMARK( ActorDispatch, Ref_RoundTrip_NoOp )
{
    FNoOpBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FActorRef Ref    = Actor.GetRef();
    const FMessage Message = MakeActorBenchmarkMessage();

    for ( auto _ : State )
    {
        const EEnqueueResult Result = Ref.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        const UInt32 Processed = Actor.ProcessMailbox( 1U );
        Benchmarks::DoNotOptimize( Processed );
    }

    Actor.Shutdown();
}

///
/// @brief Direct delivery with a deeper processing budget.
///
/// Each iteration contributes one enqueue; every Batch iterations the mailbox is drained in one or more
/// ProcessMailbox calls. Averaged over the loop this is the per-message cost of the selected batch depth, and the
/// deeper batches amortize the exclusive-processing flag over several receives.
///
LUMEN_BENCHMARK_ARGS( ActorDispatch, Direct_Pipeline, 16, 64, 256, 1024 )
{
    const UInt32 Batch = static_cast<UInt32>( State.GetArgument() );

    FNoOpBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FMessage Message = MakeActorBenchmarkMessage();
    UInt32 Produced        = 0U;

    for ( auto _ : State )
    {
        const EEnqueueResult Result = Actor.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        ++Produced;
        if ( Produced == Batch )
        {
            const UInt32 Processed = Actor.ProcessMailbox( 0U );
            Benchmarks::DoNotOptimize( Processed );
            Produced = 0U;
        }
    }

    Actor.Shutdown();
}

///
/// @brief FActorRef delivery with a deeper processing budget.
///
/// The difference against Direct_Pipeline is the complete registry/sender-pin overhead paid on every enqueue.
///
LUMEN_BENCHMARK_ARGS( ActorDispatch, Ref_Pipeline, 16, 64, 256, 1024 )
{
    const UInt32 Batch = static_cast<UInt32>( State.GetArgument() );

    FNoOpBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FActorRef Ref    = Actor.GetRef();
    const FMessage Message = MakeActorBenchmarkMessage();
    UInt32 Produced        = 0U;

    for ( auto _ : State )
    {
        const EEnqueueResult Result = Ref.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        ++Produced;
        if ( Produced == Batch )
        {
            const UInt32 Processed = Actor.ProcessMailbox( 0U );
            Benchmarks::DoNotOptimize( Processed );
            Produced = 0U;
        }
    }

    Actor.Shutdown();
}

///
/// @brief Cost of ProcessMailbox's exclusive-processing guard versus its per-message work.
///
/// A 256-message burst is enqueued, then drained with the requested budget (0 means drain until empty). Every state
/// iteration still contributes exactly one enqueue, so the reported time remains a per-message cost. Small budgets
/// pay the processing flag once per call; an unlimited budget pays it once per burst.
///
LUMEN_BENCHMARK_ARGS( ActorDispatch, ProcessBudget, 1, 8, 64, 0 )
{
    constexpr UInt32 Batch = 256U;
    const UInt32 Budget    = static_cast<UInt32>( State.GetArgument() );

    FNoOpBenchmarkActor Actor( 0U, 0ULL, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FMessage Message = MakeActorBenchmarkMessage();
    UInt32 Produced        = 0U;

    for ( auto _ : State )
    {
        const EEnqueueResult Result = Actor.EnqueueMessage( Message );
        Benchmarks::DoNotOptimize( Result );

        ++Produced;
        if ( Produced == Batch )
        {
            if ( Budget == 0U )
            {
                const UInt32 Processed = Actor.ProcessMailbox( 0U );
                Benchmarks::DoNotOptimize( Processed );
            }
            else
            {
                UInt32 Remaining = Batch;
                while ( Remaining > 0U )
                {
                    const UInt32 Processed = Actor.ProcessMailbox( Budget );
                    Benchmarks::DoNotOptimize( Processed );

                    if ( Processed == 0U )
                    {
                        break;
                    }
                    Remaining -= std::min( Remaining, Processed );
                }
            }

            Produced = 0U;
        }
    }

    Actor.Shutdown();
}

} // namespace Lumen
