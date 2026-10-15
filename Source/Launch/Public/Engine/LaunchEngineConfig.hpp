///
/// @file LaunchEngineConfig.hpp
/// @brief Launch-time configuration, filled during PreInit and immutable afterwards.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <HAL/PlatformTime.hpp>

#include <cmath>

namespace Lumen
{

namespace Launch
{
    ///
    /// @struct FLaunchEngineConfig
    /// @brief Plain data, trivially copyable. Passed by const reference to every module factory.
    ///
    struct FLaunchEngineConfig
    {
        /// @brief Fixed simulation step in seconds (`-fixedstep=`).
        Float64 FixedTimeStep = HAL::FPlatformTime::DefaultTickRate;

        /// @brief Upper bound on a single frame delta, protects against the "spiral of death" (`-maxframetime=`).
        Float64 MaxFrameTime = HAL::FPlatformTime::MaxTickRate;

        /// @brief Frame-rate cap, 0 = uncapped (`-fps=`).
        Float64 MaxFPS = 0.0;

        /// @brief Stop after N frames, 0 = run until an exit is requested (`-maxframes=`). Handy for CI and benchmarks.
        UInt64 MaxFrames = 0;

        /// @brief No window / no presentation (`-headless`).
        Bool bHeadless = false;
    };

    static_assert( std::is_trivially_copyable_v<FLaunchEngineConfig>, "Modules copy the config out of the factory call." );

} // namespace Launch

} // namespace Lumen
