/**
 * @file Application.cpp
 * @brief Implements Lumina startup, camera world and the per-frame render handoff.
 */

#include "Templates/Expected.hpp"
#include <Application.hpp>

#include <Camera/CameraActor.hpp>
#include <Generic/GenericWindow.hpp>
#include <LogLumina.hpp>
#include <MessageHandler.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <OrbitCamera.hpp>
#include <PathTracerExtension.hpp>

#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Engine/Modules/LaunchRendererModule.hpp>
#include <Engine/Modules/LaunchWindowModule.hpp>

#include <World/World.hpp>

#include <Maths/Macros.hpp>

#include <charconv>

namespace Lumen
{

namespace
{

    inline TUnexpected<Launch::FLaunchError> LuminaFailure ( const FStringView InMessage ) noexcept
    {
        return Launch::LaunchFailure( Launch::ELaunchStage::Init, Launch::ELaunchErrorCode::ModuleCreationFailed, InMessage );
    }

} // namespace

LUMEN_REGISTER_MODULE( FLuminaApplication )
const FLogCategory LogLumina{ "Lumina" };

FLuminaApplication::FLuminaApplication () noexcept  = default;
FLuminaApplication::~FLuminaApplication () noexcept = default;

Launch::TLaunchResult<TUniquePtr<FLuminaApplication>>
FLuminaApplication::Create ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept
{
    TUniquePtr<FLuminaApplication> Application = MakeUnique<FLuminaApplication>();
    LUMEN_TRY_EXPECT_ERROR( Application->Initialize( InContext ) );
    return Application;
}

Launch::TLaunchResult<void>
FLuminaApplication::Initialize ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept
{
    bHeadless                                     = InContext.GetConfig().bHeadless;
    Launch::FLaunchRendererModule &RendererModule = InContext.Get<Launch::FLaunchRendererModule>();
    if ( not bHeadless )
    {
        Launch::FLaunchWindowModule *const WindowModule = InContext.Find<Launch::FLaunchWindowModule>();
        if ( WindowModule == nullptr or not WindowModule->GetWindow().IsValid() )
        {
            return LuminaFailure( "Lumina requires a native window outside headless mode" );
        }
        Window = WindowModule->GetWindow();
    }

    IngressJournal.Reserve( FWorldIngressJournal::DefaultCapacity );
    World = MakeUnique<FWorld>( FWorldDesc{
        .WorkerCount             = 1U,
        .MaxFixedStepsPerTick    = 8U,
        .MaxActors               = 1ULL,
        .DeferredCommandCapacity = 16ULL,
        .ActorOutboxCapacity     = 4ULL,
        .EmissionCapacity        = 16ULL,
        .FixedDeltaTime          = InContext.GetConfig().FixedStep,
        .RenderCapacities        = { .TransformCapacity = 1ULL, .CameraCapacity = 1ULL },
    } );
    World->SetIngressJournal( &IngressJournal );

    Maths::FVec2i InitialSize{ 1280, 720 };
    if ( Window.IsValid() )
    {
        InitialSize = Window->GetWindowSize();
        if ( InitialSize.X <= 0 or InitialSize.Y <= 0 )
        {
            return LuminaFailure( "Lumina received an invalid native window extent" );
        }
    }
    const Float32 InitialAspect = static_cast<Float32>( InitialSize.X ) / static_cast<Float32>( InitialSize.Y );

    const FOrbitCameraState InitialState{};
    const FOrbitCameraBasis InitialBasis = ComputeOrbitCamera( InitialState );
    const FCameraComponentDesc CameraDesc{
        .View       = Maths::FMat4x4f::LookAt( InitialBasis.Position, InitialState.Target, InitialBasis.Up ),
        .Projection = Maths::FMat4x4f::Perspective( LuminaCameraFieldOfViewDegrees * Maths::DegToRad<Float32>, InitialAspect,
                                                    0.1F, 1000.0F ),
        .NearPlane  = 0.1F,
        .FarPlane   = 1000.0F,
        .Flags      = ERenderCameraFlags::Active,
    };

    auto SpawnedCamera = World->TrySpawnActor<ACameraActor>( World->GetSpatialRegistry(), World->GetCameraRegistry() );
    if ( not SpawnedCamera )
    {
        return LuminaFailure( "Lumina could not spawn its generic camera actor" );
    }
    CameraActor = ( *SpawnedCamera ).Get();
    Camera      = CameraActor->GetId();
    if ( not World->GetSpatialRegistry().TryRegisterSpatialEntity( Camera ) )
    {
        return LuminaFailure( "Lumina could not register its camera transform" );
    }
    if ( not World->TryAddCameraComponent( Camera, CameraDesc ) )
    {
        return LuminaFailure( "Lumina could not register its camera component" );
    }

    const FCameraLookAtPayload InitialLookAt{
        .Eye = InitialBasis.Position, .Target = InitialState.Target, .Up = InitialBasis.Up };
    CameraActor->Receive(
        FMessage::Make( static_cast<UInt32>( EEngineMessage::CameraSetLookAt ), InvalidActorID, InitialLookAt ) );
    if ( not World->PublishRenderSnapshot() )
    {
        return LuminaFailure( "Lumina could not publish its initial camera snapshot" );
    }

    const FRenderExtent3D Extent{ .Width=static_cast<UInt32>( InitialSize.X ), .Height=static_cast<UInt32>( InitialSize.Y ), .Depth=1U };
    const Launch::FRenderSessionDesc SessionDesc{
        .SnapshotStream = &World->GetRenderSnapshotStream(),
        .Extent         = Extent,
        .Feature        = MakeShared<FLuminaPathTracerExtension>(),
        .Readback       = { .Capacity = bHeadless ? static_cast<UInt64>( Extent.Width ) * Extent.Height * 4ULL : 0ULL },
    };
    Session.Emplace( RendererModule, SessionDesc );
    if ( not Session->IsActive() )
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
        MessageHandlerGuard = InstallLuminaMessageHandler( *ApplicationModule, *World, IngressJournal, Camera );
    }

