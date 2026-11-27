/**
 * @file LaunchApplicationModule.hpp
 * @brief Launch the application module.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericApplication;

namespace Launch
{

    /**
     * @class FLaunchMessageHandler
     * @brief Routes OS-level requests to the engine loop.
     */
    class FLaunchMessageHandler final : public FGenericApplicationMessageHandler
    {
    public:

        void OnRequestExit () noexcept override;
        void OnQuit () noexcept override;
    };

    /**
     * @class FLaunchApplicationModule
     * @brief Launches and owns the platform application instance.
     */
    class LAUNCH_EXPORT FLaunchApplicationModule final : public IEngineModule
    {
    public:

        static constexpr ETickPhase TickPhase = ETickPhase::Input;

    public:

        [[nodiscard]] static Bool IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept;

        [[nodiscard]] static TLaunchResult<TUniquePtr<FLaunchApplicationModule>>
        Create ( const TModuleContext<FLaunchApplicationModule> &InContext ) noexcept;

    public:

        explicit FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept;
        ~FLaunchApplicationModule () noexcept override;

    public:

        [[nodiscard]] EModuleTraits GetTraits () const noexcept override;

        void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept override;

        [[nodiscard]] const TSharedPtr<FGenericApplication> &GetApplication () const noexcept;

    private:

        TSharedPtr<FGenericApplication> Application;
        TSharedRef<FLaunchMessageHandler> MessageHandler;
    };

} // namespace Launch

} // namespace Lumen
