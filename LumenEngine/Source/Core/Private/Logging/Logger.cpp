/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class.
 */

#include "Logging/Logger.hpp"
#include "Container/Signal.hpp"
#include "CoreTypes.hpp"
#include "HAL/PlatformTime.hpp"

#include <print>

LumenEngine::FLogger &LumenEngine::FLogger::GetInstance ()
{
    static FLogger Instance;

    return Instance;
}

void LumenEngine::FLogger::Initialize ()
{
    if ( not WorkerThread.joinable() )
    {
        WorkerThread = std::jthread( [this] ( std::stop_token Token ) { FlushLogMessages( Token ); } );
    }
}

void LumenEngine::FLogger::Flush ( const AnsiChar *const String )
{
    std::println( "{}", String );
    std::fflush( stdout );
}

LumenEngine::FLogger::~FLogger ()
{
    Shutdown();
}

void LumenEngine::FLogger::Shutdown ()
{
    WorkerThread.request_stop();
    Condition.notify_all();
}

void LumenEngine::FLogger::EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message )
{
    {
        std::lock_guard<std::mutex> Lock( QueueMutex );
        LogMessageQueue.push( { Category, Verbosity, std::move( Message ), HAL::FPlatformTime::Seconds() } );
    }
    Condition.notify_one();
}

namespace
{

inline void CoutMessage ( const LumenEngine::FLogger::FLogMessage &LogMessage ) noexcept
{
    static constexpr const LumenEngine::AnsiChar *const ResetColor   = "\033[0m";
    static constexpr const LumenEngine::AnsiChar *const FormatString = "[{:.4f}] {}: {}{}: {}{}";

    const LumenEngine::AnsiChar *const VerbosityColor  = LumenEngine::ELogVerbosity::ToColor( LogMessage.Verbosity );
    const LumenEngine::AnsiChar *const VerbosityString = LumenEngine::ELogVerbosity::ToString( LogMessage.Verbosity );

    std::println( FormatString, LogMessage.Timestamp, LogMessage.Category.CategoryName, VerbosityColor, VerbosityString, LogMessage.Message, ResetColor );

    if ( LogMessage.Verbosity == LumenEngine::ELogVerbosity::Fatal )
    {
        LumenEngine::FSignal::Raise( LumenEngine::ESystemSignal::Terminate );
    }
}

} // namespace

void LumenEngine::FLogger::FlushLogMessages ( std::stop_token &StopToken )
{
    while ( not StopToken.stop_requested() or not LogMessageQueue.empty() )
    {
        std::unique_lock Lock( QueueMutex );

        Condition.wait( Lock, StopToken, [this] { return !LogMessageQueue.empty(); } );

        while ( not LogMessageQueue.empty() )
        {
            FLogMessage Msg = std::move( LogMessageQueue.front() );
            LogMessageQueue.pop();

            Lock.unlock();
            CoutMessage( Msg );
            Lock.lock();
        }
    }
}
