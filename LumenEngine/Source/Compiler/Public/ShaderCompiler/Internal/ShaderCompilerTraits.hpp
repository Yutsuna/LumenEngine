/**
 * @file ShaderCompilerTraits.hpp
 * @brief Traits definition for FShaderCompiler to use with TCompiler.
 */

#pragma once

#include "CoreTypes.hpp"

#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

#include "Filesystem/Path.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @struct FShaderCompilerTraits
     * @brief Traits for the shader compiler implementation.
     */
    struct FShaderCompilerTraits
    {
        using RequestType    = FShaderCompileRequest;
        using ResultType     = FShaderCompileResult;
        using CompiledType   = FCompiledShader;
        using ConfigType     = FShaderCompilerConfig;
        using MetaType       = FShaderCacheMetaData;
        using BinaryWordType = FSpirvWord;

        /** File extension used for the binary payload written to disk. */
        static constexpr const AnsiChar *BinaryExtension = ".spv";

        /**
         * @brief Return the source file path embedded in a compile request.
         * @param InRequest The compile request.
         * @return Path as a string.
         */
        [[nodiscard]] static FString GetSourcePath ( const FShaderCompileRequest &InRequest ) noexcept;

        /**
         * @brief Build the full filesystem path for a cache file.
         * @param InCacheDir  Root cache directory.
         * @param InHash      64-bit FNV-1a hash of the request.
         * @param InRequest   Compile request (used for the block-name suffix).
         * @param InExt       File extension including the leading dot.
         * @return Full filesystem path as a string.
         */
        [[nodiscard]] static FString
        BuildCachePath ( const Filesystem::FPath &InCacheDir, FSourceHash InHash, const FShaderCompileRequest &InRequest, const AnsiChar *InExt ) noexcept;

        /**
         * @brief Validate that a deserialised metadata entry matches the request.
         * @param InMeta     Deserialised metadata.
         * @param InHash     Expected hash.
         * @param InRequest  The original compile request.
         * @return True if hash, asset-type, and block-name all match.
         */
        [[nodiscard]] static Bool IsValidMeta ( const FShaderCacheMetaData &InMeta, FSourceHash InHash, const FShaderCompileRequest &InRequest ) noexcept;

        /**
         * @brief Reconstruct a FCompiledShader from cached data.
         * @param InHash    Hash of the cached entry.
         * @param InMeta    Deserialised metadata.
         * @param InBinary  Raw binary blob loaded from disk.
         * @return Fully populated FCompiledShader.
         */
        [[nodiscard]] static FCompiledShader RestoreFromCache ( FSourceHash InHash, const FShaderCacheMetaData &InMeta, TVector<FSpirvWord> &&InBinary ) noexcept;

        /**
         * @brief Build metadata from a freshly compiled asset.
         * @param InHash      Hash of the compiled asset.
         * @param InRequest   The compile request that produced the asset.
         * @param InCompiled  The compiled asset.
         * @return Populated FShaderCacheMetaData (CompiledAtNs is set by TCompilerCache).
         */
        [[nodiscard]] static FShaderCacheMetaData CreateMeta ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept;

        /**
         * @brief Return the binary payload to persist on disk.
         * @param InCompiled The compiled asset.
         * @return Const reference to the raw byte vector.
         */
        [[nodiscard]] static const TVector<FSpirvWord> &GetBinaryData ( const FCompiledShader &InCompiled ) noexcept;
    };

} // namespace Compiler

} // namespace LumenEngine
