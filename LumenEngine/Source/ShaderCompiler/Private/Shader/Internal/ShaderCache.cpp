/**
 * @file ShaderCache.cpp
 * @brief Implementation of the thread-safe shader cache using Core TCache.
 */

#include "Shader/Internal/ShaderCache.hpp"
#include "Shader/ShaderCompilerTypes.hpp"

#include "Container/File.hpp"

#include "HAL/PlatformTime.hpp"

#include <filesystem>
#include <format>

LumenEngine::Internal::FShaderCache::FShaderCache ( const FShaderCompilerConfig &InConfig ) noexcept : Config( InConfig ), MemoryCache( 1024U )
{
    std::error_code ErrorCode;
    std::filesystem::create_directories( Config.CacheDirectory, ErrorCode );

    if ( ErrorCode and Config.ErrorCallback )
    {
        Config.ErrorCallback( std::format( "FShaderCache: Failed to create cache directory: {}", ErrorCode.message() ) );
    }
}

LumenEngine::TOptional<LumenEngine::FCompiledShader>
LumenEngine::Internal::FShaderCache::TryGet ( FSourceHash InHash, EShaderStage::Type InStage, const FString &InEntryPoint ) noexcept
{
    if ( TOptional<FCompiledShader> MemoryResult = MemoryCache.TryGetCopy( InHash ) )
    {
        return MemoryResult;
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

    FCompiledShader Shader = {
        .SpirV = std::move( *SpirV ),
        /* */
        .Reflection = {},
        /* */
        .Stage = InStage,
        /* */
        .Hash = InHash,
        /* */
        .bFromCache = true,
        /* */
        .EntryPoint = InEntryPoint,
    };

    MemoryCache.Put( InHash, Shader );

    return Shader;
}

void LumenEngine::Internal::FShaderCache::Put ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept
{
    MemoryCache.Put( InHash, InCompiled );

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

void LumenEngine::Internal::FShaderCache::Invalidate ( FSourceHash InHash, EShaderStage::Type InStage ) noexcept
{

    if ( not MemoryCache.Contains( InHash ) )
    {
        return;
    }

    std::error_code ErrorCode;
    std::filesystem::remove( BuildCachePath( InHash, InStage, ".spv" ), ErrorCode );
    std::filesystem::remove( BuildCachePath( InHash, InStage, ".meta" ), ErrorCode );
}

LumenEngine::USize LumenEngine::Internal::FShaderCache::Clear () noexcept
{
    return MemoryCache.Clear();
}