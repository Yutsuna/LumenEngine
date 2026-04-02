/**
 * @file Logger.inl
 * @brief Implementation of the inline functions of the Logger class
 */

#pragma once

#include "Logging/Logger.hpp"

#include "HAL/PlatformTime.hpp"

#include <format>

template <typename... Args>
void LumenEngine::FLogger::TLog ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, const FStringView Format, Args &&...InArgs )
{
    FString FormattedMessage = std::vformat( Format, std::make_format_args( InArgs... ) );
    const bool bAsync        = bIsAsync.load( std::memory_order_relaxed ) && WorkerThread.joinable();

    if ( not bAsync )
    {
        CoutMessage( { .Category = Category, .Verbosity = Verbosity, .Message = std::move( FormattedMessage ), .Timestamp = HAL::FPlatformTime::Seconds() } );
        return;
    }
    EnqueueLogMessage( Category, Verbosity, std::move( FormattedMessage ) );
}
