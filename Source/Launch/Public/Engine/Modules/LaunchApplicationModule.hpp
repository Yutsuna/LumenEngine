/*
- @file LaunchApplicationModule.hpp
- @brief Launch the application module
 */

#pragma once

#include "Engine/LaunchEngineConfig.hpp"
#include "Launch/LaunchError.hpp"
#include <Engine/LaunchEngineModule.hpp>
#include <Generic/GenericApplicationMessageHandler.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

class FGenericApplication;

namespace Launch
{

    /**
    - @class FLaunchMessageHandler
    - @brief Routes OS-level requests to the engine loop.
     */
    class FLaunchMessageHandler : public FGenericApplicationMessageHandler
    {
    public:

        void OnRequestExit () noexcept override;
        void OnQuit () noexcept override;
    };

    /**
    - @class FLaunchApplicationModule
    - @brief Launches the application module.
     */
    class FLaunchApplicationModule final : public IEngineModule
    {
    public:

        explicit FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept;
        ~FLaunchApplicationModule () noexcept override;

    public:

        [[nodiscard]] FStringView GetName () const noexcept override;
        [[nodiscard]] EModuleTraits GetTraits () const noexcept override;

        void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept override;

    private:

        TSharedPtr<FGenericApplication> Application;
        TSharedRef<FLaunchMessageHandler> MessageHandler;
    };

    /** @brief Accessor for modules created after this one. */
    [[nodiscard]] TSharedPtr<FGenericApplication> GetApplication () noexcept;

    /** @brief Factory registered in GetEngineModules. */
    [[nodiscard]] TLaunchResult<TUniquePtr<IEngineModule>> MakeApplicationModule ( const FLaunchEngineConfig &InConfig ) noexcept;

} // namespace Launch

} // namespace Lumen
