/**
 * @file Application.cpp
 * @brief Implements Lumina startup, native presentation and the per-frame world/render handoff.
 */

#include "Templates/Expected.hpp"
#include <Application.hpp>

#include <CameraActor.hpp>
#include <Execution/Renderer.hpp>
#include <Generic/GenericWindow.hpp>
#include <LogLumina.hpp>
#include <MessageHandler.hpp>
#include <PathTracerExtension.hpp>

#include <Engine/Modules/LaunchSurfaceBridge.hpp>
#include <RHIOpenGL/RHIOpenGLModule.hpp>

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <World/World.hpp>

#include <charconv>

namespace
{

[[nodiscard]] Lumen::TUnexpected<Lumen::Launch::FLaunchError>
LuminaFailure ( const Lumen::FStringView InMessage ) noexcept
{
    return Lumen::Launch::LaunchFailure( Lumen::Launch::ELaunchStage::Init,
                                         Lumen::Launch::ELaunchErrorCode::ModuleCreationFailed, InMessage );
}

void LogRHIFailure ( const Lumen::FStringView InOperation, const Lumen::RHI::FRHIError &InError ) noexcept
{
    LUMEN_LOG_ERROR( Lumen::LogLumina, "{}: {}", InOperation, InError.Message );
}

} // namespace

LUMEN_REGISTER_MODULE( Lumen::FLuminaApplication )
const Lumen::FLogCategory Lumen::LogLumina{ "Lumina" };

Lumen::FLuminaApplication::FLuminaApplication () noexcept = default;
Lumen::FLuminaApplication::~FLuminaApplication () noexcept = default;

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::FLuminaApplication>>
Lumen::FLuminaApplication::Create ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept
{
    TUniquePtr<FLuminaApplication> Application = MakeUnique<FLuminaApplication>();

    LUMEN_TRY_EXPECT_ERROR(Application->Initialize( InContext ));
    return Application;
}

