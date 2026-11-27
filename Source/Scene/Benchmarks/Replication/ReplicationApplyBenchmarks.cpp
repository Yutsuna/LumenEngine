/**
 * @file ReplicationApplyBenchmarks.cpp
 * @brief Cost of applying replicated absolute transforms on a replica.
 */

#include <Replication/ReplicationSystem.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include "ReplicationBenchSupport.hpp"

/** @brief All actors change transform every tick, so every frame carries dirty absolute values. */
LUMEN_BENCHMARK_ARGS( FReplicationApply, AllDirtyTransforms, 100, 1000, 10000 )
{
    const Lumen::Int64 Argument            = State.GetArgument();
    const Lumen::USize Count               = Argument > 0 ? static_cast<Lumen::USize>( Argument ) : 100ULL;
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( Count );
    Lumen::BenchReplication::FMemoryTransport StateTransport( Config.MaxPayloadBytes );
    Lumen::BenchReplication::FNoopActorHandler AuthorityHandler;
    Lumen::BenchReplication::FNoopActorHandler ReplicaHandler;
    Lumen::FReplicationSystem Authority;
    Lumen::FReplicationSystem Replica;
    static_cast<void>( Authority.Initialize( Config, Lumen::EActorRole::Authority, StateTransport, AuthorityHandler ) );
    static_cast<void>( Replica.Initialize( Config, Lumen::EActorRole::Replica, StateTransport, ReplicaHandler ) );

    Lumen::TVector<Lumen::FReplicationActorSourceRecord> Source;
    Source.reserve( Count );
    Source.resize( Count );
    for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
    {
        Source[Index].LocalId = static_cast<Lumen::ActorID>( Index + 1ULL );
    }
    const Lumen::FReplicationSourceView View{ .Actors = Source, .FrameIndex = 1ULL };

    /** INFO: Warm up the authority's shadow so measured frames carry only Transform records. */
    static_cast<void>( Authority.Tick( 1ULL, View ) );
    static_cast<void>( Replica.Tick( 1ULL, {} ) );

    State.SetItemsPerIteration( Count );
    for ( auto _ : State )
    {
        for ( Lumen::FReplicationActorSourceRecord &Record : Source )
        {
            Record.LocalTransform = Lumen::Maths::FMat4x4f::Translate(
                Lumen::Maths::FVec3f{ static_cast<Lumen::Float32>( Record.LocalId % 17ULL ), 0.0F, 0.0F } );
        }
        const auto AuthorityResult = Authority.Tick( 2ULL, View );
        const auto ReplicaResult   = Replica.Tick( 2ULL, {} );
        Lumen::Benchmarks::DoNotOptimize( AuthorityResult );
        Lumen::Benchmarks::DoNotOptimize( ReplicaResult );
    }
}
