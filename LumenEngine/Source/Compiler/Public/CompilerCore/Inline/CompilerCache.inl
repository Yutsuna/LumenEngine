/**
 * @file CompilerCache.inl
 * @brief Inline implementations for the TCompilerCache class template.
 */

#pragma once

#include "CompilerCore/CompilerCache.hpp"
#include "CompilerCore/CompilerTypes.hpp"

#include "Container/File.hpp"
#include "HAL/PlatformTime.hpp"

template <typename TTraits>
LumenEngine::Compiler::TCompilerCache<TTraits>::TCompilerCache( const typename TTraits::ConfigType &InConfig ) : Config( InConfig ), MemoryCache( 1024U )
{
    std::error_code ErrorCode;
    std::filesystem::create_directories( Config.CacheDirectory, ErrorCode );

    if ( ErrorCode )
    {
        LUMEN_LOG_ERROR( LogCompiler, "TCompilerCache: Failed to create cache directory '{}': {}", Config.CacheDirectory.string(), ErrorCode.message() );
    }
}

template <typename TTraits>
LumenEngine::TOptional<typename TTraits::CompiledType> LumenEngine::Compiler::TCompilerCache<TTraits>::TryGet ( const FSourceHash InHash,
                                                                                                                const TRequest &InRequest ) noexcept
{
    if ( TOptional<TCompiled> MemoryResult = MemoryCache.TryGetCopy( InHash ) )
    {
        return MemoryResult;
    }

    const FString MetaPath   = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" );
    const FString BinaryPath = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension );

    std::error_code Ec;
    if ( not std::filesystem::exists( MetaPath, Ec ) )
    {
        return std::nullopt;
    }

    TOptional<TVector<Byte>> MetaData = FIOFile::ReadAllBytes<Byte>( MetaPath );
    if ( MetaData.has_value() )
    {
        TOptional<TMeta> MetaOpt = TMeta::Deserialize( std::span<const Byte>( *MetaData ) );
        if ( MetaOpt.has_value() and TTraits::IsValidMeta( *MetaOpt, InHash, InRequest ) )
        {
            TOptional<TVector<typename TTraits::BinaryWordType>> BinaryData = FIOFile::ReadAllBytes<typename TTraits::BinaryWordType>( BinaryPath );
            if ( BinaryData.has_value() )
            {
                TCompiled Compiled = TTraits::RestoreFromCache( InHash, InRequest, *MetaOpt, std::move( *BinaryData ) );
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
    const FString MetaPath   = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" );
    const FString BinaryPath = TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension );

    TMeta Meta        = TTraits::CreateMeta( InHash, InRequest, InCompiled );
    Meta.CompiledAtNs = HAL::FPlatformTime::NowNanoseconds();

    if ( not FIOFile::WriteAllBytes( BinaryPath, TTraits::GetBinaryData( InCompiled ) ) )
    {
        LUMEN_LOG_WARNING( LogCompiler, "TCompilerCache: Failed to write binary data to '{}'", BinaryPath );
    }

    if ( not FIOFile::WriteAllBytes( MetaPath, Meta.Serialize() ) )
    {
        LUMEN_LOG_WARNING( LogCompiler, "TCompilerCache: Failed to write metadata to '{}'", MetaPath );
    }
}

template <typename TTraits> void LumenEngine::Compiler::TCompilerCache<TTraits>::Invalidate ( const FSourceHash InHash, const TRequest &InRequest ) noexcept
{
    MemoryCache.Erase( InHash );

    std::error_code ErrorCode;
    std::filesystem::remove( TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, ".meta" ), ErrorCode );
    std::filesystem::remove( TTraits::BuildCachePath( Config.CacheDirectory, InHash, InRequest, TTraits::BinaryExtension ), ErrorCode );
}

template <typename TTraits> LumenEngine::USize LumenEngine::Compiler::TCompilerCache<TTraits>::Clear () noexcept
{
    MemoryCache.Clear();
    USize RemovedCount = 0U;
    std::error_code ErrorCode;

    if ( not std::filesystem::exists( Config.CacheDirectory, ErrorCode ) )
    {
        return 0U;
    }

    for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory, ErrorCode ) )
    {

        if ( ErrorCode )
        {
            LUMEN_LOG_ERROR( LogCompiler, "TCompilerCache: Failed to iterate cache directory '{}': {}", Config.CacheDirectory.string(), ErrorCode.message() );
            break;
        }

        const auto Extension = Entry.path().extension();
        if ( Extension == ".meta" or Extension == TTraits::BinaryExtension )
        {
            if ( std::filesystem::remove( Entry.path(), ErrorCode ) )
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
    std::error_code ErrorCode;

    if ( not std::filesystem::exists( Config.CacheDirectory, ErrorCode ) )
    {
        return 0U;
    }

    for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory, ErrorCode ) )
    {

        if ( ErrorCode )
        {
            LUMEN_LOG_ERROR( LogCompiler, "TCompilerCache: Failed to iterate cache directory '{}': {}", Config.CacheDirectory.string(), ErrorCode.message() );
            break;
        }

        if ( Entry.path().extension() == ".meta" )
        {
            if ( auto MetaBytes = FIOFile::ReadAllBytes<Byte>( Entry.path().string() ) )
            {
                if ( auto MetaOpt = TMeta::Deserialize( std::span<const Byte>( *MetaBytes ) ) )
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
