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

        [[nodiscard]] TOptional<TCompiled> TryGet ( const FSourceHash InHash, const TRequest &InRequest ) noexcept;
        void Put ( const FSourceHash InHash, const TRequest &InRequest, const TCompiled &InCompiled ) noexcept;
        void Invalidate ( const FSourceHash InHash, const TRequest &InRequest ) noexcept;
        USize Clear () noexcept;
        USize ClearStale ( const Float64 InMaxAgeSeconds ) noexcept;

    public:

        [[nodiscard]] TCache<FSourceHash, TCompiled, Cache::FLruPolicy<FSourceHash, TCompiled>> &GetMemoryCache () noexcept;

    private:

        const typename TTraits::ConfigType &Config;
        TCache<FSourceHash, TCompiled, Cache::FLruPolicy<FSourceHash, TCompiled>> MemoryCache;
    };

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/CompilerCache.inl"
