/**
 * @file ActorBenchmarks.cpp
 * @brief Benchmarks for AActor frame processing and ActorID bookkeeping.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Messages/EngineMessageTypes.hpp>

namespace
{

/** @brief Delta time used for every simulated frame. */
constexpr Lumen::Float64 FrameDeltaTime = 0.016;

/**
 * @class FBenchActor
 * @brief Minimal concrete actor: counts processed messages, no extra bookkeeping.
 */
class FBenchActor final : public Lumen::AActor
{
    LUMEN_ACTOR_BODY ( FBenchActor );

public:

    explicit FBenchActor ( const Lumen::ActorID InId ) noexcept : Lumen::AActor( InId )
    {
        /**
         *
         */
    }

    void Receive ( const Lumen::FMessage &InMessage ) noexcept override
    {
        ProcessedMessageCount += InMessage.Type;
    }

    /** @brief Exposes the protected mailbox reservation for benchmark setup. */
    void ReserveMailbox ( const Lumen::USize InCapacity ) noexcept
    {
        GetMailbox().Reserve( InCapacity );
    }

    [[nodiscard]] Lumen::UInt32 GetProcessedMessageCount () const noexcept
    {
        return ProcessedMessageCount;
    }

private:

    Lumen::UInt32 ProcessedMessageCount = 0U;
};

} // namespace

/**
 * Micro
 */

/**
 * Micro: AActor::RunFrame over a mailbox pre-loaded with Argument messages
 * (the enqueue cost is paused out). Argument 100 matches the world's
 * MaxMessagesPerTick long-tail cap; RunFrame drains every pending message
 * and runs OnTick.
 */
LUMEN_BENCHMARK_ARGS( AActor, RunFrameProcessMessages, 1, 16, 100 )
{
    const Lumen::UInt32 MessageCount = static_cast<Lumen::UInt32>( State.GetArgument() );
    State.SetItemsPerIteration( MessageCount );

    FBenchActor Actor{ Lumen::Internal::FActorRegistry::AllocateId() };
    Actor.ReserveMailbox( MessageCount );

    const Lumen::FMessage Message = Lumen::FMessage::Make( Lumen::EEngineMessage::Tick, Lumen::InvalidActorID,
                                                           Lumen::FTickPayload{ .DeltaTime = FrameDeltaTime } );

    for ( auto _ : State )
    {
        State.PauseTiming();

        for ( Lumen::UInt32 MessageIndex = 0U; MessageIndex < MessageCount; ++MessageIndex )
        {
            Actor.EnqueueMessage( Message );
        }

        State.ResumeTiming();

        Actor.RunFrame( FrameDeltaTime, 0U );
    }

    Lumen::Benchmarks::DoNotOptimize( Actor.GetProcessedMessageCount() );
}

/**
 * Micro: lock-free ActorID allocation plus release of an unbound ID — the
 * slot bookkeeping every spawn/despawn pays. Slots recycle through the free
 * list, so steady-state iterations do not allocate.
 */
LUMEN_BENCHMARK( FActorRegistry, AllocateReleaseId )
{
    for ( auto _ : State )
    {
        const Lumen::ActorID NewId = Lumen::Internal::FActorRegistry::AllocateId();
        Lumen::Benchmarks::DoNotOptimize( NewId );
        Lumen::Internal::FActorRegistry::ReleaseId( NewId );
    }
}
