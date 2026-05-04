/**
 * @file LumenCompiler.hpp
 * @brief Public API for the DLSL asset compiler with persistent cache.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "Container/String.hpp"

#include "CompilerCore/CompilerBase.hpp"

#include "LumenCompiler/Internal/DLSLTypes.hpp"
#include "LumenCompiler/Internal/LumenCompilerTraits.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class FLumenCompiler
     * @brief DLSL asset compiler with FNV-1a hashing and LRU + disk caching.
     *
     * @code
     *   FLumenCompilerConfig Config;
     *   Config.CacheDirectory = "Data/AssetCache/";
     *
     *   FLumenCompiler Compiler( Config );
     *
     *   FLumenCompileRequest Request;
     *   Request.SourcePath      = "Assets/Hero.dlsl";
     *   Request.TargetBlockName = "Hero";
     *   Request.ExpectedBlockType = "Mesh";
     *
     *   FLumenCompileResult Result = Compiler.CompileAsset( Request );
     *   if ( Result.IsSuccess() )
     *   {
     *       // Write Result.Asset->BinaryBlob to disk ...
     *   }
     * @endcode
     */
    class LUMEN_ENGINE_API FLumenCompiler final : public TCompiler<FLumenCompilerTraits, FLumenCompiler>
    {
    public:

        /**
         * @brief Constructs the LumenCompiler.
         * @param InConfig  Configuration (cache directory, scratch buffer size, etc.).
         */
        explicit FLumenCompiler ( FLumenCompilerConfig InConfig ) noexcept;

        ~FLumenCompiler () override = default;

    public:

        /**
         * @brief Compile (or load from cache) the asset described by InRequest.
         *
         * If InRequest.SourcePath is set, the source is read from disk.
         * On a cache hit, the result is returned immediately without re-parsing.
         *
         * @param InRequest  Compilation parameters.
         * @return FLumenCompileResult containing the asset blob or an error.
         */
        [[nodiscard]] FLumenCompileResult CompileAsset ( const FLumenCompileRequest &InRequest ) noexcept;

        /**
         * @brief Compile from an in-memory DLSL source string.
         *
         * Useful for hot-reload scenarios or unit tests where the source is
         * already in memory and no file I/O is desired.
         *
         * @param InSource   DLSL source text.
         * @param InRequest  Compilation parameters (SourcePath may be empty).
         * @return FLumenCompileResult containing the asset blob or an error.
         */
        [[nodiscard]] FLumenCompileResult CompileAssetFromSource ( FStringView InSource, const FLumenCompileRequest &InRequest ) noexcept;

    public:

        /**
         * @brief Pre-warm the in-memory LRU cache from the disk cache directory.
         * @return Number of entries successfully loaded into memory.
         */
        USize WarmCache () noexcept;

        /**
         * @brief Remove all .lumenbin + .meta files from the cache directory.
         * @return Number of asset entries removed.
         */
        USize ClearCache () noexcept;

        /**
         * @brief Remove cache entries older than InMaxAgeSeconds.
         * @param InMaxAgeSeconds Maximum age threshold in seconds.
         * @return Number of stale entries removed.
         */
        USize ClearStaleCache ( Float64 InMaxAgeSeconds ) noexcept;

    public:

        /**
         * @brief Compute a deterministic FNV-1a hash for a compile request.
         *
         * Called by TCompiler<> as a CRTP hook — must be non-const and static.
         *
         * @param InSource   Raw DLSL source text.
         * @param InRequest  Compile request parameters.
         * @return 64-bit FNV-1a hash.
         */
        [[nodiscard]] static FAssetHash ComputeHash ( FStringView InSource, const FLumenCompileRequest &InRequest ) noexcept;

        /**
         * @brief Post-compilation reflection hook required by TCompiler<>.
         *
         * Binary asset compilation requires no SPIR-V-style reflection;
         * this hook is a deliberate no-op that always succeeds.
         *
         * @param InOutCompiled  Compiled asset (unused).
         * @param OutErrorLog    Error log (unused).
         * @return Always true.
         */
        [[nodiscard]] static Bool TryReflect ( FCompiledLumenAsset &InOutCompiled, FString &OutErrorLog ) noexcept;

        /**
         * @brief Run the full Scanner -> Parser -> Serializer pipeline.
         *
         * @param InSource    Raw DLSL source text.
         * @param InRequest   Compile request parameters.
         * @param InHash      Pre-computed hash (stored in the result).
         * @param OutCompiled Populated on success.
         * @param OutErrorLog Human-readable error on failure.
         * @return True on success, false on any parse or serialization error.
         */
        [[nodiscard]] Bool
        CompileFresh ( FStringView InSource, const FLumenCompileRequest &InRequest, FAssetHash InHash, FCompiledLumenAsset &OutCompiled, FString &OutErrorLog );

    private:

        /**
         * @brief Find the target root block within a parsed document.
         *
         * @param InDocument  Parsed AST document.
         * @param InRequest   Compile request carrying the filter parameters.
         * @return Pointer to the matched block, or nullptr if not found.
         */
        [[nodiscard]] static const FDLSLRootBlock *FindTargetBlock ( const FDLSLDocument *InDocument, const FLumenCompileRequest &InRequest ) noexcept;

        /**
         * @brief Dispatch serialization to the correct serializer based on block type.
         * @param InBlock  The root block to serialize.
         * @return Binary blob on success, error string on failure.
         */
        [[nodiscard]] static TExpected<TVector<Byte>, FString> SerializeBlock ( const FDLSLRootBlock *InBlock );

        /**
         * @brief Resolve the EAssetType from a BlockType string view.
         * @param InBlockType  BlockType field of an FDLSLRootBlock.
         * @return Resolved EAssetType, or EAssetType::Unknown if unrecognised.
         */
        [[nodiscard]] static EAssetType::Type ResolveAssetType ( FStringView InBlockType ) noexcept;

    private:

        /** Scratch buffer used by the per-call FLinearAllocator for AST nodes. */
        TVector<Byte> ScratchBuffer;
    };

} // namespace Compiler

} // namespace LumenEngine
