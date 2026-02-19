/**
 * @file LaunchEngine.cpp
 * @brief Implementation of the entry points of the Engine
 */

#include "LaunchEngine.hpp"
#include "Container/Signal.hpp"
#include "ErrorCodes.hpp"
#include "LaunchEngineLoop.hpp"

namespace
{

static inline void EngineTick ()
{
    LumenEngine::GEngineLoop.Tick();
}

static inline bool bEngineRequestingExit ()
{
    return LumenEngine::GEngineLoop.ShouldExit();
}

static inline void EngineExit ()
{
    LumenEngine::GEngineLoop.Exit();
}

static void EngineTrapInterrupt ( const LumenEngine::ESystemSignal::Type )
{
    LumenEngine::FSignal::Raise( LumenEngine::ESystemSignal::Terminate );
}

static void EngineTrapTerminate ( const LumenEngine::ESystemSignal::Type )
{
    LumenEngine::GEngineLoop.RequestExit( "Termination signal received" );
}

static inline LumenEngine::Int32 EngineInit ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Interrupt, &EngineTrapInterrupt );
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Terminate, &EngineTrapTerminate );

    return LumenEngine::GEngineLoop.PreInit( Argc, Argv );
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

    while ( not bEngineRequestingExit() )
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
