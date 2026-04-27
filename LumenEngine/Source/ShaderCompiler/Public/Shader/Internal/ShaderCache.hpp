/**
 * @file ShaderCache.hpp
 * @brief Thread-safe persistent cache subsystem.
 */

#pragma once

#include "Container/Map.hpp"
#include "Container/Optional.hpp"

#include "HAL/SharedMutex.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Internal
{

    /**
     * @class FShaderCache
     * @brief Manages caching of compiled shaders on disk and in memory.
     */
    class FShaderCache final : public FNonCopyable, public FNonMovable
    {
    public:

        explicit FShaderCache ( const FShaderCompilerConfig &InConfig ) noexcept;

    public:

        /**
         * @brief Attempts to retrieve a compiled shader from the cache.
         * @param InHash Source hash.
         * @param InStage Shader stage.
         * @param InEntryPoint Entry point name.
         * @return Optional compiled shader if found, otherwise nullopt.
         */
        [[nodiscard]] TOptional<FCompiledShader> TryGet ( FSourceHash InHash, EShaderStage::Type InStage, FStringView InEntryPoint ) noexcept;

        /**
         * @brief Stores a compiled shader in the cache.
         * @param InHash Source hash.
         * @param InRequest Shader compile request.
         * @param InCompiled Compiled shader.
         */
        void Put ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept;

    public:

        /**
         * @brief Warms up the cache by loading pre-compiled shaders.
         * @return Number of shaders loaded.
         */
        USize WarmUp () noexcept;

        /**
         * @brief Invalidates a cached shader.
         * @param InHash Source hash.
         * @param InStage Shader stage.
         */
        void Invalidate ( FSourceHash InHash, EShaderStage::Type InStage ) noexcept;

        /**
         * @brief Clears the entire cache.
         * @return Number of shaders cleared.
         */
        USize Clear () noexcept;

        /**
         * @brief Clears stale shaders from the cache.
         * @param MaxAgeSeconds Maximum age in seconds.
         * @return Number of stale shaders cleared.
         */
        USize ClearStale ( double MaxAgeSeconds ) noexcept;

    public:

        /**
         * @brief Records a cache hit.
         */
        void RecordHit () noexcept;

        /**
         * @brief Records a cache miss.
         */
        void RecordMiss () noexcept;

        /**
         * @brief Gets the number of cache hits.
         * @return Number of cache hits.
         */
        [[nodiscard]] UInt64 GetHits () const noexcept;

        /**
         * @brief Gets the number of cache misses.
         * @return Number of cache misses.
         */
        [[nodiscard]] UInt64 GetMisses () const noexcept;

    private:

        [[nodiscard]] FString BuildCachePath ( FSourceHash InHash, EShaderStage::Type InStage, const AnsiChar *InExt ) const;

    private:

        const FShaderCompilerConfig &Config;

        TMap<FSourceHash, FCompiledShader> MemoryCache;
        FSharedMutex CacheMutex;

        TAtomic<UInt64> CacheHitCount{ 0 };
        TAtomic<UInt64> CacheMissCount{ 0 };
    };

} // namespace Internal

} // namespace LumenEngine