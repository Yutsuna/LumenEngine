/**
 * @file Logger.hpp
 * @brief Definition of the Logger class
 */

#pragma once

#include "Container/Queue.hpp"
#include "LoggingCategory.hpp"
#include "LoggingVerbosity.hpp"
#include <condition_variable>
#include <mutex>

namespace LumenEngine
{

class LUMEN_ENGINE_API FLogger
{
public:

    /** Logs a message to the console */
    template <typename... Args> void TLog ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, const FStringView Format, Args &&...InArgs );

    /** Shuts down the logger and flushes all pending log messages. */
    void Shutdown ();

public:

    /** Gets the singleton instance of the logger. */
    static FLogger &GetInstance ();

private:

    void EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message );
    void FlushLogMessages ();

private:

    FLogger ();
    ~FLogger ();

public:

    struct FLogMessage
    {
        FLogCategory Category;
        ELogVerbosity::Type Verbosity;
        FString Message;
        Float64 Timestamp;
    };

private:

    FQueue<FLogMessage> LogMessageQueue;
    std::mutex QueueMutex;
    std::condition_variable Condition;
    std::thread WorkerThread;
    Atomic<Bool> bIsRunning;
};

} // namespace LumenEngine

#include "Inline/Logger.inl"

#define LUMEN_LOG( Category, Verbosity, Format, ... ) LumenEngine::FLogger::GetInstance().TLog( Category, Verbosity, Format, ##__VA_ARGS__ )

#define LUMEN_LOG_FATAL( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Fatal, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_ERROR( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Error, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_WARNING( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Warning, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_DISPLAY( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Display, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_INFO( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Info, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_VERBOSE( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::Verbose, Format, ##__VA_ARGS__ )
#define LUMEN_LOG_VERY_VERBOSE( Category, Format, ... ) LUMEN_LOG( Category, ELogVerbosity::VeryVerbose, Format, ##__VA_ARGS__ )
