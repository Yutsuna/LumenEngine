/**
 * @file LumenCompilerTraits.hpp
 * @brief Traits definition for FLumenCompiler to use with TCompiler<>.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Filesystem/Path.hpp"

#include "LumenCompiler/LumenCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @struct FLumenCompilerTraits
     * @brief Policy struct that maps the LumenCompiler pipeline onto TCompiler<>.
     */
    struct FLumenCompilerTraits
    {
        using RequestType    = FLumenCompileRequest;
        using ResultType     = FLumenCompileResult;
        using CompiledType   = FCompiledLumenAsset;
        using ConfigType     = FLumenCompilerConfig;
        using MetaType       = FLumenCacheMetaData;
        using BinaryWordType = Byte;

        /** File extension used for the binary payload written to disk. */
        static constexpr const AnsiChar *BinaryExtension = ".lumenbin";

        /**
         * @brief Return the source file path embedded in a compile request.
         * @param InRequest The compile request.
         * @return Path as a string.
         */
        [[nodiscard]] static FString GetSourcePath ( const FLumenCompileRequest &InRequest ) noexcept;

        /**
         * @brief Build the full filesystem path for a cache file.
         * @param InCacheDir  Root cache directory.
         * @param InHash      64-bit FNV-1a hash of the request.
         * @param InRequest   Compile request (used for the block-name suffix).
         * @param InExt       File extension including the leading dot.
         * @return Full filesystem path as a string.
         */
        [[nodiscard]] static FString
        BuildCachePath ( const Filesystem::FPath &InCacheDir, FAssetHash InHash, const FLumenCompileRequest &InRequest, const AnsiChar *InExt ) noexcept;

        /**
         * @brief Validate that a deserialised metadata entry matches the request.
         * @param InMeta     Deserialised metadata.
         * @param InHash     Expected hash.
         * @param InRequest  The original compile request.
         * @return True if hash, asset-type, and block-name all match.
         */
        [[nodiscard]] static Bool IsValidMeta ( const FLumenCacheMetaData &InMeta, FAssetHash InHash, const FLumenCompileRequest &InRequest ) noexcept;

        /**
         * @brief Reconstruct a FCompiledLumenAsset from cached data.
         * @param InHash    Hash of the cached entry.
         * @param InMeta    Deserialised metadata.
         * @param InBinary  Raw binary blob loaded from disk.
         * @return Fully populated FCompiledLumenAsset.
         */
        [[nodiscard]] static FCompiledLumenAsset RestoreFromCache ( FAssetHash InHash, const FLumenCacheMetaData &InMeta, TVector<Byte> &&InBinary ) noexcept;

        /**
         * @brief Build metadata from a freshly compiled asset.
         * @param InHash      Hash of the compiled asset.
         * @param InRequest   The compile request that produced the asset.
         * @param InCompiled  The compiled asset.
         * @return Populated FLumenCacheMetaData (CompiledAtNs is set by TCompilerCache).
         */
        [[nodiscard]] static FLumenCacheMetaData CreateMeta ( FAssetHash InHash, const FLumenCompileRequest &InRequest, const FCompiledLumenAsset &InCompiled ) noexcept;

        /**
         * @brief Return the binary payload to persist on disk.
         * @param InCompiled The compiled asset.
         * @return Const reference to the raw byte vector.
         */
        [[nodiscard]] static const TVector<Byte> &GetBinaryData ( const FCompiledLumenAsset &InCompiled ) noexcept;
    };

} // namespace Compiler

} // namespace LumenEngine
