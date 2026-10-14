///
/// @file Logger.hpp
/// @brief Definition of the Logger class
///

#pragma once

#include <Logging/LoggingCategory.hpp>

#include <HAL/Mutex.hpp>
#include <Templates/Queue.hpp>

#include <condition_variable>
#include <thread>

namespace Lumen
{

///
/// @class FLogger
/// @brief Asynchronous Logging
///
class CORE_EXPORT FLogger
{
public:

    /// @brief Logs a message to the console
    template <typename... Args>
    void TLog ( const FLogCategory &Category, ELogVerbosity::Type Verbosity, FStringView Format, Args &&...InArgs ) noexcept;

    /// @brief Initializes the logger and starts the worker thread.
    void Initialize () noexcept;

    /// @brief Shuts down the logger and flushes all pending log messages.
    void Shutdown () noexcept;

public:

    /// @brief Gets the singleton instance of the logger.
    static FLogger &GetInstance () noexcept;

    /// @brief Flushes a message to the console immediately without enqueuing it.
    static void Flush ( const AnsiChar *const String ) noexcept;

public:

    struct FLogMessage
    {
        FLogCategory Category;
        ELogVerbosity::Type Verbosity;
        FString Message;
        Float64 Timestamp;
    };

private:

    void EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message );
    void FlushLogMessages ( std::stop_token &StopToken );
    void CoutMessage ( const FLogMessage &LogMessage ) const noexcept;

private:

    FLogger () noexcept = default;
    ~FLogger ();

private:

    FMutex QueueMutex;
    std::condition_variable_any Condition;
    TQueue<FLogMessage> LogMessageQueue;

    std::jthread WorkerThread;
    TAtomic<Bool> bIsAsync;
};

} // namespace Lumen

#include "Inline/Logger.inl"

#define LUMEN_LOG( Category, Verbosity, Format, ... )                                                                            \
    Lumen::FLogger::GetInstance().TLog( Category, Verbosity, Format __VA_OPT__(, ) __VA_ARGS__ )

#define LUMEN_LOG_DEFINE_CATEGORY( CategoryVar, CategoryName ) static const ::Lumen::FLogCategory CategoryVar( CategoryName );

#define LUMEN_LOG_FATAL( Category, Format, ... )                                                                                 \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Fatal, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_ERROR( Category, Format, ... )                                                                                 \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Error, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_WARNING( Category, Format, ... )                                                                               \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Warning, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_DISPLAY( Category, Format, ... )                                                                               \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Display, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_INFO( Category, Format, ... )                                                                                  \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Info, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_VERBOSE( Category, Format, ... )                                                                               \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::Verbose, Format __VA_OPT__(, ) __VA_ARGS__ )
#define LUMEN_LOG_VERY_VERBOSE( Category, Format, ... )                                                                          \
    LUMEN_LOG( Category, Lumen::ELogVerbosity::VeryVerbose, Format __VA_OPT__(, ) __VA_ARGS__ )

#ifdef LUMEN_SHOW_LOG_SHADER_TRACE
    #define LUMEN_LOG_SHADER_TRACE( Category, Format, ... ) LUMEN_LOG_VERBOSE( Category, Format __VA_OPT__(, ) __VA_ARGS__ )
#else
    #define LUMEN_LOG_SHADER_TRACE( Category, Format, ... ) ( ( void )0 )
#endif