Lumen::Launch::TLaunchResult<void>
Lumen::FLuminaApplication::Initialize ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept
{
    bHeadless = InContext.GetConfig().bHeadless;
    if ( not bHeadless )
    {
        Launch::FLaunchWindowModule *const WindowModule = InContext.Find<Launch::FLaunchWindowModule>();
        if ( WindowModule == nullptr or not WindowModule->GetWindow().IsValid() )
        {
            return LuminaFailure( "Lumina requires a native OpenGL window outside headless mode" );
        }
        Window = WindowModule->GetWindow();
    }

    IngressJournal.Reserve( FWorldIngressJournal::DefaultCapacity );
    World = MakeUnique<FWorld>( FWorldDesc{
        .WorkerCount            = 1U,
        .MaxFixedStepsPerTick   = 8U,
        .MaxActors              = 1ULL,
        .DeferredCommandCapacity = 16ULL,
        .ActorOutboxCapacity    = 4ULL,
        .EmissionCapacity       = 16ULL,
        .FixedDeltaTime         = InContext.GetConfig().FixedStep,
        .RenderCapacities       = { .TransformCapacity = 1ULL, .CameraCapacity = 1ULL },
    } );
    World->SetIngressJournal( &IngressJournal );

    auto SpawnedCamera = World->TrySpawnActor<ALuminaCameraActor>( World->GetSpatialRegistry() );
    if ( not SpawnedCamera )
    {
        return LuminaFailure( "Lumina could not spawn its orbital camera actor" );
    }
    CameraActor = *SpawnedCamera;
    Camera      = CameraActor->GetId();
    if ( not World->GetSpatialRegistry().TryRegisterSpatialEntity( Camera ) )
    {
        return LuminaFailure( "Lumina could not register its camera transform" );
    }
    CameraActor->PublishTransform();
    World->GetRenderContent().Cameras.push_back( FRenderCameraRecord{
        .ProxyId = FRenderProxyId( Camera ),
        .Flags   = ERenderCameraFlags::Active,
    } );
    if ( not World->PublishRenderSnapshot() )
    {
        return LuminaFailure( "Lumina could not publish its initial camera snapshot" );
    }

    RHI::FRHIExtent3D Extent{ 1280U, 720U, 1U };
    TOptional<FNativeWindowHandles> NativeHandles;
    TOptional<RHIOpenGL::FOpenGLNativeWindowDesc> NativeWindow;
    if ( Window.IsValid() )
    {
        const Maths::FVec2i Size = Window->GetWindowSize();
        if ( Size.X <= 0 or Size.Y <= 0 )
        {
            return LuminaFailure( "Lumina received an invalid native window extent" );
        }
        Extent = { static_cast<UInt32>( Size.X ), static_cast<UInt32>( Size.Y ), 1U };
        NativeHandles = Window->GetNativeHandles();
        if ( not NativeHandles )
        {
            return LuminaFailure( "Lumina could not query native OpenGL window handles" );
        }
        NativeWindow = Launch::MakeOpenGLNativeWindowDesc( *NativeHandles, Extent );
        if ( not NativeWindow )
        {
            return LuminaFailure( "Lumina native window handles are not supported by OpenGL" );
        }
    }

    const auto ModuleResult = NativeWindow ? RHIOpenGL::CreateRHIOpenGLModule( *NativeWindow )
                                           : RHIOpenGL::CreateRHIOpenGLModule();
    if ( not ModuleResult )
    {
        LogRHIFailure( "OpenGL module creation failed", ModuleResult.Error() );
        return LuminaFailure( "Lumina could not initialize the OpenGL RHI module" );
    }
    RHIModule = *ModuleResult;

    auto Adapters = RHIModule->EnumerateAdapters();
    if ( not Adapters or Adapters->empty() )
    {
        if ( not Adapters )
        {
            LogRHIFailure( "OpenGL adapter enumeration failed", Adapters.Error() );
        }
        return LuminaFailure( "Lumina found no OpenGL adapter" );
    }
    const RHI::FRHIAdapterInfo *SelectedAdapter = nullptr;
    for ( const RHI::FRHIAdapterInfo &Adapter : *Adapters )
    {
        if ( SelectedAdapter == nullptr or
             ( SelectedAdapter->Type == RHI::ERHIAdapterType::Software and Adapter.Type != RHI::ERHIAdapterType::Software ) )
        {
            SelectedAdapter = &Adapter;
        }
    }

    RHI::FRHIDeviceDesc DeviceDesc;
    DeviceDesc.AdapterId.Emplace( SelectedAdapter->AdapterId );
    DeviceDesc.RequiredCapabilities.Features = RHI::ERHICapabilityFlags::DynamicRendering |
                                               RHI::ERHICapabilityFlags::TimelineSemaphores;
    if ( Window.IsValid() )
    {
        DeviceDesc.RequiredCapabilities.Features |= RHI::ERHICapabilityFlags::Presentation;
    }
    auto DeviceResult = RHIModule->CreateDevice( DeviceDesc );
    if ( not DeviceResult )
    {
        LogRHIFailure( "OpenGL device creation failed", DeviceResult.Error() );
        return LuminaFailure( "Lumina could not create its OpenGL device" );
    }
    Device = *DeviceResult;

    if ( Window.IsValid() )
    {
        auto BridgeResult = Launch::CreateSurfaceBridge( *NativeHandles, Extent );
        if ( not BridgeResult )
        {
            LogRHIFailure( "OpenGL surface creation failed", BridgeResult.Error() );
            return LuminaFailure( "Lumina could not create its native OpenGL surface" );
        }
        SurfaceBridge = *BridgeResult;

        RHI::FRHISwapchainDesc SwapchainDesc;
        SwapchainDesc.Surface = {
            .Type              = RHI::ERHISurfaceType::Window,
            .NativeWindow      = NativeHandles->NativeWindow,
            .NativeDisplay     = NativeHandles->NativeDisplay,
            .bRequiresDisplay  = NativeHandles->NativeDisplay != 0U,
            .Bridge            = SurfaceBridge,
            .InitialExtent     = Extent,
        };
        SwapchainDesc.Format     = RHI::ERHIFormat::RGBA8Unorm;
        SwapchainDesc.Extent     = Extent;
        SwapchainDesc.ImageCount = 3U;
        auto SwapchainResult     = Device->CreateSwapchain( SwapchainDesc );
        if ( not SwapchainResult )
        {
            LogRHIFailure( "OpenGL swapchain creation failed", SwapchainResult.Error() );
            return LuminaFailure( "Lumina could not create its OpenGL swapchain" );
        }
        Swapchain = *SwapchainResult;
    }

    Renderer = MakeUnique<FRenderer>();
    TSharedPtr<IRenderFrameExtension> FrameExtension =
        MakeShared<FLuminaPathTracerExtension>( SelectedAdapter->Type == RHI::ERHIAdapterType::Software );
    const FRendererDesc RendererDesc{
        .Device         = Device,
        .SnapshotStream = &World->GetRenderSnapshotStream(),
        .Extent         = Extent,
        .CommandCapacity = 8ULL,
        .Swapchain      = Swapchain,
        .Shaders        = {},
        .ProxyCapacity  = 1ULL,
        .AssetCapacity  = 1ULL,
        .Readback       = { .Capacity = bHeadless ? static_cast<UInt64>( Extent.Width ) * Extent.Height * 4ULL : 0ULL },
        .FrameExtension = FrameExtension,
    };
    if ( not Renderer->Start( RendererDesc ) )
    {
        return LuminaFailure( "Lumina could not start its path-tracing renderer" );
    }

    if ( not bHeadless )
    {
        Launch::FLaunchApplicationModule *const ApplicationModule = InContext.Find<Launch::FLaunchApplicationModule>();
        if ( ApplicationModule == nullptr )
        {
            return LuminaFailure( "Lumina lost its application dependency during startup" );
        }
        MessageHandlerGuard = InstallLuminaMessageHandler( *ApplicationModule, *World, IngressJournal, Camera, *Renderer );
    }

    LUMEN_LOG_INFO( LogLumina, "Lumina path tracer started on '{}'.", SelectedAdapter->Name );
    UpdateWindowTitle( 0ULL );
    return {};
}

