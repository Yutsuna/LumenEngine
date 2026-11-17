/**
 * @file NullCaches.cpp
 * @brief Implements deterministic Null pipeline caching and serialization.
 */

#include <Algo/Sort.hpp>
#include <Caches/NullCaches.hpp>
#include <HAL/LockGuard.hpp>
#include <Templates/Hash.hpp>

namespace Lumen
{

namespace
{

    constexpr UInt32 PipelineCacheVersion      = 1U;
    constexpr USize PipelineCacheHeaderSize    = 24U;
    constexpr USize PipelineCacheKeyHeaderSize = 40U;

    UInt64 GetKeyBytes ( const FRHIPipelineCompatibility &InKey ) noexcept
    {
        return static_cast<UInt64>( PipelineCacheKeyHeaderSize + InKey.ColorFormats.size() * sizeof( UInt32 ) );
    }

    void WriteU32 ( TVector<Byte> &InData, const UInt32 InValue ) noexcept
    {
        for ( UInt32 ByteIndex = 0U; ByteIndex < 4U; ++ByteIndex )
        {
            const UInt32 Shift = ByteIndex * 8U;
            InData.push_back( static_cast<Byte>( ( InValue >> Shift ) & 0xffU ) );
        }
    }

    void WriteU64 ( TVector<Byte> &InData, const UInt64 InValue ) noexcept
    {
        WriteU32( InData, static_cast<UInt32>( InValue & 0xffffffffULL ) );
        WriteU32( InData, static_cast<UInt32>( InValue >> 32U ) );
    }

    Bool ReadU32 ( const TSpan<const Byte> InData, USize &InOutOffset, UInt32 &OutValue ) noexcept
    {
        if ( InOutOffset > InData.size() or InData.size() - InOutOffset < 4U )
        {
            return false;
        }
        OutValue = 0U;
        for ( UInt32 ByteIndex = 0U; ByteIndex < 4U; ++ByteIndex )
        {
            const UInt32 Shift = ByteIndex * 8U;
            OutValue |= static_cast<UInt32>( InData[InOutOffset + ByteIndex] ) << Shift;
        }
        InOutOffset += 4U;
        return true;
    }

    Bool ReadU64 ( const TSpan<const Byte> InData, USize &InOutOffset, UInt64 &OutValue ) noexcept
    {
        UInt32 Low  = 0U;
        UInt32 High = 0U;
        if ( not ReadU32( InData, InOutOffset, Low ) or not ReadU32( InData, InOutOffset, High ) )
        {
            return false;
        }
        OutValue = static_cast<UInt64>( Low ) | ( static_cast<UInt64>( High ) << 32U );
        return true;
    }

    void WriteKey ( TVector<Byte> &InData, const FRHIPipelineCompatibility &InKey ) noexcept
    {
        WriteU64( InData, InKey.LayoutHash );
        WriteU32( InData, static_cast<UInt32>( InKey.DepthStencilFormat ) );
        WriteU32( InData, static_cast<UInt32>( InKey.SampleCount ) );
        WriteU64( InData, InKey.VertexLayoutHash );
        WriteU32( InData, static_cast<UInt32>( InKey.ColorFormats.size() ) );
        WriteU32( InData, 0U );
        for ( const ERHIFormat Format : InKey.ColorFormats )
        {
            WriteU32( InData, static_cast<UInt32>( Format ) );
        }
    }

    Bool ReadKey ( const TSpan<const Byte> InData, USize &InOutOffset, FRHIPipelineCompatibility &OutKey ) noexcept
    {
        UInt32 DepthFormat = 0U;
        UInt32 SampleCount = 0U;
        UInt32 ColorCount  = 0U;
        UInt32 Reserved    = 0U;
        if ( not ReadU64( InData, InOutOffset, OutKey.LayoutHash ) or not ReadU32( InData, InOutOffset, DepthFormat ) or
             not ReadU32( InData, InOutOffset, SampleCount ) or not ReadU64( InData, InOutOffset, OutKey.VertexLayoutHash ) or
             not ReadU32( InData, InOutOffset, ColorCount ) or not ReadU32( InData, InOutOffset, Reserved ) or Reserved != 0U or
             ColorCount > 64U or DepthFormat >= static_cast<UInt32>( ERHIFormat::Count ) or
             ToSampleCount( static_cast<ERHISampleCount>( SampleCount ) ) == 0U )
        {
            return false;
        }
        OutKey.DepthStencilFormat = static_cast<ERHIFormat>( DepthFormat );
        OutKey.SampleCount        = static_cast<ERHISampleCount>( SampleCount );
        for ( UInt32 ColorIndex = 0U; ColorIndex < ColorCount; ++ColorIndex )
        {
            UInt32 Format = 0U;
            if ( not ReadU32( InData, InOutOffset, Format ) or Format >= static_cast<UInt32>( ERHIFormat::Count ) )
            {
                return false;
            }
            OutKey.ColorFormats.push_back( static_cast<ERHIFormat>( Format ) );
        }
        return true;
    }

