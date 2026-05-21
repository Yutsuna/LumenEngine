/**
 * @file HotReload.cpp
 * @brief Implementation of the asset hot-reload utility.
 */

#include "Assets/HotReload.hpp"
#include "Assets/AssetCompiler.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/Path.hpp"

#include "HAL/PlatformTime.hpp"
#include "Logging/Logger.hpp"

#include <utility>

LUMEN_LOG_DEFINE_CATEGORY( LogHotReload, "HotReload" );

/**
 * Ctor
 */

LumenEngine::Engine::FCompilerHotReload::FCompilerHotReload ( FAssetCompiler &InAssetCompiler, FString InAssetsPath ) noexcept
    : AssetCompiler( InAssetCompiler ), AssetsPath( std::move( InAssetsPath ) )
{
    const Filesystem::FPath Root( AssetsPath );

    if ( Filesystem::FDirectory::Exists( Root ) )
    {
        LUMEN_LOG_INFO( LogHotReload, "HotReload initialized for: {}", AssetsPath.c_str() );
        Scan();
    }
}

/**
 * Public
 */

void LumenEngine::Engine::FCompilerHotReload::Tick () noexcept
{
    const Float64 CurrentTime = HAL::FPlatformTime::Seconds();

    if ( CurrentTime - LastScanTime >= ScanInterval )
    {
        Scan();
        LastScanTime = CurrentTime;
    }
}

void LumenEngine::Engine::FCompilerHotReload::SetOnAssetReloadedCallback ( TFunction<void( const FString &, Compiler::EAssetType::Type )> InCallback ) noexcept
{
    OnAssetReloaded = std::move( InCallback );
}

/**
 * Private
 */

void LumenEngine::Engine::FCompilerHotReload::Scan () noexcept
{
    const Filesystem::FPath Root( AssetsPath );

    if ( not Filesystem::FDirectory::Exists( Root ) )
    {
        return;
    }

    ScanFolder( Root / "Materials", Compiler::EAssetType::Material );
    ScanFolder( Root / "Meshes", Compiler::EAssetType::Mesh );
    ScanFolder( Root / "Shaders", Compiler::EAssetType::Shader );
}

void LumenEngine::Engine::FCompilerHotReload::ScanFolder ( const Filesystem::FPath &InPath, Compiler::EAssetType::Type InAssetType )
{
    if ( not Filesystem::FDirectory::Exists( InPath ) )
    {
        return;
    }

    auto FilesResult = Filesystem::FDirectory::GetFiles( InPath );
    if ( not FilesResult )
    {
        return;
    }

    for ( const Filesystem::FFileInfo &FileInfo : *FilesResult )
    {
        if ( FileInfo.IsDirectory() )
        {
            continue;
        }

        const FString &FilePath  = FileInfo.Path;
        const FString &Extension = FileInfo.Extension;

        switch ( InAssetType )
        {
        case Compiler::EAssetType::Material:
        case Compiler::EAssetType::Mesh:
            if ( Extension != ".lumen" )
            {
                continue;
            }
            break;

        case Compiler::EAssetType::Shader:
            if ( Extension != ".vert" and Extension != ".frag" and Extension != ".comp" )
            {
                continue;
            }
            break;

        default:
            continue;
        }

        const Float64 LastWriteTime = FileInfo.LastModified;

        if ( not WatchedFiles.contains( FilePath ) )
        {
            WatchedFiles[FilePath] = { .LastWriteTime = LastWriteTime, .AssetType = InAssetType };
        }
        else if ( WatchedFiles[FilePath].LastWriteTime != LastWriteTime )
        {
            LUMEN_LOG_INFO( LogHotReload, "Detected change in: {}. Re-compiling...", FilePath.c_str() );

            const Engine::FAssetCompileResult Result = AssetCompiler.CompileFile( FilePath, InAssetType );

            if ( Result.IsSuccess() )
            {
                WatchedFiles[FilePath].LastWriteTime = LastWriteTime;
                if ( OnAssetReloaded )
                {
                    OnAssetReloaded( FilePath, InAssetType );
                }
            }
        }
    }
}
