///
/// @file Main.cpp
/// @brief App module entry-point.
///

#include <Engine/LaunchEngineLoop.hpp>

Lumen::Int32 main ( const Lumen::Int32 Argc, const Lumen::AnsiChar *Argv[] )
{
    return Lumen::Launch::FLaunchEngineLoop::GuardedMain( Argc, Argv );
}
