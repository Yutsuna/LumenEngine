/**
 * @file LaunchEngineLoops.cpp
 * @brief Implementation of the main loops of the Engine.
 */

#include "LaunchEngineLoop.hpp"

#include "ErrorCodes.hpp"
#include "HAL/PlatformTime.hpp"

#include "Generic/GenericApplication.hpp"
#include "Generic/GenericWindow.hpp"
#include "Generic/GenericWindowDescription.hpp"
#include "Maths/Macros.hpp"

#include "Logging/Logger.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    #include "Linux/LinuxApplication.hpp"
#endif

namespace LumenEngine
{

namespace
{
    static FLogCategory LogLaunch( "LogLaunch" );
}

FEngineLoop GEngineLoop;
TSharedPtr<FGenericApplication> GPlatformApplication = nullptr;

namespace
{
    static inline const FGenericWindowDescription GetDefaultWindowDescription ()
    {
        return { .Title        = "Lumen Engine",
                 .Position     = Maths::FVec2i( 100, 100 ),
                 .Size         = Maths::FVec2i( 1280, 720 ),
                 .WindowMode   = EWindowMode::Windowed,
                 .bIsResizable = true,
                 .bIsVisible   = true };
    }

} // namespace

} // namespace LumenEngine

LumenEngine::Int32 LumenEngine::FEngineLoop::PreInit ( Int32 Argc, const AnsiChar *[] )
{
    LUMEN_LOG_INFO( LogLaunch, "Engine PreInit started with {} arguments", Argc );

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    GPlatformApplication = FLinuxApplication::CreateLinuxApplication();
#else
    // TODO: Add other platforms
    LUMEN_LOG_ERROR( LogLaunch, "Platform Application layer not yet implemented for this OS." );
    return EErrorCode::Unsupported;
#endif

    if ( !GPlatformApplication.IsValid() )
    {
        LUMEN_LOG_ERROR( LogLaunch, "Failed to create Platform Application layer." );
        return EErrorCode::Failure;
    }

    LastFrameSeconds = FPlatformTime::Seconds();
    TotalTickTime    = 0.0;
    LastTickTime     = FPlatformTime::DEFAULT_TICK_RATE;
    bRequestingExit  = false;

    LUMEN_LOG_INFO( LogLaunch, "Engine PreInit completed successfully." );
    return EErrorCode::Success;
}

LumenEngine::Int32 LumenEngine::FEngineLoop::Init ()
{
    LUMEN_LOG_INFO( LogLaunch, "Engine Init started..." );

    TSharedRef<FGenericWindow> MainWindow            = GPlatformApplication->MakeWindow();
    TSharedPtr<FGenericWindow> ParentWindow          = nullptr;
    TSharedRef<FGenericWindowDescription> WindowDesc = MakeShared<FGenericWindowDescription>( GetDefaultWindowDescription() );
    const Bool bShowImmediately                      = true;

    LUMEN_LOG_INFO( LogLaunch, "Creating main application window..." );
    GPlatformApplication->InitializeWindow( MainWindow, WindowDesc, ParentWindow, bShowImmediately );

    LUMEN_LOG_INFO( LogLaunch, "Engine Init completed successfully." );
    return EErrorCode::Success;
}

void LumenEngine::FEngineLoop::Tick ()
{
    CalculateDeltaTime();

    if ( GPlatformApplication.IsValid() )
    {
        GPlatformApplication->PumpMessages( LastTickTime );
    }
}

void LumenEngine::FEngineLoop::Exit ()
{
    LUMEN_LOG_INFO( LogLaunch, "Engine Exit requested. Releasing platform application..." );
    GPlatformApplication.Reset();
}

void LumenEngine::FEngineLoop::RequestExit ( const AnsiChar *Reason )
{
    LUMEN_LOG_WARNING( LogLaunch, "Exit requested: {}", Reason );
    bRequestingExit = true;
}

LumenEngine::Bool LumenEngine::FEngineLoop::ShouldExit () const
{
    return bRequestingExit;
}

LumenEngine::EErrorCode::Type LumenEngine::FEngineLoop::AppInit ()
{
    LUMEN_LOG_INFO( LogLaunch, "Application Logic Init started..." );

    if ( GEngineLoop.Init() != EErrorCode::Success )
    {
        LUMEN_LOG_ERROR( LogLaunch, "Failed to initialize Engine from AppInit. " );
        return EErrorCode::Failure;
    }

    return EErrorCode::Success;
}

void LumenEngine::FEngineLoop::AppShutdown ()
{
    LUMEN_LOG_INFO( LogLaunch, "Application Logic Shutdown started..." );
    GPlatformApplication.Reset();
}

void LumenEngine::FEngineLoop::CalculateDeltaTime () noexcept
{
    const Float64 CurrentFrameSeconds = FPlatformTime::Seconds();

    LastTickTime = CurrentFrameSeconds - LastFrameSeconds;

    if ( LastTickTime > FPlatformTime::MAX_TICK_RATE )
    {
        LastTickTime = FPlatformTime::MAX_TICK_RATE;
    }

    if ( LastTickTime < Maths::EPSILON )
    {
        LastTickTime = 0.0;
    }

    LastFrameSeconds = CurrentFrameSeconds;
    TotalTickTime += LastTickTime;
}
