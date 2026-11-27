/**
 * @file Main.cpp
 * @brief App module entry-point.
 */

#include <Engine/LaunchEngine.hpp>

#include <LaunchExport.hpp>

/** INFO: Exported so an executable linking libLaunch needs no main of its own. */
LAUNCH_EXPORT Lumen::Int32 main ( const Lumen::Int32 Argc, const Lumen::AnsiChar *Argv[] )
{
    return Lumen::Launch::GuardedMain( Argc, Argv );
}
