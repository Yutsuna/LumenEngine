/**
 * @file LaunchEngine.cpp
 * @brief Implementation of the entry points of the Engine
 */

#include "GameApplication.hpp"

#include "Container/Signal.hpp"
#include "ErrorCodes.hpp"
#include "LaunchEngineLoop.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

namespace
{

const LumenEngine::FLogCategory LogLaunch( "LogLaunch" );

inline void EngineTick ()
{
    LumenEngine::GEngineLoop.Tick();
}

inline LumenEngine::Bool EngineRequestingExit ()
{
    return LumenEngine::GEngineLoop.ShouldExit();
}

inline void EngineExit ()
{
    LumenEngine::Launch::GetGameApplication().Shutdown();
    LumenEngine::GEngineLoop.Exit();
    LumenEngine::FSignal::Reset();
    LumenEngine::FLogger::GetInstance().Shutdown();
}

void EngineTrapInterrupt ( const LumenEngine::ESystemSignal::Type __attribute__( ( unused ) ) SignalType )
{
    LumenEngine::FLogger::Flush( "\rInterrupt signal trapped: exiting gracefully..." );
    LumenEngine::GEngineLoop.RequestExitAsyncSafe();
}

void EngineTrapTerminate ( const LumenEngine::ESystemSignal::Type __attribute__( ( unused ) ) SignalType )
{
    LumenEngine::FLogger::Flush( "\rTerminate signal trapped: exiting gracefully..." );
    LumenEngine::GEngineLoop.RequestExitAsyncSafe();
}

inline LumenEngine::Int32 EngineInit ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Interrupt, &EngineTrapInterrupt );
    LumenEngine::FSignal::Bind( LumenEngine::ESystemSignal::Terminate, &EngineTrapTerminate );
    LumenEngine::Int32 ErrorCode = LumenEngine::GEngineLoop.PreInit( Argc, Argv );

    if ( ErrorCode != LumenEngine::EErrorCode::Success )
    {
        return ErrorCode;
    }

    ErrorCode = LumenEngine::GEngineLoop.Init();
    if ( ErrorCode != LumenEngine::EErrorCode::Success )
    {
        return ErrorCode;
    }

    return LumenEngine::Launch::GetGameApplication().Initialize( Argc, Argv );
}

} // namespace

LumenEngine::Int32 main ( const LumenEngine::Int32 Argc, const LumenEngine::AnsiChar *Argv[] )
{
    const LumenEngine::Int32 ErrorCode = EngineInit( Argc, Argv );
    struct FScopeCleanupGuard
    {
        ~FScopeCleanupGuard ()
        {
            EngineExit();
        }
    } ScopeCleanupGuard;

    if ( ErrorCode != LumenEngine::EErrorCode::Success )
    {
        return ErrorCode;
    }

    while ( not EngineRequestingExit() )
    {
        EngineTick();
    }

    return ErrorCode;
}
