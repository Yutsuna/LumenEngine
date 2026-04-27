/**
 * @file ShaderCache.cpp
 * @brief Implementation of the thread-safe shader cache.
 */

#include "Shader/Internal/ShaderCache.hpp"

#include "Container/File.hpp"
#include "HAL/PlatformTime.hpp"
#include "Shader/ShaderCompilerTypes.hpp"

#include <chrono>
#include <filesystem>
#include <format>

LumenEngine::Internal::FShaderCache::FShaderCache ( const FShaderCompilerConfig &InConfig ) noexcept : Config( InConfig )
{
    std::error_code ErrorCode;
    std::filesystem::create_directories( Config.CacheDirectory, ErrorCode );

    if ( ErrorCode and Config.ErrorCallback )
    {
        Config.ErrorCallback( std::format( "FShaderCache: Failed to create cache directory: {}", ErrorCode.message() ) );
    }
}

LumenEngine::TOptional<LumenEngine::FCompiledShader>
LumenEngine::Internal::FShaderCache::TryGet ( FSourceHash InHash, EShaderStage::Type InStage, FStringView InEntryPoint ) noexcept
{
    {
        TSharedLock<FSharedMutex> ReadLock( CacheMutex );
        if ( auto It = MemoryCache.find( InHash ); It != MemoryCache.end() )
        {
            FCompiledShader Cached = It->second;
            Cached.bFromCache      = true;
            return Cached;
        }
    }

    const FString MetaPath = BuildCachePath( InHash, InStage, ".meta" );
    const FString SpvPath  = BuildCachePath( InHash, InStage, ".spv" );

    TOptional<TVector<Byte>> MetaData = FIOFile::ReadAllBytes<Byte>( MetaPath );
    if ( not MetaData.has_value() )
    {
        return std::nullopt;
    }

    TOptional<FShaderCacheMetaData> MetaOpt = FShaderCacheMetaData::Deserialize( std::span<const Byte>( *MetaData ) );
    if ( not MetaOpt.has_value() or MetaOpt->SourceHash != InHash or MetaOpt->Stage != InStage )
    {
        return std::nullopt;
    }

    TOptional<TVector<FSpirvWord>> SpirV = FIOFile::ReadAllBytes<FSpirvWord>( SpvPath );
    if ( not SpirV.has_value() )
    {
        return std::nullopt;
    }

    FCompiledShader Shader;
    Shader.SpirV      = std::move( *SpirV );
    Shader.Hash       = InHash;
    Shader.Stage      = InStage;
    Shader.EntryPoint = InEntryPoint;
    Shader.bFromCache = true;

    {
        TUniqueLock<FSharedMutex> WriteLock( CacheMutex );
        MemoryCache.emplace( InHash, Shader );
    }

    return Shader;
}

void LumenEngine::Internal::FShaderCache::Put ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept
{
    {
        TUniqueLock<FSharedMutex> WriteLock( CacheMutex );
        MemoryCache.emplace( InHash, InCompiled );
    }

    const FString SpvPath  = BuildCachePath( InHash, InRequest.Stage, ".spv" );
    const FString MetaPath = BuildCachePath( InHash, InRequest.Stage, ".meta" );

    FShaderCacheMetaData Meta{ .SourceHash     = InHash,
                               .Stage          = InRequest.Stage,
                               .Optimization   = InRequest.Optimization,
                               .CompiledAtNs   = static_cast<UInt64>( HAL::FPlatformTime::Seconds() * 1e9 ),
                               .SpirVWordCount = static_cast<UInt32>( InCompiled.SpirV.size() ),
                               .EntryPoint     = InRequest.EntryPoint };

    if ( not FIOFile::WriteAllBytes( SpvPath, InCompiled.SpirV ) and Config.WarningCallback )
    {
        Config.WarningCallback( std::format( "FShaderCache: Failed to write SPV to '{}'", SpvPath ) );
    }

    if ( not FIOFile::WriteAllBytes( MetaPath, Meta.Serialize() ) and Config.WarningCallback )
    {
        Config.WarningCallback( std::format( "FShaderCache: Failed to write Meta to '{}'", MetaPath ) );
    }
}

LumenEngine::USize LumenEngine::Internal::FShaderCache::WarmUp () noexcept
{
    USize LoadedCount = 0ULL;

    try
    {
        if ( not std::filesystem::exists( Config.CacheDirectory ) )
        {
            return 0ULL;
        }

        for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory ) )
        {
            if ( Entry.path().extension() == ".meta" )
            {
                TOptional<TVector<Byte>> MetaBytes = FIOFile::ReadAllBytes<Byte>( Entry.path().string() );
                if ( MetaBytes.has_value() )
                {
                    TOptional<FShaderCacheMetaData> MetaOpt = FShaderCacheMetaData::Deserialize( std::span<const Byte>( *MetaBytes ) );
                    if ( MetaOpt.has_value() )
                    {
                        if ( TOptional<FCompiledShader> ShaderOpt = TryGet( MetaOpt->SourceHash, MetaOpt->Stage, MetaOpt->EntryPoint ) )
                        {
                            ++LoadedCount;
                        }
                    }
                }
            }
        }
    }
    catch ( const std::filesystem::filesystem_error &SystemError )
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while warming up cache: %s", SystemError.what() );
    }
    return LoadedCount;
}

