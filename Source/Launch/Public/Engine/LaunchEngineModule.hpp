/**
 * @file LaunchEngineModule.hpp
 * @brief Contract between the launcher (composition root) and the engine modules.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>

#include <LaunchExport.hpp>

#include <CLI/CommandLine.hpp>
#include <Engine/LaunchEngineConfig.hpp>
#include <Launch/LaunchError.hpp>

#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>
#include <Templates/Vector.hpp>

#include <String/StringView.hpp>

namespace Lumen
{

namespace Launch
{

    class IEngineModule;
    class FLaunchModuleStack;

    /** @brief Stable compile-time module or service identifier (safe across shared library boundaries). */
    using FModuleId = UInt64;

    /**
     * @enum EModuleTraits
     * @brief Declares which per-frame hooks a module actually implements.
     */
    enum class EModuleTraits : UInt8
    {
        None      = 0,
        FixedTick = 1 << 0,
        Tick      = 1 << 1,
    };

    LUMEN_ENUM_FLAGS( EModuleTraits )

    /**
     * @enum ETickPhase
     * @brief Coarse per-frame Tick ordering bucket (independent from startup/teardown DAG order).
     */
    enum class ETickPhase : UInt8
    {
        Input = 0,
        PreUpdate,
        Update,
        PostUpdate,
        PreRender,
        Render,
        Present,
    };

    /**
     * @struct TModuleList
     * @brief Declarative compile-time type list for `FRequires`, `FOptionalRequires`, `FBefore`, `FProvides`.
     */
    template <typename... Ts> struct TModuleList
    {
    };

    /**
     * @struct FModuleDep
     * @brief Lightweight static metadata describing a dependency or ordering edge.
     */
    struct FModuleDep
    {
        FModuleId Id = 0;
        FStringView Name;
        FStringView QualifiedName;
    };

    /**
     * @struct FServiceProvider
     * @brief Describes a concrete module or abstract interface exposed by a module, with safe multiple-inheritance upcast.
     */
    struct FServiceProvider
    {
        FModuleId Id = 0;
        FStringView Name;
        FStringView QualifiedName;
        void *( *Upcast )( IEngineModule * ) noexcept = nullptr;
    };

    /**
     * @class FModuleContext
     * @brief Runtime initialization context passed to module factories.
     */
    class LAUNCH_EXPORT FModuleContext final
    {
    public:

        struct FServiceEntry
        {
            FModuleId Id = 0;
            FStringView Name;
            FStringView QualifiedName;
            void *Instance = nullptr;
        };

    public:

        FModuleContext ( const FLaunchEngineConfig &InConfig,
                         const FCommandLine &InCommandLine,
                         TSpan<const FServiceEntry> InServices ) noexcept;

    public:

        [[nodiscard]] void *FindRaw ( FModuleId InId ) const noexcept;
        [[nodiscard]] const FLaunchEngineConfig &GetConfig () const noexcept;
        [[nodiscard]] const FCommandLine &GetCommandLine () const noexcept;

    private:

        const FLaunchEngineConfig &Config;
        const FCommandLine &CommandLine;
        TSpan<const FServiceEntry> Services;
    };

    /**
     * @class TModuleContext
     * @brief Compile-time checked view over FModuleContext enforcing declarative dependency access.
     */
    template <typename TModule> class TModuleContext final
    {
    public:

        explicit TModuleContext ( const FModuleContext &InInner ) noexcept;

    public:

        /** @brief Returns a guaranteed non-null reference to a dependency declared in `TModule::FRequires`. */
        template <typename TDep> [[nodiscard]] TDep &Get () const noexcept;

        /** @brief Returns a nullable pointer to a dependency declared in `FRequires` or `FOptionalRequires`. */
        template <typename TDep> [[nodiscard]] TDep *Find () const noexcept;

        [[nodiscard]] const FLaunchEngineConfig &GetConfig () const noexcept;
        [[nodiscard]] const FCommandLine &GetCommandLine () const noexcept;

    private:

        const FModuleContext &Inner;
    };

    /**
     * @class IEngineModule
     * @brief A subsystem whose lifetime is bound to its C++ lifetime.
     */
    class LAUNCH_EXPORT IEngineModule
    {
    public:

        virtual ~IEngineModule () noexcept = default;

    public:

        /** @return The registered name of the module. */
        [[nodiscard]] virtual FStringView GetName () const noexcept;

        /** @return The traits of the module. */
        [[nodiscard]] virtual EModuleTraits GetTraits () const noexcept;

        /**
         * @brief Fixed Tick simulation hook (gameplay, physics...).
         * @param InFixedDeltaSeconds The fixed delta time in seconds.
         */
        virtual void FixedTick ( Float64 InFixedDeltaSeconds ) noexcept;

        /**
         * @brief Tick once per frame.
         * @param InDeltaSeconds The delta time in seconds.
         * @param InAlpha The interpolation factor value.
         */
        virtual void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept;

    private:

        friend class FLaunchModuleStack;

        FStringView ModuleName = "Unknown";
    };

    using FModuleResult = TLaunchResult<TUniquePtr<IEngineModule>>;

    /**
     * @struct FModuleDescriptor
     * @brief Static constinit descriptor registered via `LUMEN_REGISTER_MODULE`.
     */
    struct FModuleDescriptor
    {
        FModuleId Id = 0;
        FStringView Name;
        FStringView QualifiedName;

        TSpan<const FServiceProvider> Provides;
        TSpan<const FModuleDep> Requires;
        TSpan<const FModuleDep> OptionalRequires;
        TSpan<const FModuleDep> Before;
        TSpan<const FStringView> CliOptions;

        ETickPhase TickPhase = ETickPhase::Update;
        Int32 LoadPriority   = 0;

        Bool ( *IsEnabled )( const FLaunchEngineConfig &, const FCommandLine & ) noexcept = nullptr;
        FModuleResult ( *Create )( const FModuleContext & ) noexcept                      = nullptr;

        FModuleDescriptor *Next = nullptr;
        Bool bRegistered        = false;
    };

    /**
     * @brief Appends a module descriptor to the intrusive static registry (SIOF-safe, zero heap allocation).
     * @details Intended for `LUMEN_REGISTER_MODULE` or dynamic plugin entry points.
     */
    LAUNCH_EXPORT void RegisterModuleDescriptor ( FModuleDescriptor &InDescriptor ) noexcept;

    /** @brief Head of the intrusive registered module descriptor list. */
    [[nodiscard]] LAUNCH_EXPORT const FModuleDescriptor *GetRegisteredModuleHead () noexcept;

    /** @brief Collects all registered module descriptors into a contiguous vector. */
    [[nodiscard]] LAUNCH_EXPORT TVector<const FModuleDescriptor *> CollectRegisteredModules () noexcept;

} // namespace Launch

} // namespace Lumen

/**
 * @brief Registers a module descriptor at static initialization time (SIOF-safe, zero heap allocation).
 * @param ModuleType Concrete class derived from `Lumen::Launch::IEngineModule`.
 */
#define LUMEN_REGISTER_MODULE( ModuleType ) LUMEN_REGISTER_MODULE_DETAIL_IMPL( ModuleType, __COUNTER__ )


#include "Inline/LaunchEngineModule.inl"
