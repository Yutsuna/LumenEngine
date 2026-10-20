///
/// @file Logger.cpp
/// @brief Implementation of the Logger class.
///

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Logging/Logger.hpp>

#include <HAL/PlatformTime.hpp>
#include <HAL/Signal.hpp>

#include <cerrno>
#include <cstdio>
#include <exception>
#include <print>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <io.h>
    #include <windows.h>
#elif defined( LUMEN_PLATFORM_UNIX ) || defined( LUMEN_PLATFORM_MACOS )
    #include <unistd.h>
#endif

namespace Lumen
{

namespace
{

    ///
    /// @brief Writes the whole buffer to stdout, bypassing stdio buffering. Never allocates.
    /// @return false on an unrecoverable I/O error.
    ///
    Bool WriteStdout ( const AnsiChar *Data, USize Size ) noexcept
    {
#if defined( LUMEN_PLATFORM_WINDOWS )
        while ( Size > 0 )
        {
            const unsigned Chunk = Size > 0x40000000u ? 0x40000000u : static_cast<unsigned>( Size );
            const int Written    = ::_write( 1, Data, Chunk );

            if ( Written <= 0 )
            {
                return false;
            }
            Data += Written;
            Size -= static_cast<USize>( Written );
        }
        return true;
#elif defined( LUMEN_PLATFORM_UNIX ) || defined( LUMEN_PLATFORM_MACOS )
        while ( Size > 0 )
        {
            const ISize Written = ::write( STDOUT_FILENO, Data, Size );

            if ( Written < 0 )
            {
                if ( errno == EINTR )
                {
                    continue;
                }
                return false;
            }
            if ( Written == 0 )
            {
                return false;
            }
            Data += Written;
            Size -= static_cast<USize>( Written );
        }
        return true;
#else
        // Unknown platform (console SDK, ...): portable fallback. Route through the HAL if you need raw I/O there.
        return std::fwrite( Data, 1, Size, stdout ) == Size;
#endif
    }

    ///
    /// @brief Legacy Windows consoles print ANSI escape sequences as garbage unless virtual terminal
    ///        processing is enabled. No-op on other platforms and when stdout is redirected.
    ///
    Bool EnableAnsiEscapeCodes () noexcept
    {
#if defined( LUMEN_PLATFORM_WINDOWS )
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif

        const HANDLE Handle = ::GetStdHandle( STD_OUTPUT_HANDLE );
        DWORD Mode          = 0;

        if ( Handle != nullptr && Handle != INVALID_HANDLE_VALUE && ::GetConsoleMode( Handle, &Mode ) )
        {
            ::SetConsoleMode( Handle, Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
        }
#endif
        return true;
    }

} // namespace

FLogger &FLogger::GetInstance () noexcept
{
    static FLogger Instance;

    return Instance;
}

void FLogger::Initialize () noexcept
{
    try
    {
        const TLockGuard<FMutex> Lock( QueueMutex );

        if ( WorkerThread.joinable() )
        {
            return;
        }
        WorkerThread = std::jthread( [this] ( std::stop_token Token ) { FlushLogMessages( Token ); } );
        bIsAsync.store( true );
    }
    catch ( std::system_error &SystemError )
    {
        static constexpr const AnsiChar *const ErrorMessage = "Failed to start logger worker thread: System Error: ";
        static constexpr const AnsiChar *const ErrorSuffix  = LUMEN_NEWLINE "Falling back to synchronous logging." LUMEN_NEWLINE;
        Flush( ErrorMessage );
        Flush( SystemError.what() );
        Flush( ErrorSuffix );
    }
}

void FLogger::Flush ( const AnsiChar *const String ) noexcept
{
    /// INFO: Flush stdout before writing to it to avoid overwriting earlier log lines.
    std::fflush( stdout );

    WriteStdout( String, std::char_traits<AnsiChar>::length( String ) );
}

FLogger::~FLogger ()
{
    Shutdown();
}

void FLogger::Shutdown () noexcept
{
    /// INFO: Serialize concurrent callers to avoid race conditions.
    const TLockGuard<FMutex> ShutdownLock( ShutdownMutex );

    std::jthread Worker;

    {
        const TLockGuard<FMutex> Lock( QueueMutex );

        bIsAsync.store( false );
        Worker = std::move( WorkerThread );
    }

    if ( Worker.joinable() )
    {
        Worker.request_stop();
        Worker.join();
    }
}

void FLogger::EnqueueLogMessage ( const FLogCategory &Category, const ELogVerbosity::Type Verbosity, FString &&Message )
{
    FLogMessage LogMessage{ .Category  = Category,
                            .Verbosity = Verbosity,
                            .Message   = std::move( Message ),
                            .Timestamp = HAL::FPlatformTime::Seconds() };

    if ( Verbosity == ELogVerbosity::Fatal )
    {
        Shutdown();
        CoutMessage( LogMessage );
        return;
    }

    const bool bQueued = [&] -> bool
    {
        const TLockGuard<FMutex> Lock( QueueMutex );

        if ( not bIsAsync.load() )
        {
            return false;
        }
        LogMessageQueue.push( std::move( LogMessage ) );
        return true;
    }();

    if ( bQueued )
    {
        Condition.notify_one();
        return;
    }

    CoutMessage( LogMessage );
}

void FLogger::FlushLogMessages ( std::stop_token &StopToken )
{
    decltype( LogMessageQueue ) Batch;

    for ( ;; )
    {
        {
            TUniqueLock<FMutex> Lock( QueueMutex );

            Condition.wait( Lock, StopToken, [this] { return !LogMessageQueue.empty(); } );

            if ( LogMessageQueue.empty() )
            {
                return;
            }

            std::swap( Batch, LogMessageQueue );
        }

        while ( not Batch.empty() )
        {
            CoutMessage( Batch.front() );
            Batch.pop();
        }

        std::fflush( stdout );
    }
}

// NOLINTNEXTLINE (readability-convert-member-functions-to-static)
void FLogger::CoutMessage ( const FLogMessage &LogMessage ) const noexcept
{
    static constexpr const AnsiChar *const ResetColor   = "\033[0m";
    static constexpr const AnsiChar *const FormatString = "[{:.4f}] {}: {}{}: {}{}";

    /// @brief Runs once, needed on Windows consoles; trivially true elsewhere.
    [[maybe_unused]] static const Bool bAnsiEscapeCodesReady = EnableAnsiEscapeCodes();

    const AnsiChar *const VerbosityColor  = ELogVerbosity::ToColor( LogMessage.Verbosity );
    const AnsiChar *const VerbosityString = ELogVerbosity::ToString( LogMessage.Verbosity );

    try
    {
        std::println( FormatString, LogMessage.Timestamp, LogMessage.Category.CategoryName, VerbosityColor, VerbosityString,
                      LogMessage.Message, ResetColor );
    }
    catch ( const std::exception &Exception )
    {
        Flush( "Log message output failed: " );
        Flush( Exception.what() );
        Flush( LUMEN_NEWLINE );
    }
    if ( LogMessage.Verbosity == ELogVerbosity::Fatal )
    {
        std::fflush( stdout );
        FSignal::Raise( ESystemSignal::Terminate );
    }
}

} // namespace Lumen