    FRHIError CorruptedCacheError () noexcept
    {
        return MakeRHICorrupted( ERHIErrorCondition::Cache, "Invalid Null pipeline cache data" );
    }

} // namespace

} // namespace Lumen

Lumen::USize Lumen::Tests::FNullPipelineCompatibilityHasher::operator()( const FRHIPipelineCompatibility &InKey ) const noexcept
{
    return static_cast<USize>( InKey.GetHash() );
}

Lumen::Tests::FNullPipelineCache::FNullPipelineCache ( const FRHIPipelineCacheDesc &InDesc,
                                                       TSharedPtr<FNullFaultPlan> InFaultPlan,
                                                       const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipelineCache>() ), Desc( InDesc ), FaultPlan( std::move( InFaultPlan ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullPipelineCache )

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullPipelineCache::Initialize () noexcept
{
    const TSpan<const Byte> InitialData = Desc.InitialData;
    Desc.InitialData                    = {};
    if ( InitialData.empty() )
    {
        return {};
    }
    if ( InitialData.size() < PipelineCacheHeaderSize or InitialData[0] != static_cast<Byte>( 'L' ) or
         InitialData[1] != static_cast<Byte>( 'R' ) or InitialData[2] != static_cast<Byte>( 'P' ) or
         InitialData[3] != static_cast<Byte>( 'C' ) )
    {
        return TUnexpected<FRHIError>( CorruptedCacheError() );
    }
    USize Offset      = 4U;
    UInt32 Version    = 0U;
    UInt64 Backend    = 0ULL;
    UInt32 EntryCount = 0U;
    UInt32 Reserved   = 0U;
    if ( not ReadU32( InitialData, Offset, Version ) or not ReadU64( InitialData, Offset, Backend ) or
         not ReadU32( InitialData, Offset, EntryCount ) or not ReadU32( InitialData, Offset, Reserved ) or
         Version != PipelineCacheVersion or Backend != Desc.BackendVersion or Reserved != 0U or EntryCount > Desc.MaxEntries )
    {
        return TUnexpected<FRHIError>( CorruptedCacheError() );
    }
    TVector<FRHIPipelineCompatibility> Restored;
    UInt64 TotalBytes = 0ULL;
    for ( UInt32 EntryIndex = 0U; EntryIndex < EntryCount; ++EntryIndex )
    {
        FRHIPipelineCompatibility Key;
        if ( not ReadKey( InitialData, Offset, Key ) )
        {
            return TUnexpected<FRHIError>( CorruptedCacheError() );
        }
        const UInt64 KeyBytes = GetKeyBytes( Key );
        if ( TotalBytes > Desc.MaxBytes or KeyBytes > Desc.MaxBytes - TotalBytes )
        {
            return TUnexpected<FRHIError>( CorruptedCacheError() );
        }
        TotalBytes += KeyBytes;
        Restored.push_back( std::move( Key ) );
    }
    if ( Offset != InitialData.size() )
    {
        return TUnexpected<FRHIError>( CorruptedCacheError() );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
    for ( const auto &Key : Restored )
    {
        ++Sequence;
        FEntry Entry{ .Pipeline = {}, .Bytes = GetKeyBytes( Key ), .LastUsed = Sequence };
        const std::pair<TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator, Bool> Inserted =
            Entries.emplace( Key, std::move( Entry ) );
        if ( not Inserted.second )
        {
            return TUnexpected<FRHIError>( CorruptedCacheError() );
        }
        Stats.OnInsert( Inserted.first->second.Bytes );
    }
    return {};
}

const Lumen::FRHIPipelineCacheDesc &Lumen::Tests::FNullPipelineCache::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::IRHIPipeline> Lumen::Tests::FNullPipelineCache::Find ( const FRHIPipelineCompatibility &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator Found = Entries.find( InKey );
    if ( Found == Entries.end() or not Found->second.Pipeline )
    {
        Stats.OnLookup( false );
        return {};
    }
    ++Sequence;
    Found->second.LastUsed = Sequence;
    Stats.OnLookup( true );
    return Found->second.Pipeline;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullPipelineCache::Insert ( const FRHIPipelineCompatibility &InKey,
                                                                                    TSharedRef<IRHIPipeline> InPipeline ) noexcept
{
    if ( FaultPlan )
    {
        if ( const TOptional<FRHIError> Error = FaultPlan->ShouldFail( "PipelineCacheInsert" ); Error.HasValue() )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    if ( not( InKey == InPipeline->GetCompatibility() ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidCache( "Pipeline cache key does not match the pipeline" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator Existing = Entries.find( InKey );
    if ( Existing != Entries.end() )
    {
        Stats.OnRemove( Existing->second.Bytes );
        Entries.erase( Existing );
    }
    const UInt64 KeyBytes = GetKeyBytes( InKey );
    if ( not MakeRoom( KeyBytes ) )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Cache, "Pipeline cache capacity is exhausted" ) );
    }
    ++Sequence;
    FEntry Entry{ .Pipeline = InPipeline, .Bytes = KeyBytes, .LastUsed = Sequence };
    Entries.emplace( InKey, std::move( Entry ) );
    Stats.OnInsert( KeyBytes );
    return {};
}

void Lumen::Tests::FNullPipelineCache::Remove ( const FRHIPipelineCompatibility &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator Found = Entries.find( InKey );
    if ( Found != Entries.end() )
    {
        Stats.OnRemove( Found->second.Bytes );
        Entries.erase( Found );
    }
}

void Lumen::Tests::FNullPipelineCache::Clear () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
}

Lumen::FRHICacheStats Lumen::Tests::FNullPipelineCache::GetStats () const noexcept
{
    return Stats.Snapshot();
}

Lumen::TExpected<Lumen::TVector<Lumen::Byte>, Lumen::FRHIError> Lumen::Tests::FNullPipelineCache::Serialize () const noexcept
{
    if ( FaultPlan )
    {
        if ( const TOptional<FRHIError> Error = FaultPlan->ShouldFail( "PipelineCacheSerialize" ); Error.HasValue() )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    TVector<FRHIPipelineCompatibility> Keys;
    Keys.reserve( Entries.size() );
    for ( const auto &Pair : Entries )
    {
        Keys.push_back( Pair.first );
    }
    Algo::Sort( Keys, [] ( const FRHIPipelineCompatibility &Left, const FRHIPipelineCompatibility &Right )
    {
        const UInt64 LeftHash  = Left.GetHash();
        const UInt64 RightHash = Right.GetHash();
        if ( LeftHash != RightHash )
        {
            return LeftHash < RightHash;
        }
        if ( Left.LayoutHash != Right.LayoutHash )
        {
            return Left.LayoutHash < Right.LayoutHash;
        }
        if ( Left.DepthStencilFormat != Right.DepthStencilFormat )
        {
            return Left.DepthStencilFormat < Right.DepthStencilFormat;
        }
        if ( Left.SampleCount != Right.SampleCount )
        {
            return Left.SampleCount < Right.SampleCount;
        }
        if ( Left.VertexLayoutHash != Right.VertexLayoutHash )
        {
            return Left.VertexLayoutHash < Right.VertexLayoutHash;
        }
        return Left.ColorFormats < Right.ColorFormats;
    } );
    TVector<Byte> Data = { static_cast<Byte>( 'L' ), static_cast<Byte>( 'R' ), static_cast<Byte>( 'P' ),
                           static_cast<Byte>( 'C' ) };
    WriteU32( Data, PipelineCacheVersion );
    WriteU64( Data, Desc.BackendVersion );
    WriteU32( Data, static_cast<UInt32>( Keys.size() ) );
    WriteU32( Data, 0U );
    for ( const auto &Key : Keys )
    {
        WriteKey( Data, Key );
    }
    return Data;
}

Lumen::Bool Lumen::Tests::FNullPipelineCache::MakeRoom ( const UInt64 InBytes ) noexcept
{
    while ( Entries.size() >= Desc.MaxEntries or Stats.Snapshot().Bytes > Desc.MaxBytes or
            InBytes > Desc.MaxBytes - Stats.Snapshot().Bytes )
    {
        if ( not Desc.bAllowEviction or Entries.empty() )
        {
            return false;
        }
        EvictOne();
    }
    return true;
}

void Lumen::Tests::FNullPipelineCache::EvictOne () noexcept
{
    TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator Oldest = Entries.begin();
    for ( TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher>::iterator Iterator = Entries.begin();
          Iterator != Entries.end(); ++Iterator )
    {
        if ( Iterator->second.LastUsed < Oldest->second.LastUsed )
        {
            Oldest = Iterator;
        }
    }
    if ( Oldest != Entries.end() )
    {
        Stats.OnEvict( Oldest->second.Bytes );
        Entries.erase( Oldest );
    }
}
