/**
 * @file Application.hpp
 * @brief Application class for the Lumen game.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>

#include <LogLumina.hpp>
#include <World/World.hpp>

namespace Lumen
{

/**
 * @class FLuminaApplication
 * @brief Game module: owns the world and installs the game message handler on the platform application.
 */
class FLuminaApplication final : public Launch::IEngineModule
{
public:

    /**
     * @brief Optional: FLaunchApplicationModule is disabled in headless mode. The stack still orders
     *        startup/teardown correctly when it is active.
     */
    using FOptionalRequires = Launch::TModuleList<Launch::FLaunchApplicationModule>;

public:

    explicit FLuminaApplication ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept;

private:

    TUniquePtr<FWorld> World;
    Launch::FScopedMessageHandler MessageHandlerGuard;
};

} // namespace Lumen
