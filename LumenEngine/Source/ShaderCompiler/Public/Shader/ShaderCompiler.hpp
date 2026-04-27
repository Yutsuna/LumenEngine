/**
 * @file ShaderCompiler.hpp
 * @brief Runtime GLSL -> SPIR-V shader compiler with persistent cache
 */

#pragma once

#include "Container/Map.hpp"
#include "Container/String.hpp"
#include "CoreTypes.hpp"
#include "HAL/SharedMutex.hpp"
#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Internal
{

    class FShaderCompilerInternal;

}

/**
 * @class FShaderCompiler
 * @brief A runtime GLSL to SPIR-V shader compiler with a persistent cache.
 * @code
 *   FShaderCompilerConfig Config;
 *   Config.CacheDirectory = "Data/ShaderCache/";
 *
 *   FShaderCompiler Compiler( Config );
 *
 *   FShaderCompileRequest Request;
 *   Request.SourcePath = "Shaders/PBR.frag";
 *   Request.Stage = EShaderStage::Fragment;
 *   Request.Optimization = EShaderOptimizationLevel::Performance;
 *   Request.Macros = { { "USE_IBL", "1" } };
 *
 *   FShaderCompileResult Result = Compiler.CompileShader( Request );
 *
 *   if ( Result.IsSuccess() )
 *   {
 *       // Upload Result.Shader->SpirV to GPU
 *   }
 * @endcode
 */
class FShaderCompiler final : public FNonCopyable, public FNonMovable
{
public:

    explicit FShaderCompiler ( const FShaderCompilerConfig &InConfig ) noexcept;
    ~FShaderCompiler () noexcept;

public:

    /**
     * @brief Compile ( or load from cache ) a single shader stage
     * @thread thread-safe
     * @details
     *   - Read source file from disk
     *   - Compute hash ( source + request params )
     *   - Check cache for existing compiled shader
     *   - If cache hit, load SPIR-V from cache
     *  @param InRequest Shader compile request parameters
     *  @return Compilation result containing SPIR-V bytecode or error message
     */
    [[nodiscard]] FShaderCompileResult CompileShader ( const FShaderCompileRequest &InRequest ) noexcept;

    /**
     * @brief Compile from an in-memory GLSL source string instead of an I/O file
     * @param InSource GLSL source text
     * @param InRequest Shader compile request parameters
     * NOTE: InRequest.SourcePath is ONLY used for diagnostic message; the file is NOT read from disk
     */
    [[nodiscard]] FShaderCompileResult CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;

public:

    /**
     * @brief Pre-warm the in-memory cache from the cache directory.
     * @details Load all valid .spvcache pairs found on disk into an in-memory index
     * so that subsequent CompileShader calls avoid all disk I/O for cache hits.
     * @return Number of entries successfully loaded into the in-memory cache index
     */
    [[nodiscard]] USize WarmCache () noexcept;

    /**
     * @brief Invalidate the cache entry for one specific request
     * @details Both in-memory entry and the on-disk files (.spv + .spvcache) are removed.
     * The next CompileShader call for this request will recompile.
     * @param InRequest Same parameters used at compile-time.
     */
    void InvalidateCacheEntry ( const FShaderCompileRequest &InRequest ) noexcept;

    /**
     * @brief Remove all cached files from the cache directory.
     * @details In-memory index is also cleared
     * @return Number of file pairs removed
     */
    [[nodiscard]] USize ClearCache () noexcept;

    /**
     * @brief Remove cache netries whose .spv files are aloder than MaxAgeSeconds.
     * @details In-memory index is also cleared
     * @param MaxAgeSeconds Maximum age of cache files in seconds.
     * @return Number of file pairs removed
     */
    [[nodiscard]] USize ClearStaleCache ( double MaxAgeSeconds ) noexcept;

public:

    /** @return  Total number of cache hits since construction. */
    [[nodiscard]] UInt64 GetCacheHitCount () const noexcept;

    /** @return  Total number of full compiles triggered since construction. */
    [[nodiscard]] UInt64 GetCacheMissCount () const noexcept;

    /** @return  True if glslang was initialised successfully. */
    [[nodiscard]] Bool IsReady () const noexcept;

    /** @return  Reference to the current configuration. */
    [[nodiscard]] const FShaderCompilerConfig &GetConfig () const noexcept;

public:

    [[nodiscard]] static FSourceHash ComputeRequestHash ( const FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;

    /**
     * @brief Disassemble a SPIR-V blob to human-readable SPIR-V assembly text.
     * @param InSpirV SPIR-V bytecode to disassemble
     * @return SPIR-V assembly text on success; empty string on failure.
     */
    [[nodiscard]] static FString DisassembleSpirV ( const FSpirVBlob &InSpirV );

    /**
     * @brief Validate SPIR-V blob using SPIR-V validation rules.
     * @param InSpirV SPIR-V bytecode to validate
     * @return Empty string on success; validation error message on failure.
     */
    [[nodiscard]] static FString ValidateSpirV ( const FSpirVBlob &InSpirV );

private:

    FShaderCompilerConfig Config;

    TMap<FSourceHash, FCompiledShader> MemoryCache;
    FSharedMutex CacheMutex;

    TAtomic<UInt64> CacheHitCount{ 0 };
    TAtomic<UInt64> CacheMissCount{ 0 };

    Bool bInitialised = false;
};

} // namespace LumenEngine
