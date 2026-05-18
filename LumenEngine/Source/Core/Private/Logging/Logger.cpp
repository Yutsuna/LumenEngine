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

LumenEngine::FLogger &LumenEngine::FLogger::GetInstance () noexcept
{
    static FLogger Instance;

    return Instance;
}

void LumenEngine::FLogger::Initialize () noexcept
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

void LumenEngine::FLogger::Flush ( const AnsiChar *const String ) noexcept
{
    const USize StringLength = std::char_traits<AnsiChar>::length( String );

    if ( write( STDOUT_FILENO, String, StringLength ) == -1 )
    {
        return;
    }
    fflush( stdout );
}

LumenEngine::FLogger::~FLogger ()
{
    Shutdown();
}

void LumenEngine::FLogger::Shutdown () noexcept
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
        const TLockGuard<FMutex> Lock( QueueMutex );
        const FLogMessage LogMessage{ .Category = Category, .Verbosity = Verbosity, .Message = std::move( Message ), .Timestamp = HAL::FPlatformTime::Seconds() };

        LogMessageQueue.emplace( LogMessage );
        Condition.notify_one();
    }
}

void LumenEngine::FLogger::FlushLogMessages ( std::stop_token &StopToken )
{
    while ( not StopToken.stop_requested() or not LogMessageQueue.empty() )
    {
        TUniqueLock<FMutex> Lock( QueueMutex );

        Condition.wait( Lock, StopToken, [this] { return !LogMessageQueue.empty(); } );

        while ( not LogMessageQueue.empty() )
        {
            const FLogMessage Msg = std::move( LogMessageQueue.front() );
            LogMessageQueue.pop();

            Lock.Unlock();
            CoutMessage( Msg );
            Lock.Lock();
        }
    }
}

// NOLINTNEXTLINE (readability-convert-member-functions-to-static)
void LumenEngine::FLogger::CoutMessage ( const FLogMessage &LogMessage ) const noexcept
{
    static constexpr const LumenEngine::AnsiChar *const ResetColor   = "\033[0m";
    static constexpr const LumenEngine::AnsiChar *const FormatString = "[{:.4f}] {}: {}{}: {}{}";

    const LumenEngine::AnsiChar *const VerbosityColor  = LumenEngine::ELogVerbosity::ToColor( LogMessage.Verbosity );
    const LumenEngine::AnsiChar *const VerbosityString = LumenEngine::ELogVerbosity::ToString( LogMessage.Verbosity );

    try
    {
        std::println( FormatString, LogMessage.Timestamp, LogMessage.Category.CategoryName, VerbosityColor, VerbosityString, LogMessage.Message, ResetColor );
    }
    catch ( const std::format_error &FormatError )
    {
        Flush( "Log message formatting failed: " );
        Flush( FormatError.what() );
        Flush( "\n" );
    }

    if ( LogMessage.Verbosity == LumenEngine::ELogVerbosity::Fatal )
    {
        LumenEngine::FSignal::Raise( LumenEngine::ESystemSignal::Terminate );
    }
}
