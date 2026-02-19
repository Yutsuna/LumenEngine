/**
 * @file LaunchEngine.cpp
 * @brief Implementation of the entry points of the Engine
 */

#include "LaunchEngine.hpp"
#include "ErrorCodes.hpp"

LumenEngine::Int32 LumenEngine::GuardedMain ( Int32 Argc, const AnsiChar *Argv[], FEngineLoop *GameInstance )
{
    ( void )Argc;
    ( void )Argv;
    ( void )GameInstance;
    return EErrorCode::Success;
}
