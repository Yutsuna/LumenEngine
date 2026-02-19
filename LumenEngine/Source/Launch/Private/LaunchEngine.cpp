/**
 * @file LaunchEngine.cpp
 * @brief Implementation of the entry points of the Engine
 */

#include "LaunchEngine.hpp"
#include "ErrorCodes.hpp"
#include "LaunchEngineLoop.hpp"

namespace
{

static inline LumenEngine::Int32 EngineInit ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    return LumenEngine::GEngineLoop.PreInit( Argc, Argv );
}

static inline void EngineTick ()
{
    LumenEngine::GEngineLoop.Tick();
}

static inline bool EngineRequestingExit ()
{
    return LumenEngine::GEngineLoop.ShouldExit();
}

static inline void EngineExit ()
{
    LumenEngine::GEngineLoop.Exit();
}

} // namespace

LumenEngine::Int32 LumenEngine::Launch::GuardedMain ( const Int32 Argc, const AnsiChar *Argv[] )
{
    const Int32 ErrorCode = EngineInit( Argc, Argv );
    struct FScopeCleanupGuard
    {
        ~FScopeCleanupGuard ()
        {
            EngineExit();
        }
    } ScopeCleanupGuard;

    if ( ErrorCode != EErrorCode::Success )
    {
        return ErrorCode;
    }

    while ( not EngineRequestingExit() )
    {
        EngineTick();
    }

    return ErrorCode;
}

#define Main main

LumenEngine::Int32 Main ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    return LumenEngine::Launch::GuardedMain( Argc, Argv );
}

#undef Main
