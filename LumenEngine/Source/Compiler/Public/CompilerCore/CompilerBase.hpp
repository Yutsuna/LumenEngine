/**
 * @file CompilerBase.hpp
 * @brief Declarations for the CRTP Base class.
 */

#pragma once

#include "CoreTypes.hpp"

#include "CompilerCore/CompilerCache.hpp"

#include "Container/UniquePtr.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class TCompiler
     * @brief Orchestrator for hashing, caching, and running specific compiler backends.
     */
    template <typename TTraits, typename TDerived> class TCompiler : public FNonCopyable, public FNonMovable
    {
    public:

        using TRequest  = typename TTraits::RequestType;
        using TResult   = typename TTraits::ResultType;
        using TCompiled = typename TTraits::CompiledType;

    public:

        explicit TCompiler ( typename TTraits::ConfigType InConfig );
        virtual ~TCompiler () noexcept = default;

    public:

        [[nodiscard]] TResult Compile ( const TRequest &InRequest ) noexcept;
        [[nodiscard]] TResult CompileFromSource ( const FStringView InSource, const TRequest &InRequest ) noexcept;
        void InvalidateCacheEntry ( const TRequest &InRequest ) noexcept;

    public:

        [[nodiscard]] USize ClearCache () noexcept;
        [[nodiscard]] USize ClearStaleCache ( const Float64 InMaxAgeSeconds ) noexcept;

    public:

        [[nodiscard]] UInt64 GetCacheHitCount () const noexcept;
        [[nodiscard]] UInt64 GetCacheMissCount () const noexcept;
        [[nodiscard]] const typename TTraits::ConfigType &GetConfig () const noexcept;

    protected:

        typename TTraits::ConfigType Config;
        TUniquePtr<TCompilerCache<TTraits>> Cache;
    };

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/CompilerBase.inl"
