/**
 * @file LaunchEngineLoop.hpp
 * @brief Declaration of the main loop of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "ErrorCodes.hpp"

namespace LumenEngine
{

namespace Renderer
{
    class FRenderer;
}

class LUMEN_ENGINE_API FEngineLoop
{

public:

    FEngineLoop ()  = default;
    ~FEngineLoop () = default;

public:

    /**
     * @brief Initializes the engine loop, called before the main loop starts
     *
     * @return 0 if initialization succeeded, or an error code otherwise
     */
    [[nodiscard]] Int32 PreInit ( Int32 Argc, const AnsiChar *Argv[] );

    /**
     * @brief Initializes the engine loop, called after PreInit and before the main loop starts
     *
     * @return 0 if initialization succeeded, or an error code otherwise
     */
    [[nodiscard]] Int32 Init ();

    /** Advances the main loop */
    void Tick ();

    /** Shuts down the engine loop, called after the main loop ends */
    void Exit () const;

    /** Check if the application should exit */
    [[nodiscard]] Bool ShouldExit () const noexcept;

    /** Request the application to exit */
    void RequestExit ( const AnsiChar *Reason ) noexcept;

    /** @return The current frame index since the engine started */
    [[nodiscard]] UInt64 GetFrameIndex () const noexcept;

public:

    /** Initializes the application */
    [[nodiscard]] static EErrorCode::Type AppInit ();

    /** Shuts down the application */
    static void AppShutdown ();

private:

    void CalculateDeltaTime () noexcept;

private:

    Float64 TotalTickTime    = 0.0;
    Float64 LastTickTime     = 0.0;
    Float64 LastFrameSeconds = 0.0;

    UInt64 FrameIndex = 0;

    Bool bRequestingExit = false;
};

/**
 * Global engine loop instance.
 * NOTE: Non-const as it maintains the state of the running application.
 */
extern LUMEN_ENGINE_API FEngineLoop GEngineLoop;

} // namespace LumenEngine
