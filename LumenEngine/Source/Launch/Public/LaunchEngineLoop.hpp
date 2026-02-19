/**
 * @file LaunchEngineLoop.hpp
 * @brief Declaration of the main loop of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "Generic/GenericApplication.hpp"

namespace LumenEngine
{

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
    Int32 PreInit ( Int32 Argc, const AnsiChar *Argv[] );

    /**
     * @brief Initializes the engine loop, called after PreInit and before the main loop starts
     *
     * @return 0 if initialization succeeded, or an error code otherwise
     */
    Int32 Init ();

    /** Advances the main loop */
    void Tick ();

    /** Shuts down the engine loop, called after the main loop ends */
    void Exit ();

    /** Check if the application should exit */
    Bool ShouldExit () const;

    /** Requests the application to exit */
    void RequestExit ( const AnsiChar *Reason );

public:

    /** Initializes the application */
    static bool AppInit ();

    /** Shuts down the application */
    static void AppShutdown ();

private:

    void CalculateDeltaTime () noexcept;

private:

    Float64 TotalTickTime;
    Float64 LastTickTime;
    Float64 LastFrameSeconds;

    Bool bRequestingExit;

private:

    static TSharedPtr<FGenericApplication> GPlatformApplication;
};

extern LUMEN_ENGINE_API FEngineLoop GEngineLoop;

} // namespace LumenEngine
