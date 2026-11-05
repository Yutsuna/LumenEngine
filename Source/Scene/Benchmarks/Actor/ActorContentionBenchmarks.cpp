/// @file ActorContentionBenchmarks.cpp
/// @brief Multi-producer scaling benchmarks for the mailbox and the complete actor pipeline.
///
/// Producer threads are created before the timed loop and kept parked between samples, so thread creation and
/// destruction are not measured. The bounded mailbox is kept at FMailBox::PoolCapacity with a fully preallocated free
/// list: producers retry on Full instead of spilling into the allocation fallback, isolating contention from allocator
/// noise. Each sample delivers a constant amount of work, making producer counts directly comparable.

#include <Benchmarks/Benchmarks.hpp>

#include <Actor/Actor.hpp>

#include "ActorBenchmarkHelpers.hpp"

#include <thread>
#include <utility>

namespace Lumen
{

namespace
{

    ///
    /// @class FProducerPool
    /// @brief Persistent producer threads driven by a generation counter.
    ///
    template <typename TSendFunction> class FProducerPool final : public FNonCopyable
    {
    public:

        FProducerPool ( const UInt32 InProducerCount, const UInt32 InMessagesPerProducer, TSendFunction InSend )
            : ProducerCount{ InProducerCount }, MessagesPerProducer{ InMessagesPerProducer }, Send{ std::move( InSend ) }
        {
            Producers.reserve( ProducerCount );
            for ( UInt32 Index = 0U; Index < ProducerCount; ++Index )
            {
                Producers.emplace_back( [this] { ProducerLoop(); } );
            }
        }

        ~FProducerPool () noexcept
        {
            bStop.store( true, std::memory_order_release );
            bStart.store( true, std::memory_order_release );
            Arrival.fetch_add( 1ULL, std::memory_order_acq_rel );
            Arrival.notify_all();

            for ( std::thread &Producer : Producers )
            {
                Producer.join();
            }
        }

    public:

        /// @brief Wakes the parked producers and waits until all of them are ready to be released.
        /// @note Call while the benchmark timer is paused; thread wake-up latency is not part of the measured path.
        void PrepareSample () noexcept
        {
            bFailed.store( false, std::memory_order_relaxed );
            Completed.store( 0U, std::memory_order_relaxed );
            Ready.store( 0U, std::memory_order_relaxed );
            bStart.store( false, std::memory_order_relaxed );

            Arrival.fetch_add( 1ULL, std::memory_order_acq_rel );
            Arrival.notify_all();
        }

        [[nodiscard]] Bool IsReady () const noexcept
        {
            return Ready.load( std::memory_order_acquire ) == ProducerCount;
        }

        /// @brief Releases the armed producers. Called just after the timer is resumed.
        void StartSample () noexcept
        {
            bStart.store( true, std::memory_order_release );
        }

        [[nodiscard]] Bool IsComplete () const noexcept
        {
            return Completed.load( std::memory_order_acquire ) == ProducerCount;
        }

        [[nodiscard]] Bool HasFailed () const noexcept
        {
            return bFailed.load( std::memory_order_acquire );
        }

        [[nodiscard]] UInt64 GetTotalMessages () const noexcept
        {
            return static_cast<UInt64>( ProducerCount ) * static_cast<UInt64>( MessagesPerProducer );
        }

    private:

        void ProducerLoop () noexcept
        {
            UInt64 SeenArrival = 0ULL;
            for ( ;; )
            {
                if ( bStop.load( std::memory_order_acquire ) )
                {
                    return;
                }

                const UInt64 CurrentArrival = Arrival.load( std::memory_order_acquire );
                if ( CurrentArrival == SeenArrival )
                {
                    Arrival.wait( SeenArrival, std::memory_order_acquire );
                    continue;
                }

                SeenArrival = CurrentArrival;
                Ready.fetch_add( 1U, std::memory_order_acq_rel );

                while ( not bStart.load( std::memory_order_acquire ) )
                {
                    if ( bStop.load( std::memory_order_acquire ) )
                    {
                        return;
                    }
                    std::this_thread::yield();
                }

                for ( UInt32 Index = 0U; Index < MessagesPerProducer; ++Index )
                {
                    if ( Send() != EEnqueueResult::Success )
                    {
                        bFailed.store( true, std::memory_order_release );
                        break;
                    }
                }

                Completed.fetch_add( 1U, std::memory_order_acq_rel );
            }
        }

    private:

        UInt32 ProducerCount;
        UInt32 MessagesPerProducer;
        TSendFunction Send;

        TAtomic<UInt64> Arrival{ 0ULL };
        TAtomic<UInt32> Ready{ 0U };
        TAtomic<UInt32> Completed{ 0U };
        TAtomic<Bool> bStart{ false };
        TAtomic<Bool> bStop{ false };
        TAtomic<Bool> bFailed{ false };

        TVector<std::thread> Producers;
    };

