/**
 * @file LaunchEngineLoops.cpp
 * @brief Implementation of the main loops of the Engine.
 */

#include "LaunchEngineLoop.hpp"
#include "GameApplication.hpp"

#include "ErrorCodes.hpp"
#include "HAL/PlatformTime.hpp"

#include "Generic/GenericApplication.hpp"
#include "Generic/GenericWindow.hpp"
#include "Generic/GenericWindowDescription.hpp"
#include "Maths/Macros.hpp"

#include "Logging/Logger.hpp"
#include "Maths/Vec.hpp"

#include "Graphics/Renderer.hpp"
#include "Vulkan/VulkanRHI.hpp"

#if defined( LUMEN_ENGINE_PLATFORM_LINUX )
    #include "Linux/LinuxApplication.hpp"
#endif

namespace LumenEngine
{

namespace
{
    const FLogCategory LogLaunch( "LogLaunch" );
}

FEngineLoop GEngineLoop;
TSharedPtr<FGenericApplication> GPlatformApplication = nullptr;

namespace
{

    inline FGenericWindowDescription GetDefaultWindowDescription () noexcept
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

LumenEngine::Int32 LumenEngine::FEngineLoop::PreInit ( Int32 Argc, const AnsiChar LUMEN_UNUSED *Argv[] )
{
    LumenEngine::FLogger::GetInstance().Initialize();
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

    LastFrameSeconds = HAL::FPlatformTime::Seconds();
    TotalTickTime    = 0.0;
    LastTickTime     = HAL::FPlatformTime::DefaultTickRate;
    bRequestingExit.store( false, std::memory_order_relaxed );

    LUMEN_LOG_INFO( LogLaunch, "Engine PreInit completed successfully." );
    return EErrorCode::Success;
}

LumenEngine::Int32 LumenEngine::FEngineLoop::Init ()
{
    LUMEN_LOG_INFO( LogLaunch, "Engine Init started..." );

    TSharedRef<FGenericWindow> MainWindow            = GPlatformApplication->MakeWindow();
    TSharedRef<FGenericWindowDescription> WindowDesc = MakeShared<FGenericWindowDescription>( GetDefaultWindowDescription() );

    GPlatformApplication->InitializeWindow( MainWindow, WindowDesc, nullptr, true );

    TUniquePtr<RHI::IRHI> VulkanBackend = MakeUnique<VulkanRHI::FVulkanRHI>();
    Renderer::GRenderer                 = MakeUnique<Renderer::FRenderer>();
    Renderer::GRenderer->Initialize( std::move( VulkanBackend ), MainWindow );

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

    Launch::GetGameApplication().Tick( LastTickTime );

    if ( Renderer::GRenderer.IsValid() )
    {
        Renderer::GRenderer->RenderFrame();
    }
    ++FrameIndex;
}

void LumenEngine::FEngineLoop::Exit () const
{
    LUMEN_LOG_INFO( LogLaunch, "Engine Exit requested. Releasing platform application..." );

    Renderer::GRenderer.Reset();

    if ( GPlatformApplication.IsValid() )
    {
        GPlatformApplication->Shutdown();
    }
    GPlatformApplication.Reset();
}

void LumenEngine::FEngineLoop::RequestExit ( const AnsiChar *const Reason ) noexcept
{
    LUMEN_LOG_WARNING( LogLaunch, "Exit requested: {}", Reason );
    RequestExitAsyncSafe();
}

void LumenEngine::FEngineLoop::RequestExitAsyncSafe () noexcept
{
    bRequestingExit.store( true, std::memory_order_relaxed );
}

LumenEngine::UInt64 LumenEngine::FEngineLoop::GetFrameIndex () const noexcept
{
    return FrameIndex;
}

LumenEngine::Bool LumenEngine::FEngineLoop::ShouldExit () const noexcept
{
    return bRequestingExit.load( std::memory_order_relaxed );
}

void LumenEngine::FEngineLoop::CalculateDeltaTime () noexcept
{
    const Float64 CurrentFrameSeconds = HAL::FPlatformTime::Seconds();

    LastTickTime = CurrentFrameSeconds - LastFrameSeconds;

    if ( std::min( LastTickTime, HAL::FPlatformTime::MaxTickRate ) != LastTickTime )
    {
        LastTickTime = HAL::FPlatformTime::MaxTickRate;
    }

    if ( LastTickTime < Maths::Epsilon<Float64> )
    {
        LastTickTime = 0.0;
    }

    LastFrameSeconds = CurrentFrameSeconds;
    TotalTickTime += LastTickTime;
}
