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
TSharedPtr<FGenericApplication> FEngineLoop::GPlatformApplication = nullptr;

namespace
{
    static inline const FGenericWindowDescription GetDefaultWindowDescription ()
    {
        return ( const FGenericWindowDescription ){ .Title        = "Lumen Engine",
                                                    .Position     = Math::FVec2i( 100, 100 ),
                                                    .Size         = Math::FVec2i( 1280, 720 ),
                                                    .WindowMode   = EWindowMode::Windowed,
                                                    .bIsResizable = true,
                                                    .bIsVisible   = true };
    }

} // namespace

} // namespace LumenEngine

LumenEngine::Int32 LumenEngine::FEngineLoop::PreInit ( Int32 Argc, const AnsiChar *Argv[] )
{
#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    GPlatformApplication = FLinuxApplication::CreateLinuxApplication();
#else
    // TODO: Add other platforms
    return EErrorCode::Unsupported;
#endif

    if ( !GPlatformApplication.IsValid() )
    {
        return EErrorCode::Failure;
    }

    LastFrameSeconds = FPlatformTime::Seconds();
    TotalTickTime    = 0.0;
    LastTickTime     = FPlatformTime::DEFAULT_TICK_RATE;

    return EErrorCode::Success;
}

LumenEngine::Int32 LumenEngine::FEngineLoop::Init ()
{
    TSharedRef<FGenericWindow> MainWindow            = GPlatformApplication->MakeWindow();
    TSharedPtr<FGenericWindow> ParentWindow          = nullptr;
    TSharedRef<FGenericWindowDescription> WindowDesc = MakeShared<FGenericWindowDescription>( GetDefaultWindowDescription() );
    const Bool bShowImmediately                      = true;

    GPlatformApplication->InitializeWindow( MainWindow, WindowDesc, ParentWindow, bShowImmediately );
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

LumenEngine::Bool LumenEngine::FEngineLoop::AppInit ()
{
    return EErrorCode::Success;
}

void LumenEngine::FEngineLoop::AppShutdown ()
{
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

    if ( LastTickTime < Math::EPSILON )
    {
        LastTickTime = 0.0;
    }

    LastFrameSeconds = CurrentFrameSeconds;
    TotalTickTime += LastTickTime;
}