Lumen::Launch::EModuleTraits Lumen::FLuminaApplication::GetTraits () const noexcept
{
    return Launch::EModuleTraits::Tick;
}

void Lumen::FLuminaApplication::Tick ( const Float64 InDeltaSeconds, const Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    if ( not World.IsValid() or not Renderer.IsValid() )
    {
        return;
    }

    const auto Ticked = World->TryTick( InDeltaSeconds );
    if ( not Ticked )
    {
        LUMEN_LOG_WARNING( LogLumina, "The Lumina world rejected a tick with error {}.", static_cast<UInt32>( Ticked.Error() ) );
        return;
    }

    /**
     * INFO: Camera messages always target `WorldFrame + 1`, so a successful tick has applied every
     *       retained record. Clear the log to keep a continuous input stream bounded; the world
     *       observes the generation bump and restarts its cursor on the next tick.
     */
    IngressJournal.Clear();

    const auto Requested = Renderer->RequestRender( Ticked->FrameIndex );
    if ( not Requested and Requested.Error() != ERendererError::QueueFull )
    {
        LUMEN_LOG_WARNING( LogLumina, "The Lumina renderer rejected a frame request." );
    }

    Renderer->SwapStatusBuffers();
    UpdateWindowTitle( Renderer->GetStatus().ExtensionFrameCount );
}

void Lumen::FLuminaApplication::UpdateWindowTitle ( const UInt64 InSampleCount ) noexcept
{
    if ( not Window.IsValid() or InSampleCount == LastTitleSampleCount )
    {
        return;
    }

    AnsiChar Title[96] = "Lumina — Path Tracer OpenGL | Samples: ";
    constexpr USize PrefixSize = sizeof( "Lumina — Path Tracer OpenGL | Samples: " ) - 1ULL;
    const auto Converted = std::to_chars( Title + PrefixSize, Title + sizeof( Title ) - 1ULL, InSampleCount );
    if ( Converted.ec != std::errc{} )
    {
        return;
    }
    *Converted.ptr       = '\0';
    LastTitleSampleCount = InSampleCount;
    Window->SetWindowTitle( Title );
}
