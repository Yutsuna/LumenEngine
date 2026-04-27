/**
 * @file ShaderCompiler.hpp
 * @brief Public facade for runtime GLSL -> SPIR-V shader compilation with persistent cache.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/Optional.hpp"
#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"

#include "NonCopyable.hpp"
#include "NonMovable.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Internal
{

    class FShaderCache;

}

/**
 * @class FShaderCompiler
 * @brief A runtime GLSL to SPIR-V shader compiler with a persistent cache.
 * @details Strictly adheres to SRP. All third-party dependencies are hidden.
 */
class LUMEN_ENGINE_API FShaderCompiler final : public FNonCopyable, public FNonMovable
{
public:

    explicit FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept;
    ~FShaderCompiler () noexcept;

public:

    /**
     * @brief Compile (or load from cache) a single shader stage from a file.
     * @param InRequest Shader compile request parameters.
     * @return Compilation result containing SPIR-V bytecode or error message.
     */
    [[nodiscard]] FShaderCompileResult CompileShader ( const FShaderCompileRequest &InRequest ) noexcept;

    /**
     * @brief Compile from an in-memory GLSL source string instead of an I/O file.
     * @param InSource GLSL source text.
     * @param InRequest Shader compile request parameters.
     * @return Compilation result containing SPIR-V bytecode or error message.
     */
    [[nodiscard]] FShaderCompileResult CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;

public:

    /**
     * @brief Pre-warm the in-memory cache from the disk cache directory.
     * @return Number of entries successfully loaded.
     */
    [[nodiscard]] USize WarmCache () noexcept;

    /**
     * @brief Invalidate the cache entry for one specific request.
     */
    void InvalidateCacheEntry ( const FShaderCompileRequest &InRequest ) noexcept;

    /**
     * @brief Remove all cached files from the cache directory.
     * @return Number of file pairs removed.
     */
    [[nodiscard]] USize ClearCache () noexcept;

    /**
     * @brief Remove cache entries whose .spv files are older than MaxAgeSeconds.
     * @return Number of file pairs removed.
     */
    [[nodiscard]] USize ClearStaleCache ( double MaxAgeSeconds ) noexcept;

public:

    [[nodiscard]] UInt64 GetCacheHitCount () const noexcept;
    [[nodiscard]] UInt64 GetCacheMissCount () const noexcept;
    [[nodiscard]] Bool IsReady () const noexcept;
    [[nodiscard]] const FShaderCompilerConfig &GetConfig () const noexcept;

public:

    [[nodiscard]] static FSourceHash ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;
    [[nodiscard]] static FString DisassembleSpirV ( const FSpirVBlob &InSpirV ) noexcept;
    [[nodiscard]] static FString ValidateSpirV ( const FSpirVBlob &InSpirV ) noexcept;

private:

    FShaderCompilerConfig Config;
    TUniquePtr<Internal::FShaderCache> Cache;
    Bool bInitialised = false;
};

} // namespace LumenEngine