/**
 * @file CompilerCache.inl
 * @brief Inline implementations for the TCompilerCache class template.
 */

#pragma once

#include "CompilerCore/CompilerCache.hpp"
#include "CompilerCore/CompilerTypes.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include "Filesystem/Path.hpp"
#include "HAL/PlatformTime.hpp"
#include "Logging/Logger.hpp"

template <typename TTraits>
LumenEngine::Compiler::TCompilerCache<TTraits>::TCompilerCache( const typename TTraits::ConfigType &InConfig ) : Config( InConfig ), MemoryCache( 1024U )
{
    Filesystem::FDirectory::CreateDirectories( Config.CacheDirectory );
}

template <typename TTraits>
LumenEngine::TOptional<typename TTraits::CompiledType> LumenEngine::Compiler::TCompilerCache<TTraits>::TryGet ( const FSourceHash InHash,
                                                                                                                const TRequest &InRequest ) noexcept
{
    if ( TOptional<TCompiled> MemoryResult = MemoryCache.TryGetCopy( InHash ) )
    {
        return MemoryResult;
    }

    const Filesystem::FPath MetaPath   = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" );
    const Filesystem::FPath BinaryPath = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension );

    if ( not Filesystem::FFile::Exists( MetaPath ) )
    {
        return std::nullopt;
    }

    auto MetaData = Filesystem::FFile::ReadAllBytes<Byte>( MetaPath );
    if ( MetaData.has_value() )
    {
        TOptional<TMeta> MetaOpt = TMeta::Deserialize( std::span<const Byte>( MetaData.value() ) );
        if ( MetaOpt.has_value() and TTraits::IsValidMeta( *MetaOpt, InHash, InRequest ) )
        {
            auto BinaryData = Filesystem::FFile::ReadAllBytes<typename TTraits::BinaryWordType>( BinaryPath );
            if ( BinaryData.has_value() )
            {
                TCompiled Compiled = TTraits::RestoreFromCache( InHash, InRequest, *MetaOpt, std::move( BinaryData.value() ) );
                MemoryCache.Put( InHash, Compiled );
                return Compiled;
            }
        }
    }
    return std::nullopt;
}

template <typename TTraits>
void LumenEngine::Compiler::TCompilerCache<TTraits>::Put ( const FSourceHash InHash, const TRequest &InRequest, const TCompiled &InCompiled ) noexcept
{
    MemoryCache.Put( InHash, InCompiled );
    const Filesystem::FPath MetaPath   = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" );
    const Filesystem::FPath BinaryPath = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension );

    TMeta Meta        = TTraits::CreateMeta( InHash, InRequest, InCompiled );
    Meta.CompiledAtNs = HAL::FPlatformTime::NowNanoseconds();

    if ( not Filesystem::FFile::WriteAllBytes( BinaryPath, TTraits::GetBinaryData( InCompiled ) ) )
    {
        LUMEN_LOG_WARNING( LogCompiler, "TCompilerCache: Failed to write binary data to '{}'", BinaryPath.ToString() );
    }

    if ( not Filesystem::FFile::WriteAllBytes( MetaPath, Meta.Serialize() ) )
    {
        LUMEN_LOG_WARNING( LogCompiler, "TCompilerCache: Failed to write metadata to '{}'", MetaPath.ToString() );
    }
}

template <typename TTraits> void LumenEngine::Compiler::TCompilerCache<TTraits>::Invalidate ( const FSourceHash InHash, const TRequest &InRequest ) noexcept
{
    MemoryCache.Erase( InHash );

    Filesystem::FFile::Delete( TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" ) );
    Filesystem::FFile::Delete( TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension ) );
}

template <typename TTraits> LumenEngine::USize LumenEngine::Compiler::TCompilerCache<TTraits>::Clear () noexcept
{
    MemoryCache.Clear();
    USize RemovedCount = 0U;

    if ( not Filesystem::FDirectory::Exists( Config.CacheDirectory ) )
    {
        return 0U;
    }

    auto FilesResult = Filesystem::FDirectory::GetFiles( Config.CacheDirectory );
    if ( not FilesResult )
    {
        return 0U;
    }

    for ( const auto &FileInfo : FilesResult.value() )
    {
        if ( FileInfo.Extension == ".meta" or FileInfo.Extension == TTraits::BinaryExtension )
        {
            if ( Filesystem::FFile::Delete( Filesystem::FPath( FileInfo.Path ) ) )
            {
                ++RemovedCount;
            }
        }
    }
    return RemovedCount / 2U;
}

template <typename TTraits> LumenEngine::USize LumenEngine::Compiler::TCompilerCache<TTraits>::ClearStale ( const Float64 InMaxAgeSeconds ) noexcept
{
    USize RemovedCount    = 0U;
    const UInt64 NowNs    = HAL::FPlatformTime::NowNanoseconds();
    const UInt64 MaxAgeNs = static_cast<UInt64>( InMaxAgeSeconds * 1.0E9 );

    if ( not Filesystem::FDirectory::Exists( Config.CacheDirectory ) )
    {
        return 0U;
    }

    auto FilesResult = Filesystem::FDirectory::GetFiles( Config.CacheDirectory );
    if ( not FilesResult )
    {
        return 0U;
    }

    for ( const auto &FileInfo : FilesResult.value() )
    {
        if ( FileInfo.Extension == ".meta" )
        {
            if ( auto MetaBytes = Filesystem::FFile::ReadAllBytes<Byte>( Filesystem::FPath( FileInfo.Path ) ) )
            {
                if ( auto MetaOpt = TMeta::Deserialize( std::span<const Byte>( MetaBytes.value() ) ) )
                {
                    if ( NowNs > MetaOpt->CompiledAtNs and ( NowNs - MetaOpt->CompiledAtNs ) > MaxAgeNs )
                    {
                        Invalidate( MetaOpt->SourceHash, {} );
                        ++RemovedCount;
                    }
                }
            }
        }
    }
    return RemovedCount;
}
