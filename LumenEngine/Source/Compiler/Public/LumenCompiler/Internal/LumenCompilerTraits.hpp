/**
 * @file LumenCompilerTraits.hpp
 * @brief Traits definition for FLumenCompiler to use with TCompiler<>.
 */

#pragma once

#include "CoreTypes.hpp"

#include "LumenCompiler/LumenCompilerTypes.hpp"

#include <filesystem>

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
        [[nodiscard]] static FString GetSourcePath ( const FLumenCompileRequest &InRequest ) noexcept
        {
            return InRequest.SourcePath;
        }

        /**
         * @brief Build the full filesystem path for a cache file.
         *
         * The hash already captures the complete request (source content +
         * TargetBlockName + ExpectedBlockType), so the file name encodes only
         * the hash and the desired extension.  The block name is appended as a
         * human-readable suffix so manual cache inspection is straightforward.
         *
         * @param InCacheDir  Root cache directory.
         * @param InHash      64-bit FNV-1a hash of the request.
         * @param InRequest   Compile request (used for the block-name suffix).
         * @param InExt       File extension including the leading dot.
         * @return Full filesystem path as a string.
         */
        [[nodiscard]] static FString
        BuildCachePath ( const std::filesystem::path &InCacheDir, FAssetHash InHash, const FLumenCompileRequest &InRequest, const AnsiChar *InExt ) noexcept
        {
            const FString Suffix = InRequest.TargetBlockName.empty() ? FString{} : ( FString( "_" ) + InRequest.TargetBlockName );
            return ( InCacheDir / std::format( "{:016x}{}{}", InHash, Suffix, InExt ) ).string();
        }

        /**
         * @brief Validate that a deserialised metadata entry matches the request.
         * @param InMeta     Deserialised metadata.
         * @param InHash     Expected hash.
         * @param InRequest  The original compile request.
         * @return True if hash, asset-type, and block-name all match.
         */
        [[nodiscard]] static Bool IsValidMeta ( const FLumenCacheMetaData &InMeta, FAssetHash InHash, const FLumenCompileRequest &InRequest ) noexcept
        {
            if ( InMeta.SourceHash != InHash )
            {
                return false;
            }

            // If the request specifies a block name, verify it matches.
            if ( not InRequest.TargetBlockName.empty() and InMeta.BlockName != InRequest.TargetBlockName )
            {
                return false;
            }

            return true;
        }

        /**
         * @brief Reconstruct a FCompiledLumenAsset from cached data.
         * @param InHash    Hash of the cached entry.
         * @param InMeta    Deserialised metadata.
         * @param InBinary  Raw binary blob loaded from disk.
         * @return Fully populated FCompiledLumenAsset.
         */
        [[nodiscard]] static FCompiledLumenAsset RestoreFromCache ( FAssetHash InHash,
                                                                    [[maybe_unused]] const FLumenCompileRequest &InRequest,
                                                                    const FLumenCacheMetaData &InMeta,
                                                                    TVector<Byte> &&InBinary ) noexcept
        {
            return { .BinaryBlob = std::move( InBinary ), .AssetType = InMeta.AssetType, .Hash = InHash, .BlockName = InMeta.BlockName, .bFromCache = true };
        }

        /**
         * @brief Build metadata from a freshly compiled asset.
         * @param InHash      Hash of the compiled asset.
         * @param InRequest   The compile request that produced the asset.
         * @param InCompiled  The compiled asset.
         * @return Populated FLumenCacheMetaData (CompiledAtNs is set by TCompilerCache).
         */
        [[nodiscard]] static FLumenCacheMetaData CreateMeta ( FAssetHash InHash, const FLumenCompileRequest &InRequest, const FCompiledLumenAsset &InCompiled ) noexcept
        {
            return { .SourceHash    = InHash,
                     .AssetType     = InCompiled.AssetType,
                     .CompiledAtNs  = 0, // Set by TCompilerCache::Put
                     .BlobByteCount = static_cast<UInt32>( InCompiled.BinaryBlob.size() ),
                     .BlockName     = InRequest.TargetBlockName };
        }

        /**
         * @brief Return the binary payload to persist on disk.
         * @param InCompiled The compiled asset.
         * @return Const reference to the raw byte vector.
         */
        [[nodiscard]] static const TVector<Byte> &GetBinaryData ( const FCompiledLumenAsset &InCompiled ) noexcept
        {
            return InCompiled.BinaryBlob;
        }
    };

} // namespace Compiler

} // namespace LumenEngine
