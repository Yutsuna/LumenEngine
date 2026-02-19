/**
 * @file LaunchEngine.hpp
 * @brief Declaration of the entry points of the Engine
 */

#pragma once

#include "CoreTypes.hpp"
#include "IGameInstance.hpp"

namespace LumenEngine
{

Int32 GuardedMain ( int argc, const char *argv[], class IGameInstance *GameInstance );

#define __LUMEN_ENGINE_GAME( GameClass )                                                                                                                                 \
    Int32 Main( Int32 Argc, const AnsiChar *Argv[] )                                                                                                                     \
    {                                                                                                                                                                    \
        GameClass *GameInstance = new GameClass();                                                                                                                       \
                                                                                                                                                                         \
        const Int32 Result = GuardedMain( Argc, Argv, GameInstance );                                                                                                    \
        delete GameInstance;                                                                                                                                             \
        return Result;                                                                                                                                                   \
    }

} // namespace LumenEngine

#define LUMEN_ENGINE_GAME( GameClass ) __LUMEN_ENGINE_GAME( GameClass )
