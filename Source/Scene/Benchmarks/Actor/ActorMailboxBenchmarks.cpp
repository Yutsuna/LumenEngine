/**
 * @file ActorMailboxBenchmarks.cpp
 * @brief Benchmarks for the FMailBox MPSC mailbox hot paths (push, drain, contention).
 */

#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Messages/EngineMessageTypes.hpp>

#include <thread>

namespace
{

    /** @brief Producers contending on the mailbox in the MPSC macro benchmark. */
    constexpr Lumen::USize ProducerCount = 6U;

    /** @brief Messages enqueued by each producer in the MPSC macro benchmark. */
    constexpr Lumen::USize MessagesPerProducer = 4096U;

    /** @brief Upper bound of the drain batch range; covers the free-list pre-fill. */
    constexpr Lumen::USize MaxDrainBatch = 4096U;

    /** @brief Builds the representative per-frame tick message used by every benchmark here. */
    [[nodiscard]] Lumen::FMessage MakeTickMessage () noexcept
    {
        return Lumen::FMessage::Make( Lumen::EEngineMessage::Tick, Lumen::InvalidActorID,
                                      Lumen::FTickPayload{ .DeltaTime = 0.016 } );
    }

} // namespace

/**
 * Micro
 */

/**
 * Micro: one Push immediately consumed by one Pop — the steady-state
 * send/receive pair every actor exchange goes through. The node free-list
 * recycles the same node each iteration, so no allocation is measured.
 */
LUMEN_BENCHMARK( FMailBox, PushPopMessage )
{
    Lumen::FMailBox Mailbox;

    const Lumen::FMessage Message = MakeTickMessage();

    for ( auto _ : State )
    {
        Mailbox.Push( Message );

        Lumen::TOptional<Lumen::FMessage> Popped = Mailbox.Pop();

        Lumen::Benchmarks::DoNotOptimize( Popped );
    }
}

/**
 * Micro: push a batch of Argument messages, then drain it — enqueue and
 * consumer-drain cost across batch sizes. The free-list is pre-filled with
 * the largest batch, so steady-state iterations do not allocate.
 */
LUMEN_BENCHMARK_RANGE( FMailBox, DrainBatch, 1, MaxDrainBatch )
{
    const Lumen::USize BatchSize = static_cast<Lumen::USize>( State.GetArgument() );
    State.SetItemsPerIteration( BatchSize );

    Lumen::FMailBox Mailbox;
    Mailbox.Reserve( MaxDrainBatch );

    const Lumen::FMessage Message = MakeTickMessage();

    for ( auto _ : State )
    {
        for ( Lumen::USize MessageIndex = 0U; MessageIndex < BatchSize; ++MessageIndex )
        {
            Mailbox.Push( Message );
        }

        Lumen::USize PoppedCount = 0U;

        for ( Lumen::USize PoppedIndex = 0U; PoppedIndex < BatchSize; ++PoppedIndex )
        {
            Lumen::TOptional<Lumen::FMessage> Popped = Mailbox.Pop();

            if ( Popped.HasValue() )
            {
                ++PoppedCount;
            }

            Lumen::Benchmarks::DoNotOptimize( Popped );
        }

        Lumen::Benchmarks::DoNotOptimize( PoppedCount );
    }
}

/**
 * Macro
 */

/**
 * Macro: one full multi-producer exchange per sample — ProducerCount threads
 * push MessagesPerProducer messages each while the single consumer (this
 * thread) drains every message. Thread spawn/join is inside the sample on
 * purpose: the macro kind measures the whole exchange stage.
 */
LUMEN_MACRO_BENCHMARK( FMailBox, MpscContentionExchange )
{
    const Lumen::USize ExpectedCount = ProducerCount * MessagesPerProducer;
    State.SetItemsPerIteration( ExpectedCount );

    Lumen::FMailBox Mailbox;
    Mailbox.Reserve( ExpectedCount );

    const Lumen::FMessage Message = MakeTickMessage();

    for ( auto _ : State )
    {
        auto ProducerLoop = [ &Mailbox, &Message ] () noexcept -> void
        {
            for ( Lumen::USize MessageIndex = 0U; MessageIndex < MessagesPerProducer; ++MessageIndex )
            {
                Mailbox.Push( Message );
            }
        };

        std::thread ProducerThreads[ProducerCount];

        for ( std::thread &ProducerThread : ProducerThreads )
        {
            ProducerThread = std::thread( ProducerLoop );
        }

        Lumen::USize ReceivedCount = 0U;

        while ( ReceivedCount < ExpectedCount )
        {
            Lumen::TOptional<Lumen::FMessage> Popped = Mailbox.Pop();

            if ( Popped.HasValue() )
            {
                ++ReceivedCount;
                Lumen::Benchmarks::DoNotOptimize( Popped );
            }
            else
            {
                std::this_thread::yield();
            }
        }

        for ( std::thread &ProducerThread : ProducerThreads )
        {
            ProducerThread.join();
        }

        Lumen::Benchmarks::DoNotOptimize( ReceivedCount );
    }
}
