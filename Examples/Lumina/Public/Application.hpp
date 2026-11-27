/**
 * @file Application.hpp
 * @brief Application class for the Lumen game.
 */

#pragma once

#include <Engine/LaunchEngineModule.hpp>
#include <World/World.hpp>

namespace Lumen
{

class FLuminaApplication final : public Launch::IEngineModule
{

public:

    FLuminaApplication () noexcept;

private:

    TUniquePtr<FWorld> World;
};

} // namespace Lumen