    LUMEN_LOG_INFO( LogLumina, "Lumina path tracer started on '{}'.", RendererModule.GetDeviceDescription() );
    UpdateWindowTitle( 0ULL );
    return {};
}

Launch::EModuleTraits FLuminaApplication::GetTraits () const noexcept
{
    return Launch::EModuleTraits::Tick;
}

void FLuminaApplication::Tick ( const Float64 InDeltaSeconds, const Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    if ( not World.IsValid() or not Session.HasValue() )
    {
        return;
    }

    const auto Ticked = World->TryTick( InDeltaSeconds );
    if ( not Ticked )
    {
        LUMEN_LOG_WARNING( LogLumina, "The Lumina world rejected a tick with error {}.", static_cast<UInt32>( Ticked.Error() ) );
        return;
    }

    IngressJournal.Clear();
    Session->RequestRender( Ticked->FrameIndex );
    Session->SwapStatusBuffers();
    UpdateWindowTitle( Session->GetStatus().ExtensionFrameCount );
}

void FLuminaApplication::UpdateWindowTitle ( const UInt64 InSampleCount ) noexcept
{
    if ( not Window.IsValid() or InSampleCount == LastTitleSampleCount )
    {
        return;
    }

    AnsiChar Title[96]         = "Lumina — Path Tracer | Samples: ";
    constexpr USize PrefixSize = sizeof( "Lumina — Path Tracer | Samples: " ) - 1ULL;
    const auto Converted       = std::to_chars( Title + PrefixSize, Title + sizeof( Title ) - 1ULL, InSampleCount );
    if ( Converted.ec != std::errc{} )
    {
        return;
    }
    *Converted.ptr       = '\0';
    LastTitleSampleCount = InSampleCount;
    Window->SetWindowTitle( Title );
}

} // namespace Lumen
