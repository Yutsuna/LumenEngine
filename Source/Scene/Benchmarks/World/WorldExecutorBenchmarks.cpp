/**
 * @file WorldExecutorBenchmarks.cpp
 * @brief Benchmarks message fan-out and bounded outboxes.
 */

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorEmitContext.hpp>
#include <Actor/ActorLifecycle.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

namespace
{

    constexpr Float64 FrameDeltaTime      = 0.016;
    constexpr USize PhasedActorCount      = 1024ULL;
    constexpr USize FanOutActorCount      = 1000ULL;
    constexpr UInt32 MessagesPerActorTick = FWorld::MaxMessagesPerTick / 16U;

    /** @class FBenchPhaseActor
     * @brief Counts one dispatched task in a selected phase.
     */
    class FBenchPhaseActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FBenchPhaseActor );

    public:

        FBenchPhaseActor ( const ActorID InId, const EActorTickPhase InPhase, TAtomic<UInt64> *InCounter ) noexcept
            : AActor( InId ), Phase( InPhase ), Counter( InCounter )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnFixedTick ( const Float64 ) noexcept override
        {
            Counter->fetch_add( 1ULL, std::memory_order_relaxed );
        }

        void OnTick ( const Float64 ) noexcept override
        {
            Counter->fetch_add( 1ULL, std::memory_order_relaxed );
        }

        [[nodiscard]] EActorTickPhase GetTickPhase () const noexcept override
        {
            return Phase;
        }

    private:

        EActorTickPhase Phase    = EActorTickPhase::Update;
        TAtomic<UInt64> *Counter = nullptr;
    };

    /** @class FBenchSinkActor
     * @brief Counts received messages without producing emissions.
     */
    class FBenchSinkActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FBenchSinkActor );

    public:

        explicit FBenchSinkActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            ProcessedTypes += InMessage.Type;
        }

        [[nodiscard]] UInt32 GetProcessedTypes () const noexcept
        {
            return ProcessedTypes;
        }

    private:

        UInt32 ProcessedTypes = 0U;
    };

    /** @class FBenchEmitterActor
     * @brief Fills its bounded outbox every task without growing storage.
     */
    class FBenchEmitterActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FBenchEmitterActor );

    public:

        FBenchEmitterActor ( const ActorID InId, const ActorID InTarget ) noexcept : AActor( InId ), Target( InTarget )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            const FMessage Message = FMessage::Make( 86U, GetId() );
            for ( UInt32 Index = 0U; Index < MessagesPerActorTick + 1U; ++Index )
            {
                TExpected<void, EActorSendError> Result = TrySendMessage( Target, Message );
                if ( not Result )
                {
                    RejectedCount.fetch_add( 1U, std::memory_order_relaxed );
                }
            }
        }

        [[nodiscard]] UInt64 GetRejectedCount () const noexcept
        {
            return RejectedCount.load( std::memory_order_relaxed );
        }

    private:

        ActorID Target                = InvalidActorID;
        TAtomic<UInt64> RejectedCount = { 0ULL };
    };

    /** @brief Populates one actor per phase, cycling deterministically. */
    void PopulatePhasedActors ( FWorld &InWorld, TAtomic<UInt64> *InCounter )
    {
        constexpr EActorTickPhase Phases[] = {
            EActorTickPhase::FixedUpdate,
            EActorTickPhase::PreUpdate,
            EActorTickPhase::Update,
            EActorTickPhase::PostUpdate,
        };

        for ( USize ActorIndex = 0ULL; ActorIndex < PhasedActorCount; ++ActorIndex )
        {
            const EActorTickPhase Phase = Phases[ActorIndex % 4ULL];
            TSharedRef<AActor> Spawned  = InWorld.SpawnActor<FBenchPhaseActor>( Phase, InCounter );
            Benchmarks::DoNotOptimize( Spawned );
        }
    }

} // namespace

/**
 * one multi-phase tick (fixed + Pre/Update/Post) at 1, 2 and 4 workers.
 * The world is built once outside the measured loop; reservations happen at
 * construction, so the tick itself does not allocate deferred storage.
 */
LUMEN_BENCHMARK_ARGS( FWorldExecutor, MultiPhaseTick, 1, 2, 4 )
{
    FWorldDesc Desc;
    Desc.WorkerCount    = static_cast<UInt32>( State.GetArgument() );
    Desc.MaxActors      = PhasedActorCount;
    Desc.FixedDeltaTime = FrameDeltaTime;

    FWorld World( Desc );
    TAtomic<UInt64> Counter = { 0ULL };
    PopulatePhasedActors( World, &Counter );

    /** INFO: Untimed warm-up — the executor threads start on the first phase. */
    static_cast<void>( World.TryTick( FrameDeltaTime ) );

    State.SetItemsPerIteration( PhasedActorCount );
    for ( auto _ : State )
    {
        const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( FrameDeltaTime );
        Benchmarks::DoNotOptimize( Result );
    }

    Benchmarks::DoNotOptimize( Counter );
}

/**
 * broadcast fan-out: one message routed to 1000 actors, then dispatched
 * and merged. Repeated at 1, 2 and 4 workers.
 */
LUMEN_BENCHMARK_ARGS( FWorldExecutor, MessageFanOut, 1, 2, 4 )
{
    FWorldDesc Desc;
    Desc.WorkerCount = static_cast<UInt32>( State.GetArgument() );
    Desc.MaxActors   = FanOutActorCount + 1ULL;

    FWorld World( Desc );
    for ( USize ActorIndex = 0ULL; ActorIndex < FanOutActorCount; ++ActorIndex )
    {
        TSharedRef<FBenchSinkActor> Sink = World.SpawnActor<FBenchSinkActor>();
        Benchmarks::DoNotOptimize( Sink );
    }

    const FMessage Message = FMessage::Make( 87U, InvalidActorID );
    static_cast<void>( World.TryTick( FrameDeltaTime ) );

    State.SetItemsPerIteration( FanOutActorCount );
    for ( auto _ : State )
    {
        const TExpected<void, EActorSendError> Routed = World.TryBroadcast( Message );
        Benchmarks::DoNotOptimize( Routed );

        const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( FrameDeltaTime );
        Benchmarks::DoNotOptimize( Result );
    }
}

/**
 * a tick whose emitter keeps its reserved outbox at capacity. Every
 * extra emission is rejected structurally, so no storage grows in the loop.
 */
LUMEN_BENCHMARK_ARGS( FWorldExecutor, SaturatedOutbox, 1, 2, 4 )
{
    FWorldDesc Desc;
    Desc.WorkerCount         = static_cast<UInt32>( State.GetArgument() );
    Desc.ActorOutboxCapacity = MessagesPerActorTick;
    Desc.EmissionCapacity    = MessagesPerActorTick;

    FWorld World( Desc );
    auto Sink    = World.SpawnActor<FBenchSinkActor>();
    auto Emitter = World.SpawnActor<FBenchEmitterActor>( Sink->GetId() );

    static_cast<void>( World.TryTick( FrameDeltaTime ) );
    State.SetItemsPerIteration( MessagesPerActorTick );

    for ( auto _ : State )
    {
        const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( FrameDeltaTime );
        Benchmarks::DoNotOptimize( Result );
    }

    Benchmarks::DoNotOptimize( Emitter->GetRejectedCount() );
}

} // namespace Lumen
