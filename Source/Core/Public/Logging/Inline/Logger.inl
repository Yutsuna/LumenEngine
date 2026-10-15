///
/// @file Logger.inl
/// @brief Implementation of the inline functions of the Logger class
///

#pragma once

#include <HAL/PlatformTime.hpp>
#include <Logging/Logger.hpp>

#include <atomic>
#include <exception>
#include <format>
#include <utility>

template <typename... Args>
void Lumen::FLogger::TLog ( const FLogCategory &Category,
                            const ELogVerbosity::Type Verbosity,
                            const FStringView Format,
                            Args &&...InArgs ) noexcept
{
    try
    {
        FString FormattedMessage = std::vformat( Format, std::make_format_args( InArgs... ) );

        // Only a hint: EnqueueLogMessage re-checks under the queue mutex and falls back to synchronous
        // output if the worker is gone. WorkerThread must not be read here, it is guarded by QueueMutex.
        const Bool bAsync = bIsAsync.load( std::memory_order_relaxed );

        if ( not bAsync )
        {
            CoutMessage( { .Category  = Category,
                           .Verbosity = Verbosity,
                           .Message   = std::move( FormattedMessage ),
                           .Timestamp = HAL::FPlatformTime::Seconds() } );
            return;
        }
        EnqueueLogMessage( Category, Verbosity, std::move( FormattedMessage ) );
    }
    catch ( const std::format_error &FormatError )
    {
        Flush( "Format error in log message: " );
        Flush( FormatError.what() );
        Flush( "\n" );
    }
    catch ( const std::exception &Exception )
    {
        // This function is noexcept: bad_alloc (formatting / queueing) must not end in std::terminate.
        Flush( "Failed to log message: " );
        Flush( Exception.what() );
        Flush( "\n" );
    }
    catch ( ... )
    {
        Flush( "Failed to log message: unknown exception\n" );
    }
}
