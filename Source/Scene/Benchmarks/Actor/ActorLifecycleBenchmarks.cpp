/**
 * @file ActorLifecycleBenchmarks.cpp
 * @brief Benchmarks for lifecycle churn, deferred commands and dead letters.
 */

#include <Actor/ActorLifecycle.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Templates/Vector.hpp>
#include <World/World.hpp>

namespace Lumen
{

namespace
{

    constexpr Float64 FrameDeltaTime  = 0.016;
    constexpr USize LiveActorCount    = 1000ULL;
    constexpr USize RetiredActorCount = 100ULL;
    constexpr USize MaxLifecycleChurn = 1024ULL;

    /** @class FBenchLifecycleActor
     * @brief Minimal actor with lifecycle hooks, used for churn measurements.
     */
    class FBenchLifecycleActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FBenchLifecycleActor );

    public:

        explicit FBenchLifecycleActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnBeginPlay () noexcept override
        {
            ++BeginPlayCount;
        }

        void OnEndPlay () noexcept override
        {
            ++EndPlayCount;
        }

        [[nodiscard]] UInt32 GetBeginPlayCount () const noexcept
        {
            return BeginPlayCount;
        }

        [[nodiscard]] UInt32 GetEndPlayCount () const noexcept
        {
            return EndPlayCount;
        }

    private:

        UInt32 BeginPlayCount = 0U;
        UInt32 EndPlayCount   = 0U;
    };

} // namespace

/**
 * paired TrySpawnActor / TryDestroyActor cycles. The world is reserved
 * once and slots are recycled in place, so the measured loop does not grow
 * containers or allocate deferred commands.
 */
LUMEN_BENCHMARK_RANGE( FWorld, SpawnDestroyChurn, 1, MaxLifecycleChurn )
{
    const USize CycleCount = static_cast<USize>( State.GetArgument() );
    FWorld World;

    State.SetItemsPerIteration( CycleCount * 2ULL );
    for ( auto _ : State )
    {
        for ( USize CycleIndex = 0ULL; CycleIndex < CycleCount; ++CycleIndex )
        {
            const TExpected<TSharedRef<FBenchLifecycleActor>, EActorSpawnError> Spawned =
                World.TrySpawnActor<FBenchLifecycleActor>();
            Benchmarks::DoNotOptimize( Spawned );

            if ( Spawned )
            {
                const TExpected<void, EActorDestroyError> Destroyed = World.TryDestroyActor( ( *Spawned )->GetId() );
                Benchmarks::DoNotOptimize( Destroyed );
            }
        }
    }
}

/**
 * applying a batch of deferred spawn commands in the serial commit.
 * World construction and command enqueueing are paused out; only the tick's
 * deferred-command sweep and hook dispatch are measured.
 */
LUMEN_BENCHMARK_ARGS( FWorld, DeferredCommandApply, 1, 16, 128 )
{
    const USize BatchSize = static_cast<USize>( State.GetArgument() );

    FWorldDesc Desc;
    Desc.WorkerCount             = 1U;
    Desc.MaxActors               = BatchSize + 8ULL;
    Desc.DeferredCommandCapacity = BatchSize;

    State.SetItemsPerIteration( BatchSize );
    for ( auto _ : State )
    {
        State.PauseTiming();

        FWorld World( Desc );
        for ( USize CommandIndex = 0ULL; CommandIndex < BatchSize; ++CommandIndex )
        {
            const TExpected<FActorRef, EActorSpawnError> Deferred = World.TrySpawnActorDeferred<FBenchLifecycleActor>();
            Benchmarks::DoNotOptimize( Deferred );
        }

        State.ResumeTiming();

        const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( 0.0 );
        Benchmarks::DoNotOptimize( Result );

        State.PauseTiming();
    }
}

/**
 * one frame that sends to 100 stale IDs (each producing a dead letter)
 * and then broadcasts to 1000 live actors. The diagnostic ring stays bounded
 * at 256 records, so the hot path never grows storage.
 */
LUMEN_MACRO_BENCHMARK( FWorld, BroadcastWithDeadLetters )
{
    FWorld World;

    TVector<ActorID> RetiredIds;
    RetiredIds.reserve( RetiredActorCount );

    for ( USize ActorIndex = 0ULL; ActorIndex < LiveActorCount; ++ActorIndex )
    {
        TSharedRef<FBenchLifecycleActor> Spawned = World.SpawnActor<FBenchLifecycleActor>();
        Benchmarks::DoNotOptimize( Spawned );

        if ( ActorIndex < RetiredActorCount )
        {
            const ActorID RetiredId = Spawned->GetId();
            static_cast<void>( World.TryDestroyActor( RetiredId ) );
            RetiredIds.push_back( RetiredId );
        }
    }

    const FMessage Message = FMessage::Make( 88U, InvalidActorID );
    static_cast<void>( World.TryTick( FrameDeltaTime ) );

    State.SetItemsPerIteration( LiveActorCount + RetiredActorCount );
    for ( auto _ : State )
    {
        for ( const ActorID RetiredId : RetiredIds )
        {
            const TExpected<void, EActorSendError> DeadLettered = FActorRef( RetiredId ).TryEnqueueMessage( Message );
            Benchmarks::DoNotOptimize( DeadLettered );
        }

        const TExpected<void, EActorSendError> Broadcasted = World.TryBroadcast( Message );
        Benchmarks::DoNotOptimize( Broadcasted );

        const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( FrameDeltaTime );
        Benchmarks::DoNotOptimize( Result );
    }
}

} // namespace Lumen
