/**
 * @file LaunchModuleStack.hpp
 * @brief Owns running modules, guarantees reverse-order teardown.
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
    class FLaunchModuleStack final : public FNonCopyable
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

        /** @brief Creates every module in order; stops at the first failure and keeps the ones already started. */
        [[nodiscard]] TLaunchResult<void> StartAll ( TSpan<const FModuleDescriptor> InDescriptors,
                                                     const FLaunchEngineConfig &InConfig ) noexcept;

        /** @brief Destroys every module, last started first. Idempotent. */
        void ShutdownAll () noexcept;

    public:

        /**
         * @brief Fixed tick, called at every fixed tick.
         * @param InFixedDeltaSeconds Fixed delta time in seconds.
         */
        void FixedTick ( Float64 InFixedDeltaSeconds ) noexcept;

        /**
         * @brief Tick at every frame.
         * @param InDeltaSeconds Delta time in seconds.
         * @param InAlpha Alpha value.
         */
        void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept;

        [[nodiscard]] USize Num () const noexcept
        {
            return Modules.size();
        }

    private:

        /** @brief Owning storage, in startup order. */
        TVector<TUniquePtr<IEngineModule>> Modules;

        /** @brief Non-owning dense dispatch lists. */
        TVector<IEngineModule *> FixedTickList;
        TVector<IEngineModule *> TickList;
    };
} // namespace Launch
} // namespace Lumen
