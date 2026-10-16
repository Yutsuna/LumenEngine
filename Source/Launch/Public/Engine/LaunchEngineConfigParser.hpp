///
/// @file LaunchEngineConfigParser.hpp
/// @brief Parse the Command Line then Validate the configuration.
///

#pragma once

#include <CLI/CommandLine.hpp>

#include <Engine/LaunchEngineConfig.hpp>
#include <Launch/LaunchError.hpp>

namespace Lumen
{

namespace Launch
{

    /// @brief Validate the Engine Configuration
    /// @return The config itself on success, InvalidConfiguration otherwise.
    [[nodiscard]] TLaunchResult<FLaunchEngineConfig> ValidateEngineConfig ( const FLaunchEngineConfig &InConfig ) noexcept;

    /// @brief Parse the Engine Configuration
    /// @return The config itself on success, InvalidArgument for a malformed value, InvalidConfiguration for an incoherent set of
    /// values.
    [[nodiscard]] TLaunchResult<FLaunchEngineConfig> ParseEngineConfig ( const FCommandLine &InCommandLine ) noexcept;

} // namespace Launch

} // namespace Lumen
