/**
 * @file LaunchEngineLoops.cpp
 * @brief Implementation of the main loops of the Engine.
 */

#include "LaunchEngineLoops.hpp"

#include "ErrorCodes.hpp"
#include "HAL/PlatformTime.hpp"

#include "Generic/GenericApplication.hpp"
#include "Generic/GenericWindow.hpp"
#include "Generic/GenericWindowDescription.hpp"
#include "Maths/Macros.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    #include "Linux/LinuxApplication.hpp"
#endif

namespace
{

/**
 * @brief The platform-specific application instance, responsible for window management and input handling.
 * @details This is initialized in PreInit and used throughout the engine's lifecycle. It abstracts away platform differences.
 */
static LumenEngine::TSharedPtr<LumenEngine::FGenericApplication> GPlatformApplication = nullptr;

} // namespace

LumenEngine::Int32 LumenEngine::Launch::FEngineLoop::PreInit ( Int32 Argc, const AnsiChar *Argv[] )
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

LumenEngine::Int32 LumenEngine::Launch::FEngineLoop::Init ()
{
    TSharedRef<FGenericWindow> MainWindow            = GPlatformApplication->MakeWindow();
    TSharedRef<FGenericWindowDescription> WindowDesc = MakeShared<FGenericWindowDescription>();

    WindowDesc->Title = "Lumen Engine - Launching...";
    GPlatformApplication->InitializeWindow( MainWindow, WindowDesc, nullptr, true );
    return EErrorCode::Success;
}

void LumenEngine::Launch::FEngineLoop::Tick ()
{
    CalculateDeltaTime();

    if ( GPlatformApplication.IsValid() )
    {
        GPlatformApplication->PumpMessages( LastTickTime );
    }
}

LumenEngine::Bool LumenEngine::Launch::FEngineLoop::AppInit ()
{
    return EErrorCode::Success;
}

void LumenEngine::Launch::FEngineLoop::AppShutdown ()
{
    GPlatformApplication.Reset();
}

void LumenEngine::Launch::FEngineLoop::CalculateDeltaTime () noexcept
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
