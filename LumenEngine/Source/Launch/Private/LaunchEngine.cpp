/**
 * @file LaunchEngine.cpp
 * @brief Implementation of the entry points of the Engine
 */

#include "LaunchEngine.hpp"
#include "Container/Signal.hpp"
#include "ErrorCodes.hpp"
#include "LaunchEngineLoop.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

namespace
{

static LumenEngine::FLogCategory LogLaunch( "LogLaunch" );

static inline void EngineTick ()
{
    LumenEngine::GEngineLoop.Tick();
}

static inline LumenEngine::Bool bEngineRequestingExit ()
{
    return LumenEngine::GEngineLoop.ShouldExit();
}

static inline void EngineExit ()
{
    LumenEngine::GEngineLoop.AppShutdown();
    LumenEngine::GEngineLoop.Exit();
    LumenEngine::FLogger::GetInstance().Shutdown();
}

static void EngineTrapInterrupt ( const LumenEngine::ESystemSignal::Type )
{
    LumenEngine::FLogger::Flush( "\r" );
    LUMEN_LOG_INFO( LogLaunch, "Interrupt signal received. Requesting engine termination..." );
    LumenEngine::FSignal::Raise( LumenEngine::ESystemSignal::Terminate );
}

static void EngineTrapTerminate ( const LumenEngine::ESystemSignal::Type )
{
    LumenEngine::FLogger::Flush( "\r" );
    LUMEN_LOG_INFO( LogLaunch, "Termination signal received. Requesting engine termination..." );
    LumenEngine::GEngineLoop.RequestExit( "Termination signal received" );
}

static inline LumenEngine::Int32 EngineInit ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Interrupt, &EngineTrapInterrupt );
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Terminate, &EngineTrapTerminate );

    const LumenEngine::Int32 ErrorCode = LumenEngine::GEngineLoop.PreInit( Argc, Argv );

    if ( ErrorCode != LumenEngine::EErrorCode::Success )
    {
        return ErrorCode;
    }

    return LumenEngine::GEngineLoop.AppInit();
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

LumenEngine::Int32 main ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    return LumenEngine::Launch::GuardedMain( Argc, Argv );
}
