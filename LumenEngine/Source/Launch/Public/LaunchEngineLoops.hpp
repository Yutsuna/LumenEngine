/**
 * @file LaunchEngineLoops.hpp
 * @brief Declaration of the main loops of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

class LUMEN_ENGINE_API FEngineLoop
{

public:

    FEngineLoop ()          = default;
    virtual ~FEngineLoop () = default;

public:

    /**
     * @brief Initializes the engine loop, called before the main loop starts
     *
     * @return 0 if initialization succeeded, or an error code otherwise
     */
    virtual Int32 PreInit ( Int32 Argc, const AnsiChar *Argv[] );

    /**
     * @brief Initializes the engine loop, called after PreInit and before the main loop starts
     *
     * @return 0 if initialization succeeded, or an error code otherwise
     */
    virtual Int32 Init ();

    /** Advances the main loop */
    virtual void Tick ();

public:

    /** Initializes the application */
    static bool AppInit ();

    /** Shuts down the application */
    static void AppShutdown ();

private:

    void CalculateDeltaTime () noexcept;

protected:

    Float64 TotalTickTime;
    Float64 LastTickTime;

private:

    Float64 LastFrameSeconds;
};

} // namespace LumenEngine
