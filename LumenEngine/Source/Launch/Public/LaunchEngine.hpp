/**
 * @file LaunchEngine.hpp
 * @brief Declaration of the entry points of the Engine
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Launch
{

    /**
     * @brief Initializes the Engine and starts the main loop
     * @param Argc The number of command line arguments
     * @param Argv The command line arguments
     * @return The exit code of the application
     *
     * NOTE: See Core/Public/ErrorCodes.hpp for the list of possible return values
     */
    Int32 ClientInit ( const Int32 Argc, const AnsiChar *Argv[] );

    /**
     * @brief Advances the Engine by one tick, called every frame by the main loop
     * @param DeltaTime The time elapsed since the last tick, in seconds
     */
    void ClientTick ( const Float64 DeltaTime );

    /**
     * @brief Shuts down the client application, guaranteeing proper destruction before core systems terminate.
     */
    void ClientShutdown ();

} // namespace Launch

} // namespace LumenEngine