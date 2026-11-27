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

namespace RHI
{

    namespace
    {

        constexpr UInt32 PipelineCacheVersion      = 2U;
        constexpr USize PipelineCacheHeaderSize    = 24U;
        constexpr USize PipelineCacheKeyHeaderSize = 48U;

        UInt64 GetKeyBytes ( const FRHIPipelineCacheKey &InKey ) noexcept
        {
            return static_cast<UInt64>( PipelineCacheKeyHeaderSize + InKey.Compatibility.ColorFormats.size() * sizeof( UInt32 ) +
                                        InKey.ShaderHashes.size() * sizeof( UInt64 ) );
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

        void WriteKey ( TVector<Byte> &InData, const FRHIPipelineCacheKey &InKey ) noexcept
        {
            WriteU64( InData, InKey.BackendCacheSchemaVersion );
            WriteU64( InData, InKey.StateHash );
            WriteU64( InData, InKey.Compatibility.LayoutHash );
            WriteU32( InData, static_cast<UInt32>( InKey.Compatibility.DepthStencilFormat ) );
            WriteU32( InData, static_cast<UInt32>( InKey.Compatibility.SampleCount ) );
            WriteU64( InData, InKey.Compatibility.VertexLayoutHash );
            WriteU32( InData, static_cast<UInt32>( InKey.Compatibility.ColorFormats.size() ) );
            WriteU32( InData, static_cast<UInt32>( InKey.ShaderHashes.size() ) );
            for ( const ERHIFormat Format : InKey.Compatibility.ColorFormats )
            {
                WriteU32( InData, static_cast<UInt32>( Format ) );
            }
            for ( const UInt64 ShaderHash : InKey.ShaderHashes )
            {
                WriteU64( InData, ShaderHash );
            }
        }

        Bool ReadKey ( const TSpan<const Byte> InData, USize &InOutOffset, FRHIPipelineCacheKey &OutKey ) noexcept
        {
            UInt32 DepthFormat = 0U;
            UInt32 SampleCount = 0U;
            UInt32 ColorCount  = 0U;
            UInt32 ShaderCount = 0U;
            if ( not ReadU64( InData, InOutOffset, OutKey.BackendCacheSchemaVersion ) or
                 not ReadU64( InData, InOutOffset, OutKey.StateHash ) or
                 not ReadU64( InData, InOutOffset, OutKey.Compatibility.LayoutHash ) or
                 not ReadU32( InData, InOutOffset, DepthFormat ) or not ReadU32( InData, InOutOffset, SampleCount ) or
                 not ReadU64( InData, InOutOffset, OutKey.Compatibility.VertexLayoutHash ) or
                 not ReadU32( InData, InOutOffset, ColorCount ) or not ReadU32( InData, InOutOffset, ShaderCount ) or
                 OutKey.BackendCacheSchemaVersion == 0ULL or ColorCount > 64U or ShaderCount > 16U or
                 DepthFormat >= static_cast<UInt32>( ERHIFormat::Count ) or
                 ToSampleCount( static_cast<ERHISampleCount>( SampleCount ) ) == 0U )
            {
                return false;
            }
            OutKey.Compatibility.DepthStencilFormat = static_cast<ERHIFormat>( DepthFormat );
            OutKey.Compatibility.SampleCount        = static_cast<ERHISampleCount>( SampleCount );
            for ( UInt32 ColorIndex = 0U; ColorIndex < ColorCount; ++ColorIndex )
            {
                UInt32 Format = 0U;
                if ( not ReadU32( InData, InOutOffset, Format ) or Format >= static_cast<UInt32>( ERHIFormat::Count ) )
                {
                    return false;
                }
                OutKey.Compatibility.ColorFormats.push_back( static_cast<ERHIFormat>( Format ) );
            }
            for ( UInt32 ShaderIndex = 0U; ShaderIndex < ShaderCount; ++ShaderIndex )
            {
                UInt64 ShaderHash = 0ULL;
                if ( not ReadU64( InData, InOutOffset, ShaderHash ) )
                {
                    return false;
                }
                OutKey.ShaderHashes.push_back( ShaderHash );
            }
            return true;
        }

        FRHIError CorruptedCacheError () noexcept
        {
            return MakeRHICorrupted( ERHIErrorCondition::Cache, "Invalid Null pipeline cache data" );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen

Lumen::USize Lumen::RHI::Tests::FNullPipelineCacheKeyHasher::operator()( const FRHIPipelineCacheKey &InKey ) const noexcept
{
    return static_cast<USize>( InKey.GetHash() );
}

Lumen::RHI::Tests::FNullPipelineCache::FNullPipelineCache ( const FRHIPipelineCacheDesc &InDesc,
                                                            TSharedPtr<FNullFaultPlan> InFaultPlan,
                                                            const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipelineCache>() ), Desc( InDesc ), FaultPlan( std::move( InFaultPlan ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullPipelineCache )

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullPipelineCache::Initialize () noexcept
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
    TVector<FRHIPipelineCacheKey> Restored;
    UInt64 TotalBytes = 0ULL;
    for ( UInt32 EntryIndex = 0U; EntryIndex < EntryCount; ++EntryIndex )
    {
        FRHIPipelineCacheKey Key;
        if ( not ReadKey( InitialData, Offset, Key ) or Key.BackendCacheSchemaVersion != Desc.BackendVersion )
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
        const std::pair<TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator, Bool> Inserted =
            Entries.emplace( Key, std::move( Entry ) );
        if ( not Inserted.second )
        {
            return TUnexpected<FRHIError>( CorruptedCacheError() );
        }
        Stats.OnInsert( Inserted.first->second.Bytes );
    }
    return {};
}

const Lumen::RHI::FRHIPipelineCacheDesc &Lumen::RHI::Tests::FNullPipelineCache::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TOptional<Lumen::TSharedPtr<Lumen::RHI::IRHIPipeline>>
Lumen::RHI::Tests::FNullPipelineCache::Find ( const FRHIPipelineCacheKey &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator Found = Entries.find( InKey );
    if ( Found == Entries.end() or not Found->second.Pipeline )
    {
        Stats.OnLookup( false );
        return {};
    }
    ++Sequence;
    Found->second.LastUsed = Sequence;
    Stats.OnLookup( true );
    return TOptional<TSharedPtr<IRHIPipeline>>( Found->second.Pipeline );
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullPipelineCache::Insert ( const FRHIPipelineCacheKey &InKey,
                                                                         TSharedRef<IRHIPipeline> InPipeline ) noexcept
{
    if ( FaultPlan )
    {
        if ( const TOptional<FRHIError> Error = FaultPlan->ShouldFail( "PipelineCacheInsert" ); Error.HasValue() )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    if ( InKey.BackendCacheSchemaVersion != Desc.BackendVersion or not( InKey.Compatibility == InPipeline->GetCompatibility() ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidCache( "Pipeline cache key or schema does not match the cache entry" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator Existing = Entries.find( InKey );
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

void Lumen::RHI::Tests::FNullPipelineCache::Remove ( const FRHIPipelineCacheKey &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator Found = Entries.find( InKey );
    if ( Found != Entries.end() )
    {
        Stats.OnRemove( Found->second.Bytes );
        Entries.erase( Found );
    }
}

void Lumen::RHI::Tests::FNullPipelineCache::Clear () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
}

Lumen::RHI::FRHICacheStats Lumen::RHI::Tests::FNullPipelineCache::GetStats () const noexcept
{
    return Stats.Snapshot();
}

Lumen::TExpected<Lumen::TVector<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullPipelineCache::Serialize () const noexcept
{
    if ( FaultPlan )
    {
        if ( const TOptional<FRHIError> Error = FaultPlan->ShouldFail( "PipelineCacheSerialize" ); Error.HasValue() )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    TVector<FRHIPipelineCacheKey> Keys;
    Keys.reserve( Entries.size() );
    for ( const auto &Pair : Entries )
    {
        Keys.push_back( Pair.first );
    }
    Algo::Sort( Keys, [] ( const FRHIPipelineCacheKey &Left, const FRHIPipelineCacheKey &Right )
    {
        const UInt64 LeftHash  = Left.GetHash();
        const UInt64 RightHash = Right.GetHash();
        if ( LeftHash != RightHash )
        {
            return LeftHash < RightHash;
        }
        if ( Left.Compatibility.LayoutHash != Right.Compatibility.LayoutHash )
        {
            return Left.Compatibility.LayoutHash < Right.Compatibility.LayoutHash;
        }
        if ( Left.Compatibility.DepthStencilFormat != Right.Compatibility.DepthStencilFormat )
        {
            return Left.Compatibility.DepthStencilFormat < Right.Compatibility.DepthStencilFormat;
        }
        if ( Left.Compatibility.SampleCount != Right.Compatibility.SampleCount )
        {
            return Left.Compatibility.SampleCount < Right.Compatibility.SampleCount;
        }
        if ( Left.Compatibility.VertexLayoutHash != Right.Compatibility.VertexLayoutHash )
        {
            return Left.Compatibility.VertexLayoutHash < Right.Compatibility.VertexLayoutHash;
        }
        if ( Left.Compatibility.ColorFormats != Right.Compatibility.ColorFormats )
        {
            return Left.Compatibility.ColorFormats < Right.Compatibility.ColorFormats;
        }
        if ( Left.StateHash != Right.StateHash )
        {
            return Left.StateHash < Right.StateHash;
        }
        if ( Left.BackendCacheSchemaVersion != Right.BackendCacheSchemaVersion )
        {
            return Left.BackendCacheSchemaVersion < Right.BackendCacheSchemaVersion;
        }
        return Left.ShaderHashes < Right.ShaderHashes;
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

Lumen::Bool Lumen::RHI::Tests::FNullPipelineCache::MakeRoom ( const UInt64 InBytes ) noexcept
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

void Lumen::RHI::Tests::FNullPipelineCache::EvictOne () noexcept
{
    TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator Oldest = Entries.begin();
    for ( TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher>::iterator Iterator = Entries.begin();
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
