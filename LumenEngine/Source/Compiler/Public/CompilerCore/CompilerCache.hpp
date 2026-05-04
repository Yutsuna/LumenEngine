/**
 * @file CompilerCache.hpp
 * @brief Declarations for the generic thread-safe cache.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Cache/Cache.hpp"
#include "Cache/CachePolicy.hpp"

#include "Container/Optional.hpp"

#include "ShaderCompiler/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class TCompilerCache
     * @brief Manages disk and memory caching for specialized compiler outputs.
     */
    template <typename TTraits> class TCompilerCache final : public FNonCopyable, public FNonMovable
    {
    public:

        using TRequest  = typename TTraits::RequestType;
        using TCompiled = typename TTraits::CompiledType;
        using TMeta     = typename TTraits::MetaType;

    public:

        explicit TCompilerCache ( const typename TTraits::ConfigType &InConfig );

    public:

        /**
         * @brief Attempts to retrieve a compiled result from the cache.
         * @param InHash The hash of the source input.
         * @param InRequest The compilation request parameters.
         * @return An optional containing the compiled result if found and valid, or std::nullopt otherwise.
         */
        [[nodiscard]] TOptional<TCompiled> TryGet ( const FSourceHash InHash, const TRequest &InRequest ) noexcept;

        /**
         * @brief Stores a compiled result in the cache.
         * @param InHash The hash of the source input.
         * @param InRequest The compilation request parameters.
         * @param InCompiled The compiled result to store.
         */
        void Put ( const FSourceHash InHash, const TRequest &InRequest, const TCompiled &InCompiled ) noexcept;

        /**
         * @brief Invalidates a cached entry.
         * @param InHash The hash of the source input.
         * @param InRequest The compilation request parameters.
         */
        void Invalidate ( const FSourceHash InHash, const TRequest &InRequest ) noexcept;

        /**
         * @brief Clears all entries from the cache.
         * @return The number of entries cleared.
         */
        USize Clear () noexcept;

        /**
         * @brief Clears stale entries from the cache.
         * @param InMaxAgeSeconds The maximum age of entries to keep.
         * @return The number of entries cleared.
         */
        USize ClearStale ( const Float64 InMaxAgeSeconds ) noexcept;

    public:

        /**
         * @brief Retrieves the memory cache.
         * @return A reference to the memory cache.
         */
        [[nodiscard]] TCache<FSourceHash, TCompiled, Cache::FLruPolicy<FSourceHash, TCompiled>> &GetMemoryCache () noexcept
        {
            return MemoryCache;
        }

    private:

        const typename TTraits::ConfigType &Config;
        TCache<FSourceHash, TCompiled, Cache::FLruPolicy<FSourceHash, TCompiled>> MemoryCache;
    };

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/CompilerCache.inl"
