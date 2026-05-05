/**
 * @file HotReload.hpp
 * @brief Runtime asset hot-reload utility.
 */

#pragma once

#include "Container/Function.hpp"
#include "Container/Map.hpp"
#include "Container/String.hpp"

#include "LumenCompiler/LumenCompilerTypes.hpp"

#include <filesystem>

namespace LumenEngine
{

namespace Compiler
{

    class FAssetCompiler;

    /**
     * @class FCompilerHotReload
     * @brief Watches an asset directory and triggers re-compilation on changes.
     */
    class LUMEN_ENGINE_API FCompilerHotReload final
    {
    public:

        /** Callback triggered when an asset is successfully recompiled. */
        using FOnAssetReloaded = TFunction<void( const FString &InFilePath, EAssetType::Type InAssetType )>;

        FCompilerHotReload ( FAssetCompiler &InAssetCompiler, FString InAssetsPath ) noexcept;
        ~FCompilerHotReload () noexcept = default;

    public:

        /**
         * @brief Checks for modified files and re-compiles them.
         * Should be called regularly (e.g., from the application tick).
         */
        void Tick () noexcept;

        /** @brief Sets the callback for when an asset is successfully recompiled. */
        void SetOnAssetReloadedCallback ( FOnAssetReloaded InCallback ) noexcept;

    private:

        /** Scans for new or modified files. */
        void Scan () noexcept;

        /** Scans a specific folder and updates the watched files map. */
        void ScanFolder ( const std::filesystem::path &InPath, EAssetType::Type InAssetType ) noexcept;

    private:

        FAssetCompiler &AssetCompiler;
        FString AssetsPath;
        FOnAssetReloaded OnAssetReloaded;

        struct FFileState
        {
            std::filesystem::file_time_type LastWriteTime;
            EAssetType::Type AssetType = EAssetType::Unknown;
        };

        TMap<std::filesystem::path, FFileState> WatchedFiles;

        /** Minimum time between scans to avoid excessive I/O. */
        Float64 LastScanTime                  = 0.0;
        static constexpr Float64 ScanInterval = 1.0; //<< 1 second
    };

} // namespace Compiler

} // namespace LumenEngine
