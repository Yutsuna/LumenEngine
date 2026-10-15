///
/// @file Main.cpp
/// @brief App module entry-point.
///

#include <Engine/LaunchEngine.hpp>

Lumen::Int32 main ( const Lumen::Int32 Argc, const Lumen::AnsiChar *Argv[] )
{
    return Lumen::Launch::GuardedMain( Argc, Argv );
}
