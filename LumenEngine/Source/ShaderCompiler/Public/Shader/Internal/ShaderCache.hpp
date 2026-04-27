/**
 * @file ShaderCache.hpp
 */
#pragma once

#include "Container/Map.hpp"
#include "HAL/SharedMutex.hpp"
#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Internal
{

    /**
     * @class FShaderCache
     * @brief Manages caching of compiled shaders on disk and in memory.
     * @details
     *   - Cache key is a hash of the shader source and compile request parameters.
     *   - On cache hit, SPIR-V bytecode is loaded from disk and returned.
     *   - On cache miss, shader is compiled, stored in cache, and returned.
     *   - Cache files are stored in a configurable directory with a structured naming scheme.
     *   - Thread-safe for concurrent access from multiple threads.
     *   - Provides methods for cache management (warming, invalidation, clearing).
     *   - Tracks cache hit/miss statistics.
     */
    class FShaderCache final : public FNonCopyable, public FNonMovable
    {
    public:

        explicit FShaderCache ( const FShaderCompilerConfig &InConfig ) noexcept;

    public:

        [[nodiscard]] FSourceHash ComputeHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) const noexcept;

        [[nodiscard]] TOptional<FCompiledShader> TryGet ( FSourceHash InHash, EShaderStage::Type InStage, FStringView InEntryPoint ) noexcept;

    public:

        void Put ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept;

        USize WarmUp () noexcept;
        void Invalidate ( FSourceHash InHash, EShaderStage::Type InStage ) noexcept;
        USize Clear () noexcept;
        USize ClearStale ( double MaxAgeSeconds ) noexcept;

    public:

        void RecordHit () noexcept;
        void RecordMiss () noexcept;
        [[nodiscard]] UInt64 GetHits () const noexcept;
        [[nodiscard]] UInt64 GetMisses () const noexcept;

    private:

        [[nodiscard]] FString BuildCachePath ( FSourceHash InHash, EShaderStage::Type InStage, const AnsiChar *InExt ) const;

        const FShaderCompilerConfig &Config;
        TMap<FSourceHash, FCompiledShader> MemoryCache;
        FSharedMutex CacheMutex;

        TAtomic<UInt64> CacheHitCount{ 0 };
        TAtomic<UInt64> CacheMissCount{ 0 };
    };
} // namespace Internal

} // namespace LumenEngine