/**
 * @file LaunchModuleStack.hpp
 * @brief Owns running modules, resolves declarative DAG dependencies, guarantees reverse-order teardown.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Engine/LaunchEngineModule.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace Launch
{

    /**
     * @class FLaunchModuleStack
     * @brief Move-only owner of the live modules. Last started, first destroyed.
     */
    class LAUNCH_EXPORT FLaunchModuleStack final : public FNonCopyable
    {
    public:

        /** @brief Default constructor. */
        FLaunchModuleStack () noexcept = default;

    public:

        /** @brief Move constructor. */
        FLaunchModuleStack ( FLaunchModuleStack &&Other ) noexcept;

        /** @brief Move assignment operator. */
        FLaunchModuleStack &operator=( FLaunchModuleStack && ) = delete;

    public:

        /** @brief Destructor. */
        ~FLaunchModuleStack () noexcept;

    public:

        /** @brief Validates DAG dependencies and starts all enabled registered modules. */
        [[nodiscard]] TLaunchResult<void> StartAll ( const FLaunchEngineConfig &InConfig,
                                                     const FCommandLine &InCommandLine = {} ) noexcept;

        /** @brief Validates DAG dependencies and starts all enabled descriptors from the given span (testable). */
        [[nodiscard]] TLaunchResult<void> StartAll ( TSpan<const FModuleDescriptor *const> InDescriptors,
                                                     const FLaunchEngineConfig &InConfig,
                                                     const FCommandLine &InCommandLine = {} ) noexcept;

        /** @brief Destroys every module, last started first. Idempotent. */
        void ShutdownAll () noexcept;

    public:

        /**
         * @brief Fixed tick, called at every fixed tick in topological order.
         * @param InFixedDeltaSeconds Fixed delta time in seconds.
         */
        void FixedTick ( Float64 InFixedDeltaSeconds ) noexcept;

        /**
         * @brief Tick at every frame, sorted by `(TickPhase, TopologicalOrder)`.
         * @param InDeltaSeconds Delta time in seconds.
         * @param InAlpha Alpha value.
         */
        void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept;

        /** @return Number of running modules. */
        [[nodiscard]] USize Num () const noexcept;

        /** @return Pointer to a running module or provided service interface, or nullptr if absent. */
        template <typename TService> [[nodiscard]] TService *FindService () const noexcept;

        /** @return Untyped pointer to a registered service by ID, or nullptr if absent. */
        [[nodiscard]] void *FindServiceRaw ( FModuleId InId ) const noexcept;

    private:

        /** @brief Owning storage, in startup order. */
        TVector<TUniquePtr<IEngineModule>> Modules;

        /** @brief Non-owning resolved service map (module types + `FProvides` interfaces). */
        TVector<FModuleContext::FServiceEntry> Services;

        /** @brief Non-owning dense dispatch lists. */
        TVector<IEngineModule *> FixedTickList;
        TVector<IEngineModule *> TickList;
    };

} // namespace Launch

} // namespace Lumen

#include "Inline/LaunchModuleStack.inl"
