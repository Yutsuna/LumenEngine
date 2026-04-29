/**
 * @file CompilerCache.inl
 * @brief Inline implementations for the TCompilerCache class template.
 */

#pragma once

#include "Container/File.hpp"
#include <chrono>

template <typename TTraits>
LumenEngine::Compiler::TCompilerCache<TTraits>::TCompilerCache ( const typename TTraits::ConfigType &InConfig ) : Config( InConfig ), MemoryCache( 1024U )
{
    /* Ctor */
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
    Meta.CompiledAtNs = static_cast<UInt64>( HAL::FPlatformTime::Seconds() * 1.0E9 );

    FIOFile::WriteAllBytes( BinaryPath, TTraits::GetBinaryData( InCompiled ) );
    FIOFile::WriteAllBytes( MetaPath, Meta.Serialize() );
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
    for ( const auto &Entry : std::filesystem::directory_iterator( Config.CacheDirectory ) )
    {
        const auto Extension = Entry.path().extension();
        if ( Extension == ".meta" or Extension == TTraits::BinaryExtension )
        {
            if ( std::filesystem::remove( Entry.path(), ErrorCode ) )
                ++RemovedCount;
        }
    }
    return RemovedCount / 2U;
}

template <typename TTraits> LumenEngine::USize LumenEngine::Compiler::TCompilerCache<TTraits>::ClearStale ( const Float64 InMaxAgeSeconds ) noexcept
{
    USize RemovedCount    = 0U;
    const UInt64 NowNs    = static_cast<UInt64>( std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() );
    const UInt64 MaxAgeNs = static_cast<UInt64>( InMaxAgeSeconds * 1.0E9 );

    for ( const auto &Entry : std::filesystem::directory_iterator( Config.CacheDirectory ) )
    {
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
