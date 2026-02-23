/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class.
 */

#include "Logging/Logger.hpp"
#include "CoreTypes.hpp"
#include "HAL/PlatformTime.hpp"
#include <iostream>

LumenEngine::FLogger &LumenEngine::FLogger::GetInstance ()
{
    static FLogger Instance;

    return Instance;
}

LumenEngine::FLogger::FLogger () : bIsRunning( true )
{
    WorkerThread = std::thread( [this] { FlushLogMessages(); } );
}

LumenEngine::FLogger::~FLogger ()
{
    Shutdown();
}

void LumenEngine::FLogger::Shutdown ()
{
    bIsRunning = false;
    Condition.notify_all();

    if ( WorkerThread.joinable() )
    {
        WorkerThread.join();
    }
}

void LumenEngine::FLogger::EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message )
{
    {
        std::lock_guard<std::mutex> Lock( QueueMutex );
        LogMessageQueue.push( { Category, Verbosity, std::move( Message ), FPlatformTime::Seconds() } );
    }
    Condition.notify_one();
}

namespace
{

static inline void CoutMessage ( const LumenEngine::FLogger::FLogMessage &LogMessage ) noexcept
{
    static constexpr const LumenEngine::AnsiChar *const ResetColor = "\033[0m";

    const LumenEngine::AnsiChar *const VerbosityColor  = LumenEngine::ELogVerbosity::ToColor( LogMessage.Verbosity );
    const LumenEngine::AnsiChar *const VerbosityString = LumenEngine::ELogVerbosity::ToString( LogMessage.Verbosity );

    std::cout << std::format( "[{:.4f}][{}]{}{}{}{}\n", LogMessage.Timestamp, LogMessage.Category.CategoryName, VerbosityColor, VerbosityString, LogMessage.Message,
                              ResetColor );
}

} // namespace

void LumenEngine::FLogger::FlushLogMessages ()
{
    while ( bIsRunning || not LogMessageQueue.empty() )
    {
        std::unique_lock<std::mutex> Lock( QueueMutex );
        Condition.wait( Lock, [this] { return not LogMessageQueue.empty() || not bIsRunning; } );

        while ( not LogMessageQueue.empty() )
        {
            const FLogMessage &LogMessage = LogMessageQueue.front();

            LogMessageQueue.pop();
            Lock.unlock();

            CoutMessage( LogMessage );
        }
    }
}
