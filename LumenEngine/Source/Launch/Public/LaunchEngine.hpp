/**
 * @file LaunchEngine.hpp
 * @brief Declaration of the entry points of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "LaunchEngineLoops.hpp"

namespace LumenEngine
{

Int32 GuardedMain ( int Argc, const char *Argv[], FEngineLoop *GameInstance );

} // namespace LumenEngine

#define __LUMEN_ENGINE_GAME( GameClass )                                                                                                                                 \
    LumenEngine::Int32 main( LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )                                                                              \
    {                                                                                                                                                                    \
        GameClass *GameInstance = new GameClass();                                                                                                                       \
                                                                                                                                                                         \
        const LumenEngine::Int32 Result = LumenEngine::GuardedMain( Argc, Argv, GameInstance );                                                                          \
        delete GameInstance;                                                                                                                                             \
        return Result;                                                                                                                                                   \
    }

#define LUMEN_ENGINE_GAME( GameClass ) __LUMEN_ENGINE_GAME( GameClass )