    constexpr UInt32 ContendedMessagesPerSample = 256U;
    constexpr UInt32 PipelineMessagesPerSample  = 1024U;

} // namespace

///
/// @brief Mailbox publication scaling with 1, 2, 4 and 8 concurrent producers.
///
/// Every sample publishes a constant 256 messages in total, spread across the requested producer count. Draining is
/// paused, so the timer reports only the MPSC publication path under contention.
///
LUMEN_BENCHMARK_ARGS( ActorContention, MailboxPush, 1, 2, 4, 8 )
{
    const UInt32 ProducerCount = static_cast<UInt32>( State.GetArgument() );

    FMailBox Mailbox( FMailBox::PoolCapacity, FMailBox::PoolCapacity );
    const FMessage Message = MakeActorBenchmarkMessage();

    {
        FProducerPool Pool( ProducerCount, ContendedMessagesPerSample / ProducerCount,
                            [&Mailbox, &Message] { return Mailbox.Push( Message ); } );

        State.SetItemsPerIteration( Pool.GetTotalMessages() );

        for ( auto _ : State )
        {
            State.PauseTiming();
            Pool.PrepareSample();
            while ( not Pool.IsReady() )
            {
                std::this_thread::yield();
            }

            State.ResumeTiming();
            Pool.StartSample();

            while ( not Pool.IsComplete() )
            {
                std::this_thread::yield();
            }

            State.PauseTiming();
            DrainMailbox( Mailbox );
        }
    }

    Mailbox.Close();
}

///
/// @brief Complete direct actor pipeline scaling (enqueue, dispatch, receive).
///
/// Producers target the actor's bounded, preallocated mailbox and retry on Full. The benchmark thread is the exclusive
/// consumer, matching the actor model's single-consumer guarantee. The constant 1024-message sample includes both
/// producer backpressure and Receive dispatch.
///
LUMEN_BENCHMARK_ARGS( ActorContention, Pipeline_Direct, 1, 2, 4, 8 )
{
    const UInt32 ProducerCount = static_cast<UInt32>( State.GetArgument() );

    FNoOpBenchmarkActor Actor( 0U, FMailBox::PoolCapacity, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FMessage Message = MakeActorBenchmarkMessage();

    {
        FProducerPool Pool( ProducerCount, PipelineMessagesPerSample / ProducerCount,
                            [&Actor, &Message]
                            {
                                for ( ;; )
                                {
                                    const EEnqueueResult Result = Actor.EnqueueMessage( Message );
                                    if ( Result != EEnqueueResult::Full )
                                    {
                                        return Result;
                                    }
                                    std::this_thread::yield();
                                }
                            } );

        State.SetItemsPerIteration( Pool.GetTotalMessages() );

        for ( auto _ : State )
        {
            State.PauseTiming();
            Pool.PrepareSample();
            while ( not Pool.IsReady() )
            {
                std::this_thread::yield();
            }

            State.ResumeTiming();
            Pool.StartSample();

            while ( not Pool.IsComplete() )
            {
                const UInt32 Processed = Actor.ProcessMailbox( 0U );
                Benchmarks::DoNotOptimize( Processed );
            }

            for ( ;; )
            {
                const UInt32 Processed = Actor.ProcessMailbox( 0U );
                Benchmarks::DoNotOptimize( Processed );
                if ( Processed == 0U )
                {
                    break;
                }
            }

            State.PauseTiming();
        }
    }

    Actor.Shutdown();
}

///
/// @brief Complete FActorRef pipeline scaling, including registry lookup and the sender lifetime pin.
///
/// Comparing this against Pipeline_Direct quantifies the safety cost of reference-based delivery while the consumer
/// remains the same single actor thread.
///
LUMEN_BENCHMARK_ARGS( ActorContention, Pipeline_Ref, 1, 2, 4, 8 )
{
    const UInt32 ProducerCount = static_cast<UInt32>( State.GetArgument() );

    FNoOpBenchmarkActor Actor( 0U, FMailBox::PoolCapacity, FMailBox::PoolCapacity );
    const EActorRegistrationResult Registration = Actor.Initialize();
    Benchmarks::DoNotOptimize( Registration );

    const FActorRef Ref    = Actor.GetRef();
    const FMessage Message = MakeActorBenchmarkMessage();

    {
        FProducerPool Pool( ProducerCount, PipelineMessagesPerSample / ProducerCount,
                            [&Ref, &Message]
                            {
                                for ( ;; )
                                {
                                    const EEnqueueResult Result = Ref.EnqueueMessage( Message );
                                    if ( Result != EEnqueueResult::Full )
                                    {
                                        return Result;
                                    }
                                    std::this_thread::yield();
                                }
                            } );

        State.SetItemsPerIteration( Pool.GetTotalMessages() );

        for ( auto _ : State )
        {
            State.PauseTiming();
            Pool.PrepareSample();
            while ( not Pool.IsReady() )
            {
                std::this_thread::yield();
            }

            State.ResumeTiming();
            Pool.StartSample();

            while ( not Pool.IsComplete() )
            {
                const UInt32 Processed = Actor.ProcessMailbox( 0U );
                Benchmarks::DoNotOptimize( Processed );
            }

            for ( ;; )
            {
                const UInt32 Processed = Actor.ProcessMailbox( 0U );
                Benchmarks::DoNotOptimize( Processed );
                if ( Processed == 0U )
                {
                    break;
                }
            }

            State.PauseTiming();
        }
    }

    Actor.Shutdown();
}

} // namespace Lumen
