///
/// @file Logger.cpp
/// @brief Implementation of the Logger class.
///

#include <Core/CoreTypes.hpp>
#include <Logging/Logger.hpp>

#include <HAL/PlatformTime.hpp>
#include <HAL/Signal.hpp>

#include <print>
#include <system_error>

Lumen::FLogger &Lumen::FLogger::GetInstance () noexcept
{
    static FLogger Instance;

    return Instance;
}

void Lumen::FLogger::Initialize () noexcept
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

void Lumen::FLogger::Flush ( const AnsiChar *const String ) noexcept
{
    const USize StringLength = std::char_traits<AnsiChar>::length( String );

    if ( write( STDOUT_FILENO, String, StringLength ) == -1 )
    {
        return;
    }
    fflush( stdout );
}

Lumen::FLogger::~FLogger ()
{
    Shutdown();
}

void Lumen::FLogger::Shutdown () noexcept
{
    if ( not bIsAsync or not WorkerThread.joinable() )
    {
        return;
    }
    WorkerThread.request_stop();
    Condition.notify_all();
    WorkerThread.join();
}

void Lumen::FLogger::EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message )
{
    {
        const TLockGuard<FMutex> Lock( QueueMutex );
        const FLogMessage LogMessage{ .Category  = Category,
                                      .Verbosity = Verbosity,
                                      .Message   = std::move( Message ),
                                      .Timestamp = HAL::FPlatformTime::Seconds() };

        LogMessageQueue.emplace( LogMessage );
        Condition.notify_one();
    }
}

void Lumen::FLogger::FlushLogMessages ( std::stop_token &StopToken )
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
void Lumen::FLogger::CoutMessage ( const FLogMessage &LogMessage ) const noexcept
{
    static constexpr const AnsiChar *const ResetColor   = "\033[0m";
    static constexpr const AnsiChar *const FormatString = "[{:.4f}] {}: {}{}: {}{}";

    const AnsiChar *const VerbosityColor  = Lumen::ELogVerbosity::ToColor( LogMessage.Verbosity );
    const AnsiChar *const VerbosityString = Lumen::ELogVerbosity::ToString( LogMessage.Verbosity );

    try
    {
        std::println( FormatString, LogMessage.Timestamp, LogMessage.Category.CategoryName, VerbosityColor, VerbosityString,
                      LogMessage.Message, ResetColor );
    }
    catch ( const std::format_error &FormatError )
    {
        Flush( "Log message formatting failed: " );
        Flush( FormatError.what() );
        Flush( "\n" );
    }

    if ( LogMessage.Verbosity == ELogVerbosity::Fatal )
    {
        FSignal::Raise( ESystemSignal::Terminate );
    }
}