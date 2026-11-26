/**
 * @file OpenGLPipelineCache.cpp
 * @brief Implements bounded LRU caching and checksummed OpenGL cache serialization.
 */

#include <RHIOpenGL/Caches/OpenGLPipelineCache.hpp>

#include <Algo/Sort.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Pipelines/RHIPipelineCacheKey.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIHash.hpp>

#include <Templates/Pair.hpp>

/**
 * File-local helpers
 */

namespace
{

constexpr Lumen::UInt32 PipelineCacheVersion      = 1U;
constexpr Lumen::USize PipelineCacheHeaderSize    = 24U;
constexpr Lumen::USize PipelineCacheChecksumSize  = 8U;
constexpr Lumen::USize PipelineCacheKeyHeaderSize = 48U;

[[nodiscard]] Lumen::UInt64 GetKeyBytes ( const Lumen::RHI::FRHIPipelineCacheKey &InKey ) noexcept
{
    return static_cast<Lumen::UInt64>( PipelineCacheKeyHeaderSize ) +
           ( static_cast<Lumen::UInt64>( InKey.Compatibility.ColorFormats.size() ) *
             static_cast<Lumen::UInt64>( sizeof( Lumen::UInt32 ) ) ) +
           ( static_cast<Lumen::UInt64>( InKey.ShaderHashes.size() ) * static_cast<Lumen::UInt64>( sizeof( Lumen::UInt64 ) ) );
}

void WriteU32 ( Lumen::TVector<Lumen::Byte> &InData, const Lumen::UInt32 InValue ) noexcept
{
    for ( Lumen::UInt32 ByteIndex = 0U; ByteIndex < 4U; ++ByteIndex )
    {
        const Lumen::UInt32 Shift = ByteIndex * 8U;
        InData.push_back( static_cast<Lumen::Byte>( ( InValue >> Shift ) & 0xffU ) );
    }
}

void WriteU64 ( Lumen::TVector<Lumen::Byte> &InData, const Lumen::UInt64 InValue ) noexcept
{
    WriteU32( InData, static_cast<Lumen::UInt32>( InValue & 0xffffffffULL ) );
    WriteU32( InData, static_cast<Lumen::UInt32>( InValue >> 32U ) );
}

[[nodiscard]] Lumen::Bool
ReadU32 ( const Lumen::TSpan<const Lumen::Byte> InData, Lumen::USize &InOutOffset, Lumen::UInt32 &OutValue ) noexcept
{
    if ( InOutOffset > InData.size() or InData.size() - InOutOffset < 4U )
    {
        return false;
    }
    OutValue = 0U;
    for ( Lumen::UInt32 ByteIndex = 0U; ByteIndex < 4U; ++ByteIndex )
    {
        const Lumen::UInt32 Shift = ByteIndex * 8U;
        OutValue |= static_cast<Lumen::UInt32>( InData[InOutOffset + ByteIndex] ) << Shift;
    }
    InOutOffset += 4U;
    return true;
}

[[nodiscard]] Lumen::Bool
ReadU64 ( const Lumen::TSpan<const Lumen::Byte> InData, Lumen::USize &InOutOffset, Lumen::UInt64 &OutValue ) noexcept
{
    Lumen::UInt32 Low  = 0U;
    Lumen::UInt32 High = 0U;
    if ( not ReadU32( InData, InOutOffset, Low ) or not ReadU32( InData, InOutOffset, High ) )
    {
        return false;
    }
    OutValue = static_cast<Lumen::UInt64>( Low ) | ( static_cast<Lumen::UInt64>( High ) << 32U );
    return true;
}

void WriteKey ( Lumen::TVector<Lumen::Byte> &InData, const Lumen::RHI::FRHIPipelineCacheKey &InKey ) noexcept
{
    WriteU64( InData, InKey.BackendCacheSchemaVersion );
    WriteU64( InData, InKey.StateHash );
    WriteU64( InData, InKey.Compatibility.LayoutHash );
    WriteU32( InData, static_cast<Lumen::UInt32>( InKey.Compatibility.DepthStencilFormat ) );
    WriteU32( InData, static_cast<Lumen::UInt32>( InKey.Compatibility.SampleCount ) );
    WriteU64( InData, InKey.Compatibility.VertexLayoutHash );
    WriteU32( InData, static_cast<Lumen::UInt32>( InKey.Compatibility.ColorFormats.size() ) );
    WriteU32( InData, static_cast<Lumen::UInt32>( InKey.ShaderHashes.size() ) );
    for ( const Lumen::RHI::ERHIFormat Format : InKey.Compatibility.ColorFormats )
    {
        WriteU32( InData, static_cast<Lumen::UInt32>( Format ) );
    }
    for ( const Lumen::UInt64 ShaderHash : InKey.ShaderHashes )
    {
        WriteU64( InData, ShaderHash );
    }
}

[[nodiscard]] Lumen::Bool ReadKey ( const Lumen::TSpan<const Lumen::Byte> InData,
                                    Lumen::USize &InOutOffset,
                                    Lumen::RHI::FRHIPipelineCacheKey &OutKey ) noexcept
{
    Lumen::UInt32 DepthFormat = 0U;
    Lumen::UInt32 SampleCount = 0U;
    Lumen::UInt32 ColorCount  = 0U;
    Lumen::UInt32 ShaderCount = 0U;
    if ( not ReadU64( InData, InOutOffset, OutKey.BackendCacheSchemaVersion ) or
         not ReadU64( InData, InOutOffset, OutKey.StateHash ) or
         not ReadU64( InData, InOutOffset, OutKey.Compatibility.LayoutHash ) or not ReadU32( InData, InOutOffset, DepthFormat ) or
         not ReadU32( InData, InOutOffset, SampleCount ) or
         not ReadU64( InData, InOutOffset, OutKey.Compatibility.VertexLayoutHash ) or
         not ReadU32( InData, InOutOffset, ColorCount ) or not ReadU32( InData, InOutOffset, ShaderCount ) or
         OutKey.BackendCacheSchemaVersion == 0ULL or ColorCount > 64U or ShaderCount > 16U or
         DepthFormat >= static_cast<Lumen::UInt32>( Lumen::RHI::ERHIFormat::Count ) or
         Lumen::RHI::ToSampleCount( static_cast<Lumen::RHI::ERHISampleCount>( SampleCount ) ) == 0U )
    {
        return false;
    }
    OutKey.Compatibility.DepthStencilFormat = static_cast<Lumen::RHI::ERHIFormat>( DepthFormat );
    OutKey.Compatibility.SampleCount        = static_cast<Lumen::RHI::ERHISampleCount>( SampleCount );
    for ( Lumen::UInt32 ColorIndex = 0U; ColorIndex < ColorCount; ++ColorIndex )
    {
        Lumen::UInt32 Format = 0U;
        if ( not ReadU32( InData, InOutOffset, Format ) or Format >= static_cast<Lumen::UInt32>( Lumen::RHI::ERHIFormat::Count ) )
        {
            return false;
        }
        OutKey.Compatibility.ColorFormats.push_back( static_cast<Lumen::RHI::ERHIFormat>( Format ) );
    }
    for ( Lumen::UInt32 ShaderIndex = 0U; ShaderIndex < ShaderCount; ++ShaderIndex )
    {
        Lumen::UInt64 ShaderHash = 0ULL;
        if ( not ReadU64( InData, InOutOffset, ShaderHash ) )
        {
            return false;
        }
        OutKey.ShaderHashes.push_back( ShaderHash );
    }
    return true;
}

[[nodiscard]] Lumen::RHI::FRHIError CorruptedCacheError () noexcept
{
    return Lumen::RHI::MakeRHICorrupted( Lumen::RHI::ERHIErrorCondition::Cache, "Invalid OpenGL pipeline cache data" );
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLPipelineCache>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLPipelineCache::Create ( const RHI::FRHIPipelineCacheDesc &InDesc, const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    TSharedRef<FOpenGLPipelineCache> Cache = MakeShared<FOpenGLPipelineCache>( InDesc, InId );
    LUMEN_TRY_EXPECT_ERROR( Cache->Initialize() );
    return Cache;
}

Lumen::RHIOpenGL::FOpenGLPipelineCache::FOpenGLPipelineCache ( const RHI::FRHIPipelineCacheDesc &InDesc,
                                                               const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLPipelineCache>( InId ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLPipelineCache::~FOpenGLPipelineCache () noexcept = default;

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLPipelineCache );

Lumen::USize
Lumen::RHIOpenGL::FOpenGLPipelineCache::FKeyHasher::operator()( const RHI::FRHIPipelineCacheKey &InKey ) const noexcept
{
    return static_cast<USize>( InKey.GetHash() );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLPipelineCache::Initialize () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( Desc.Validate() );

    const TSpan<const Byte> InitialData = Desc.InitialData;
    Desc.InitialData                    = {};
    if ( InitialData.empty() )
    {
        return {};
    }
    if ( InitialData.size() < PipelineCacheHeaderSize + PipelineCacheChecksumSize or InitialData[0] != static_cast<Byte>( 'L' ) or
         InitialData[1] != static_cast<Byte>( 'P' ) or InitialData[2] != static_cast<Byte>( 'C' ) or
         InitialData[3] != static_cast<Byte>( 'O' ) )
    {
        return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
    }

    USize Offset      = 4U;
    UInt32 Version    = 0U;
    UInt64 Backend    = 0ULL;
    UInt32 EntryCount = 0U;
    UInt32 Reserved   = 0U;
    if ( not ReadU32( InitialData, Offset, Version ) or not ReadU64( InitialData, Offset, Backend ) or
         not ReadU32( InitialData, Offset, EntryCount ) or not ReadU32( InitialData, Offset, Reserved ) )
    {
        return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
    }
    const USize PayloadSize = InitialData.size() - PipelineCacheChecksumSize;
    USize ChecksumOffset    = PayloadSize;
    UInt64 StoredChecksum   = 0ULL;
    if ( Version != PipelineCacheVersion or Backend != Desc.BackendVersion or Reserved != 0U or EntryCount > Desc.MaxEntries or
         not ReadU64( InitialData, ChecksumOffset, StoredChecksum ) or
         RHI::RHIHashBytes( TSpan<const Byte>( InitialData.data(), PayloadSize ) ) != StoredChecksum )
    {
        return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
    }

    TVector<RHI::FRHIPipelineCacheKey> Restored;
    UInt64 TotalBytes = 0ULL;
    for ( UInt32 EntryIndex = 0U; EntryIndex < EntryCount; ++EntryIndex )
    {
        RHI::FRHIPipelineCacheKey Key;
        if ( not ReadKey( InitialData, Offset, Key ) or Key.BackendCacheSchemaVersion != Desc.BackendVersion )
        {
            return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
        }
        const UInt64 KeyBytes = GetKeyBytes( Key );
        if ( TotalBytes > Desc.MaxBytes or KeyBytes > Desc.MaxBytes - TotalBytes )
        {
            return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
        }
        TotalBytes += KeyBytes;
        Restored.push_back( static_cast<RHI::FRHIPipelineCacheKey &&>( Key ) );
    }
    if ( Offset != PayloadSize )
    {
        return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
    for ( const RHI::FRHIPipelineCacheKey &Key : Restored )
    {
        ++Sequence;
        FEntry Entry{ .Pipeline = {}, .Bytes = GetKeyBytes( Key ), .LastUsed = Sequence };
        const TPair<TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator, Bool> Inserted =
            Entries.emplace( Key, static_cast<FEntry &&>( Entry ) );
        if ( not Inserted.second )
        {
            Entries.erase( Inserted.first );
            return TUnexpected<RHI::FRHIError>( CorruptedCacheError() );
        }
        Stats.OnInsert( Inserted.first->second.Bytes );
    }
    return {};
}

const Lumen::RHI::FRHIPipelineCacheDesc &Lumen::RHIOpenGL::FOpenGLPipelineCache::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TOptional<Lumen::TSharedPtr<Lumen::RHI::IRHIPipeline>>
Lumen::RHIOpenGL::FOpenGLPipelineCache::Find ( const RHI::FRHIPipelineCacheKey &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator Found = Entries.find( InKey );
    if ( Found == Entries.end() or not Found->second.Pipeline )
    {
        Stats.OnLookup( false );
        return {};
    }
    ++Sequence;
    Found->second.LastUsed = Sequence;
    Stats.OnLookup( true );
    return TOptional<TSharedPtr<RHI::IRHIPipeline>>( Found->second.Pipeline );
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLPipelineCache::Insert ( const RHI::FRHIPipelineCacheKey &InKey,
                                                                          TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept
{
    if ( InKey.BackendCacheSchemaVersion != Desc.BackendVersion or not( InKey.Compatibility == InPipeline->GetCompatibility() ) )
    {
        return TUnexpected<RHI::FRHIError>(
            RHI::MakeRHIInvalidCache( "Pipeline cache key or schema does not match the cache entry" ) );
    }

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator Existing = Entries.find( InKey );
    if ( Existing != Entries.end() )
    {
        Stats.OnRemove( Existing->second.Bytes );
        Entries.erase( Existing );
    }
    const UInt64 KeyBytes = GetKeyBytes( InKey );
    if ( not MakeRoom( KeyBytes ) )
    {
        return TUnexpected<RHI::FRHIError>(
            RHI::MakeRHIOutOfMemory( RHI::ERHIErrorCondition::Cache, "Pipeline cache capacity is exhausted" ) );
    }
    ++Sequence;
    FEntry Entry{ .Pipeline = InPipeline, .Bytes = KeyBytes, .LastUsed = Sequence };
    Entries.emplace( InKey, static_cast<FEntry &&>( Entry ) );
    Stats.OnInsert( KeyBytes );
    return {};
}

void Lumen::RHIOpenGL::FOpenGLPipelineCache::Remove ( const RHI::FRHIPipelineCacheKey &InKey ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator Found = Entries.find( InKey );
    if ( Found != Entries.end() )
    {
        Stats.OnRemove( Found->second.Bytes );
        Entries.erase( Found );
    }
}

void Lumen::RHIOpenGL::FOpenGLPipelineCache::Clear () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Entries.clear();
    Stats.Reset();
}

Lumen::RHI::FRHICacheStats Lumen::RHIOpenGL::FOpenGLPipelineCache::GetStats () const noexcept
{
    return Stats.Snapshot();
}

Lumen::TExpected<Lumen::TVector<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLPipelineCache::Serialize () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    TVector<RHI::FRHIPipelineCacheKey> Keys;
    Keys.reserve( Entries.size() );
    for ( const auto &Pair : Entries )
    {
        Keys.push_back( Pair.first );
    }
    Algo::Sort( Keys, [] ( const RHI::FRHIPipelineCacheKey &Left, const RHI::FRHIPipelineCacheKey &Right ) noexcept
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

    TVector<Byte> Data = { static_cast<Byte>( 'L' ), static_cast<Byte>( 'P' ), static_cast<Byte>( 'C' ),
                           static_cast<Byte>( 'O' ) };
    WriteU32( Data, PipelineCacheVersion );
    WriteU64( Data, Desc.BackendVersion );
    WriteU32( Data, static_cast<UInt32>( Keys.size() ) );
    WriteU32( Data, 0U );
    for ( const RHI::FRHIPipelineCacheKey &Key : Keys )
    {
        WriteKey( Data, Key );
    }
    WriteU64( Data, RHI::RHIHashBytes( TSpan<const Byte>( Data.data(), Data.size() ) ) );
    return Data;
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLPipelineCache::MakeRoom ( const UInt64 InBytes ) noexcept
{
    while ( Entries.size() >= static_cast<USize>( Desc.MaxEntries ) or Stats.Snapshot().Bytes > Desc.MaxBytes or
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

void Lumen::RHIOpenGL::FOpenGLPipelineCache::EvictOne () noexcept
{
    TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator Oldest = Entries.begin();
    for ( TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher>::iterator Iterator = Entries.begin(); Iterator != Entries.end();
          ++Iterator )
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
