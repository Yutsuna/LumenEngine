/**
 * @file ReplicationFrameBenchmarks.cpp
 * @brief Codec cost for bounded replication frames at increasing actor counts.
 */

#include <Replication/ReplicationFrame.hpp>
#include <Replication/ReplicationTypes.hpp>

#include <Benchmarks/Benchmark.hpp>

#include "ReplicationBenchSupport.hpp"
#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace
{

void AppendActor ( Lumen::FReplicationFrameBuilder &InBuilder, const Lumen::UInt64 InValue ) noexcept
{
    const Lumen::FPersistentActorId Persistent{ InValue };
    Lumen::FReplicationRecord Spawn;
    Spawn.Actor        = Persistent;
    Spawn.Kind         = Lumen::EActorReplicationKind::Spawn;
    Spawn.PropertyRank = static_cast<Lumen::UInt8>( Lumen::EActorReplicationProperty::Alive );
    static_cast<void>( InBuilder.TryAppend( Spawn ) );

    Lumen::FReplicationRecord Transform;
    Transform.Actor        = Persistent;
    Transform.Kind         = Lumen::EActorReplicationKind::Transform;
    Transform.PropertyRank = static_cast<Lumen::UInt8>( Lumen::EActorReplicationProperty::LocalTransform );
    Transform.SetLocalTransform( Lumen::Maths::FMat4x4f::Identity() );
    static_cast<void>( InBuilder.TryAppend( Transform ) );

    Lumen::FReplicationRecord Parent;
    Parent.Actor        = Persistent;
    Parent.Kind         = Lumen::EActorReplicationKind::Parent;
    Parent.PropertyRank = static_cast<Lumen::UInt8>( Lumen::EActorReplicationProperty::Parent );
    Parent.SetParent( Lumen::InvalidPersistentActorId );
    static_cast<void>( InBuilder.TryAppend( Parent ) );

    Lumen::FReplicationRecord Visibility;
    Visibility.Actor        = Persistent;
    Visibility.Kind         = Lumen::EActorReplicationKind::Visibility;
    Visibility.PropertyRank = static_cast<Lumen::UInt8>( Lumen::EActorReplicationProperty::Visibility );
    Visibility.SetVisibility( true );
    static_cast<void>( InBuilder.TryAppend( Visibility ) );
}

} // namespace

/** @brief Full snapshot build at reserved capacities; no allocation occurs inside the loop. */
LUMEN_BENCHMARK_ARGS( FReplicationFrame, BuildAllDirty, 100, 1000, 10000 )
{
    const Lumen::Int64 Argument            = State.GetArgument();
    const Lumen::USize Count               = Argument > 0 ? static_cast<Lumen::USize>( Argument ) : 100ULL;
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( Count );
    Lumen::FReplicationFrameBuilder Builder;
    Builder.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    Lumen::TVector<Lumen::Byte> Bytes;
    Bytes.reserve( Config.MaxPayloadBytes );
    Bytes.resize( Config.MaxPayloadBytes );

    Lumen::FReplicationFrameHeader Header;
    Header.Direction   = Lumen::EReplicationDirection::StateToReplica;
    Header.Channel     = Lumen::EReplicationChannel::Reliable;
    Header.RecordCount = static_cast<Lumen::UInt16>( Count * 4ULL );
    Header.Flags       = Lumen::ReplicationFrameFullSnapshot;

    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( Count ) * Lumen::MaxReplicationRecordBytes );
    for ( auto _ : State )
    {
        Builder.Reset();
        for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
        {
            AppendActor( Builder, Index + 1ULL );
        }
        const auto Written = Builder.TryWrite( Bytes, Header );
        Lumen::Benchmarks::DoNotOptimize( Written );
    }
}

/** @brief Parse a prebuilt full snapshot; measures checksum and record reconstruction. */
LUMEN_BENCHMARK_ARGS( FReplicationFrame, ParseAllDirty, 100, 1000, 10000 )
{
    const Lumen::Int64 Argument            = State.GetArgument();
    const Lumen::USize Count               = Argument > 0 ? static_cast<Lumen::USize>( Argument ) : 100ULL;
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( Count );
    Lumen::FReplicationFrameBuilder Builder;
    Builder.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    Lumen::TVector<Lumen::Byte> Bytes;
    Bytes.reserve( Config.MaxPayloadBytes );
    Bytes.resize( Config.MaxPayloadBytes );
    for ( Lumen::USize Index = 0ULL; Index < Count; ++Index )
    {
        AppendActor( Builder, Index + 1ULL );
    }
    Lumen::FReplicationFrameHeader Header;
    Header.Direction   = Lumen::EReplicationDirection::StateToReplica;
    Header.Channel     = Lumen::EReplicationChannel::Reliable;
    Header.RecordCount = static_cast<Lumen::UInt16>( Count * 4ULL );
    Header.Flags       = Lumen::ReplicationFrameFullSnapshot;
    const auto Written = Builder.TryWrite( Bytes, Header );
    if ( not Written )
    {
        return;
    }

    Lumen::FReplicationFrameBuilder Parser;
    Parser.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    State.SetItemsPerIteration( Count );
    State.SetBytesPerIteration( static_cast<Lumen::UInt64>( *Written ) );
    for ( auto _ : State )
    {
        const auto Parsed = Parser.TryParse( { Bytes.data(), *Written } );
        Lumen::Benchmarks::DoNotOptimize( Parsed );
    }
}

/** @brief Empty frame baseline: header plus checksum only. */
LUMEN_BENCHMARK( FReplicationFrame, Idle )
{
    const Lumen::FReplicationConfig Config = Lumen::BenchReplication::MakeConfig( 8ULL );
    Lumen::FReplicationFrameBuilder Builder;
    Builder.Reserve( Config.MaxPayloadBytes, Config.MaxRecordsPerFrame );
    Lumen::TVector<Lumen::Byte> Bytes;
    Bytes.reserve( Config.MaxPayloadBytes );
    Bytes.resize( Config.MaxPayloadBytes );
    Lumen::FReplicationFrameHeader Header;
    Header.Direction = Lumen::EReplicationDirection::StateToReplica;
    Header.Channel   = Lumen::EReplicationChannel::Reliable;

    State.SetItemsPerIteration( 1ULL );
    State.SetBytesPerIteration( Lumen::ReplicationFrameHeaderBytes + Lumen::ReplicationFrameChecksumBytes );
    for ( auto _ : State )
    {
        Builder.Reset();
        const auto Written = Builder.TryWrite( Bytes, Header );
        Lumen::Benchmarks::DoNotOptimize( Written );
    }
}