void LumenEngine::Internal::FShaderCache::Invalidate ( FSourceHash InHash, EShaderStage::Type InStage ) noexcept
{
    {
        TUniqueLock<FSharedMutex> WriteLock( CacheMutex );
        MemoryCache.erase( InHash );
    }

    try
    {
        std::error_code ErrorCode;
        std::filesystem::remove( BuildCachePath( InHash, InStage, ".spv" ), ErrorCode );
        std::filesystem::remove( BuildCachePath( InHash, InStage, ".meta" ), ErrorCode );
        std::filesystem::remove( BuildCachePath( InHash, InStage, ".spvasm" ), ErrorCode );
    }
    catch ( const std::filesystem::filesystem_error &SystemError )
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while invalidating cache for hash {:016X}: %s", InHash, SystemError.what() );
    }
}

LumenEngine::USize LumenEngine::Internal::FShaderCache::Clear () noexcept
{
    USize RemovedCount = 0ULL;

    {
        TUniqueLock<FSharedMutex> WriteLock( CacheMutex );
        MemoryCache.clear();
    }

    try
    {
        if ( not std::filesystem::exists( Config.CacheDirectory ) )
        {
            return 0ULL;
        }

        std::error_code ErrorCode;
        for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory ) )
        {
            const std::filesystem::path Extension = Entry.path().extension();
            if ( Extension == ".meta" or Extension == ".spv" or Extension == ".spvasm" )
            {
                if ( std::filesystem::remove( Entry.path(), ErrorCode ) )
                    ++RemovedCount;
            }
        }
    }
    catch ( const std::filesystem::filesystem_error &SystemError )
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while clearing cache: %s", SystemError.what() );
    }

    return RemovedCount / 2ULL;
}

LumenEngine::USize LumenEngine::Internal::FShaderCache::ClearStale ( const double MaxAgeSeconds ) noexcept
{
    USize RemovedCount    = 0ULL;
    const UInt64 NowNs    = static_cast<UInt64>( std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() );
    const UInt64 MaxAgeNs = static_cast<UInt64>( MaxAgeSeconds * 1e9 );

    try
    {
        if ( not std::filesystem::exists( Config.CacheDirectory ) )
        {
            return 0ULL;
        }

        std::error_code ErrorCode;
        for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory ) )
        {
            if ( Entry.path().extension() == ".meta" )
            {
                if ( TOptional<TVector<Byte>> MetaBytes = FIOFile::ReadAllBytes<Byte>( Entry.path().string() ) )
                {
                    if ( TOptional<FShaderCacheMetaData> MetaOpt = FShaderCacheMetaData::Deserialize( std::span<const Byte>( *MetaBytes ) ) )
                    {
                        if ( NowNs > MetaOpt->CompiledAtNs and ( NowNs - MetaOpt->CompiledAtNs ) > MaxAgeNs )
                        {
                            Invalidate( MetaOpt->SourceHash, MetaOpt->Stage );
                            ++RemovedCount;
                        }
                    }
                }
            }
        }
    }
    catch ( const std::filesystem::filesystem_error &SystemError )
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "Filesystem error while clearing stale cache entries: %s", SystemError.what() );
    }

    return RemovedCount;
}

void LumenEngine::Internal::FShaderCache::RecordHit () noexcept
{
    CacheHitCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

void LumenEngine::Internal::FShaderCache::RecordMiss () noexcept
{
    CacheMissCount.fetch_add( 1ULL, std::memory_order_relaxed );
}

LumenEngine::UInt64 LumenEngine::Internal::FShaderCache::GetHits () const noexcept
{
    return CacheHitCount.load( std::memory_order_relaxed );
}

LumenEngine::UInt64 LumenEngine::Internal::FShaderCache::GetMisses () const noexcept
{
    return CacheMissCount.load( std::memory_order_relaxed );
}

LumenEngine::FString LumenEngine::Internal::FShaderCache::BuildCachePath ( const FSourceHash InHash, const EShaderStage::Type InStage, const AnsiChar *InExt ) const
{
    return ( std::filesystem::path( Config.CacheDirectory ) / std::format( "{:016X}_{}{}", InHash, EShaderStage::ToString( InStage ), InExt ) ).string();
}