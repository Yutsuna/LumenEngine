/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class.
 */

#include "Logging/Logger.hpp"
#include "Container/Signal.hpp"
#include "CoreTypes.hpp"
#include "HAL/PlatformTime.hpp"

#include <print>
#include <system_error>

LumenEngine::FLogger &LumenEngine::FLogger::GetInstance ()
{
    static FLogger Instance;

    return Instance;
}

void LumenEngine::FLogger::Initialize ()
{
    if ( not WorkerThread.joinable() )
    {
        try
        {
            WorkerThread = std::jthread( [this] ( std::stop_token Token ) { FlushLogMessages( Token ); } );
            bIsAsync     = true;
        }
        catch ( const std::system_error & /*SystemError*/ )
        {
            bIsAsync = false;
            Flush( "Failed to start logger worker thread, falling back to synchronous logging." );
        }
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
    if ( not bIsAsync or not WorkerThread.joinable() )
    {
        return;
    }
    WorkerThread.request_stop();
    Condition.notify_all();
    WorkerThread.join();
}

void LumenEngine::FLogger::EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message )
{
    {
        TLockGuard<FMutex> Lock( QueueMutex );
        LogMessageQueue.push( { Category, Verbosity, std::move( Message ), HAL::FPlatformTime::Seconds() } );
    }
    Condition.notify_one();
}

void LumenEngine::FLogger::FlushLogMessages ( std::stop_token &StopToken )
{
    while ( not StopToken.stop_requested() or not LogMessageQueue.empty() )
    {
        TUniqueLock<FMutex> Lock( QueueMutex );

        Condition.wait( Lock, StopToken, [this] { return !LogMessageQueue.empty(); } );

        while ( not LogMessageQueue.empty() )
        {
            FLogMessage Msg = std::move( LogMessageQueue.front() );
            LogMessageQueue.pop();

            Lock.Unlock();
            CoutMessage( Msg );
            Lock.Lock();
        }
    }
}

void LumenEngine::FLogger::CoutMessage ( const FLogMessage &LogMessage ) const noexcept
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
