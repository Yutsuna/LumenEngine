///
/// @file LaunchEngineLoop.hpp
/// @brief Engine entry pipeline: PreInit -> Init -> Run, chained monadically.
///

#pragma once

#include <Core/CoreTypes.hpp>

#include <CLI/CommandLine.hpp>

#include <Engine/LaunchEngineConfig.hpp>

#include <Launch/LaunchError.hpp>
#include <Launch/ModuleStack.hpp>

namespace Lumen
{
namespace Launch
{
    ///
    /// @struct FPreInitState
    /// @brief Output -> Argument Parsed, Configuration Validated.
    ///
    struct FPreInitState
    {
        FCommandLine CommandLine;
        FLaunchEngineConfig Config;
    };

    ///
    /// @struct FPInitState
    /// @brief Output -> Modules Alive, destructing it tears them down in reverse order.
    ///
    struct FInitState
    {
        FLaunchEngineConfig Config;
        FModuleStack Modules;
    };

    ///
    /// @struct FRunSummary
    /// @brief Output -> Run.
    ///
    struct FRunSummary
    {
        UInt64 FrameCount      = 0;
        Float64 ElapsedSeconds = 0.0;
        Bool bInterrupted      = false;
    };

    ///
    /// @class FLaunchEngineLoop
    /// @brief Static facade of the launch pipeline.
    ///
    class FLaunchEngineLoop final
    {
    public:

        FLaunchEngineLoop () = delete;

    public:

        /// @brief Whole pipeline in one expression. Never throws, always returns a process exit code.
        [[nodiscard]] static Int32 GuardedMain ( Int32 InArgc, const AnsiChar *const *InArgv ) noexcept;

    public:

        /// @brief Process-level setup (signals, OOM policy, clock epoch) + configuration parsing / validation.
        [[nodiscard]] static TLaunchResult<FPreInitState> PreInit ( FCommandLine InCommandLine ) noexcept;

        /// @brief Starts every registered module. On failure, already-started modules are torn down by RAII.
        [[nodiscard]] static TLaunchResult<FInitState> Init ( FPreInitState &&InPreInitState ) noexcept;

        /// @brief Fixed-timestep main loop. Takes ownership of the state: modules die when the loop ends.
        [[nodiscard]] static TLaunchResult<FRunSummary> Run ( FInitState &&InInitState ) noexcept;

    public:

        /// @brief Thread-safe and async-signal-safe. The loop exits at the end of the current frame.
        static void RequestExit () noexcept;

        [[nodiscard]] static Bool IsExitRequested () noexcept;
    };
} // namespace Launch
} // namespace Lumen
