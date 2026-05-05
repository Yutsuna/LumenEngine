/**
 * @file HotReload.cpp
 * @brief Implementation of the asset hot-reload utility.
 */

#include "Assets/HotReload.hpp"
#include "Assets/AssetCompiler.hpp"

#include "HAL/PlatformTime.hpp"
#include "Logging/Logger.hpp"

#include <filesystem>
#include <utility>

LUMEN_LOG_DEFINE_CATEGORY( LogHotReload, "HotReload" );

/**
 * Ctor
 */

LumenEngine::Compiler::FCompilerHotReload::FCompilerHotReload ( FAssetCompiler &InAssetCompiler, FString InAssetsPath ) noexcept
    : AssetCompiler( InAssetCompiler ), AssetsPath( std::move( InAssetsPath ) )
{
    const std::filesystem::path Root( AssetsPath.c_str() );

    if ( std::filesystem::exists( Root ) )
    {
        LUMEN_LOG_INFO( LogHotReload, "HotReload initialized for: {}", AssetsPath.c_str() );
        Scan();
    }
}

/**
 * Public
 */

void LumenEngine::Compiler::FCompilerHotReload::Tick () noexcept
{
    const Float64 CurrentTime = HAL::FPlatformTime::Seconds();

    if ( CurrentTime - LastScanTime >= ScanInterval )
    {
        Scan();
        LastScanTime = CurrentTime;
    }
}

void LumenEngine::Compiler::FCompilerHotReload::SetOnAssetReloadedCallback ( TFunction<void( const FString &, EAssetType::Type )> InCallback ) noexcept
{
    OnAssetReloaded = std::move( InCallback );
}

/**
 * Private
 */

void LumenEngine::Compiler::FCompilerHotReload::Scan () noexcept
{
    const std::filesystem::path Root( AssetsPath.c_str() );

    if ( not std::filesystem::exists( Root ) )
    {
        return;
    }

    ScanFolder( Root / "Materials", EAssetType::Material );
    ScanFolder( Root / "Meshes", EAssetType::Mesh );
    ScanFolder( Root / "Shaders", EAssetType::Shader );
}

void LumenEngine::Compiler::FCompilerHotReload::ScanFolder ( const std::filesystem::path &InPath, EAssetType::Type InAssetType ) noexcept
{
    if ( not std::filesystem::exists( InPath ) )
    {
        return;
    }

    for ( const auto &Entry : std::filesystem::directory_iterator( InPath ) )
    {
        if ( not Entry.is_regular_file() )
        {
            continue;
        }

        const std::filesystem::path &FilePath = Entry.path();
        const std::filesystem::path Extension = FilePath.extension();

        switch ( InAssetType )
        {
        case EAssetType::Material:
        case EAssetType::Mesh:
            if ( Extension != ".lumen" )
            {
                continue;
            }
            break;

        case EAssetType::Shader:
            if ( Extension != ".vert" and Extension != ".frag" and Extension != ".comp" )
            {
                continue;
            }
            break;

        default:
            continue;
        }

        const std::filesystem::file_time_type LastWriteTime = std::filesystem::last_write_time( FilePath );

        if ( not WatchedFiles.contains( FilePath ) )
        {
            WatchedFiles[FilePath] = { .LastWriteTime = LastWriteTime, .AssetType = InAssetType };
        }
        else if ( WatchedFiles[FilePath].LastWriteTime != LastWriteTime )
        {
            LUMEN_LOG_INFO( LogHotReload, "Detected change in: {}. Re-compiling...", FilePath.string().c_str() );

            const FAssetCompileResult Result = AssetCompiler.CompileFile( FilePath.string(), InAssetType );

            if ( Result.IsSuccess() )
            {
                WatchedFiles[FilePath].LastWriteTime = LastWriteTime;
                if ( OnAssetReloaded )
                {
                    OnAssetReloaded( FilePath.string(), InAssetType );
                }
            }
        }
    }
}
