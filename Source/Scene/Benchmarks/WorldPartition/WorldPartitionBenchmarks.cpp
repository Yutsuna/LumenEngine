/**
 * @file WorldPartitionBenchmarks.cpp
 * @brief Cell churn, bounded extraction and read-only partition queries at 100/1k/10k actors.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>
#include <World/WorldTypes.hpp>
#include <WorldPartition/WorldPartition.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace Lumen
{

namespace
{

    constexpr USize BenchmarkCells   = 16ULL;
    constexpr USize BenchmarkJournal = 262144ULL;

    [[nodiscard]] FWorldPartitionConfig MakePartitionConfig ( const USize InCount ) noexcept
    {
        return FWorldPartitionConfig{
            .MaxCells = BenchmarkCells, .MaxAssignedActors = InCount, .MaxJournalRecords = BenchmarkJournal };
    }

    [[nodiscard]] FWorldDesc MakeWorldDesc ( const USize InCount ) noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount      = 1U;
        Desc.MaxActors        = InCount;
        Desc.WorldId          = 99020ULL;
        Desc.PartitionConfig  = MakePartitionConfig( InCount );
        Desc.RenderCapacities = FRenderSnapshotCapacities{
            .TransformCapacity = InCount,
            .MeshCapacity      = 0ULL,
            .CameraCapacity    = 0ULL,
            .LightCapacity     = 0ULL,
        };
        return Desc;
    }

    [[nodiscard]] ActorID BenchmarkActor ( const USize InIndex ) noexcept
    {
        return MakeActorId( static_cast<UInt32>( InIndex ) + 1U, 9U );
    }

    [[nodiscard]] FWorldCellId BenchmarkCell ( const USize InIndex ) noexcept
    {
        return FWorldCellId{ ( InIndex % BenchmarkCells ) + 1ULL };
    }

    class FPartitionBenchmarkActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FPartitionBenchmarkActor );

    public:

        explicit FPartitionBenchmarkActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            static_cast<void>( InMessage );
        }
    };

    void RegisterActors ( FWorldPartition &InPartition, const USize InCount ) noexcept
    {
        for ( USize Index = 0ULL; Index < InCount; ++Index )
        {
            static_cast<void>( InPartition.TryAssignActor( BenchmarkActor( Index ), BenchmarkCell( Index ),
                                                           FPersistentActorId{ Index + 1ULL } ) );
        }
        for ( USize CellIndex = 0ULL; CellIndex < BenchmarkCells; ++CellIndex )
        {
            static_cast<void>( InPartition.TryLoadCell( FWorldCellId{ CellIndex + 1ULL }, 0ULL, 0ULL ) );
        }
    }

} // namespace

/** @brief Bounded load/unload churn on one cell per iteration; journal is pre-reserved. */
LUMEN_BENCHMARK_ARGS( FWorldPartitionBench, CellChurn, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FWorldPartition Partition;
    static_cast<void>( Partition.Initialize( MakePartitionConfig( Count ) ) );
    RegisterActors( Partition, Count );

    State.SetItemsPerIteration( 2ULL );
    State.SetBytesPerIteration( 2ULL * sizeof( FPartitionJournalRecord ) );
    UInt64 Sequence = 0ULL;

    for ( auto _ : State )
    {
        if ( Sequence % 4096ULL == 0ULL and Partition.GetJournal().size() > ( BenchmarkJournal / 2ULL ) )
        {
            /** INFO: Keep the bounded journal from saturating; rebuilding reserved state is outside the clock. */
            State.PauseTiming();
            Partition.Clear();
            RegisterActors( Partition, Count );
            State.ResumeTiming();
        }
        const FWorldCellId Cell = BenchmarkCell( Sequence++ );
        static_cast<void>( Partition.TryUnloadCell( Cell, Sequence, 0ULL ) );
        static_cast<void>( Partition.TryLoadCell( Cell, Sequence, 1ULL ) );
        Benchmarks::DoNotOptimize( Partition.GetStats() );
    }
}

/** @brief Extraction cost with a fixed subset of cells dormant; no mutation in the timed loop. */
LUMEN_BENCHMARK_ARGS( FWorldPartitionBench, IncrementalExtraction, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FWorld World( MakeWorldDesc( Count ) );
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        auto Spawned = World.TrySpawnActor<FPartitionBenchmarkActor>();
        if ( not Spawned )
        {
            continue;
        }
        const ActorID Id        = ( *Spawned )->GetId();
        const FWorldCellId Cell = BenchmarkCell( Index );
        static_cast<void>( World.GetSpatialRegistry().TryRegisterSpatialEntity( Id ) );
        static_cast<void>( World.TryAssignActorToCell( Id, Cell, FPersistentActorId{ Index + 1ULL } ) );
        /** INFO: Load a fixed half of the cells, not a range of actor indices that wraps all of them. */
        if ( Cell.GetValue() <= ( BenchmarkCells / 2ULL ) )
        {
            static_cast<void>( World.TryPartitionLoadCell( Cell ) );
        }
    }

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<UInt64>( Count ) * sizeof( FRenderTransformRecord ) );

    for ( auto _ : State )
    {
        const auto Result = World.TryTick( 0.016 );
        Benchmarks::DoNotOptimize( Result );
    }
}

/** @brief Read-only membership queries remain bounded by the assigned actor count. */
LUMEN_BENCHMARK_ARGS( FWorldPartitionBench, BoundedQueries, 100, 1000, 10000 )
{
    const USize Count = static_cast<USize>( State.GetArgument() );
    FWorldPartition Partition;
    static_cast<void>( Partition.Initialize( MakePartitionConfig( Count ) ) );
    RegisterActors( Partition, Count );
    TVector<ActorID> Members;
    Members.resize( Count );

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<UInt64>( Count ) * sizeof( ActorID ) );

    for ( auto _ : State )
    {
        USize Total = 0ULL;
        for ( USize CellIndex = 0ULL; CellIndex < BenchmarkCells; ++CellIndex )
        {
            Total += Partition.GetCellActors( FWorldCellId{ CellIndex + 1ULL }, TSpan<ActorID>( Members.data(), Count ) );
        }
        Benchmarks::DoNotOptimize( Total );
    }
}

} // namespace Lumen
