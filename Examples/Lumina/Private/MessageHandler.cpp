/**
 * @file MessageHandler.cpp
 * @brief Implements Lumina input journaling and native resize forwarding.
 */

#include "Logging/Logger.hpp"
#include <MessageHandler.hpp>

#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Generic/GenericWindow.hpp>
#include <LogLumina.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <World/World.hpp>

#include <utility>

Lumen::Launch::FScopedMessageHandler Lumen::InstallLuminaMessageHandler ( Launch::FLaunchApplicationModule &InApplicationModule,
                                                                          FWorld &InWorld,
                                                                          FWorldIngressJournal &InJournal,
                                                                          const ActorID InCamera ) noexcept
{
    return InApplicationModule.PushMessageHandler<FLuminaMessageHandler>( InWorld, InJournal, InCamera );
}

Lumen::FLuminaMessageHandler::FLuminaMessageHandler ( TSharedPtr<FGenericApplicationMessageHandler> InNextHandler,
                                                      FWorld &InWorld,
                                                      FWorldIngressJournal &InJournal,
                                                      const ActorID InCamera ) noexcept
    : NextHandler( std::move( InNextHandler ) ), World( &InWorld ), Journal( &InJournal ), Camera( InCamera )
{
    /* */
}

void Lumen::FLuminaMessageHandler::SendCameraMessage ( const FMessage &InMessage ) noexcept
{
    if ( Journal == nullptr or World == nullptr )
    {
        return;
    }
    const auto Result = Journal->AppendMessage( World->GetFrameIndex() + 1ULL, Camera, InvalidActorID, InMessage );
    if ( not Result )
    {
        LUMEN_LOG_WARNING( LogLumina, "Camera input was rejected: journal error {}.", static_cast<UInt32>( Result.Error() ) );
    }
}

void Lumen::FLuminaMessageHandler::PublishCameraPose () noexcept
{
    const FOrbitCameraBasis Basis = ComputeOrbitCamera( State );
    const FCameraLookAtPayload LookAt{ .Eye = Basis.Position, .Target = State.Target, .Up = Basis.Up };
    SendCameraMessage( FMessage::Make( static_cast<UInt32>( EEngineMessage::CameraSetLookAt ), InvalidActorID, LookAt ) );
}

void Lumen::FLuminaMessageHandler::PublishCameraParams ( const Float32 InAspectRatio ) noexcept
{
    const FCameraParamsPayload Params{ .FieldOfViewDegrees = LuminaCameraFieldOfViewDegrees,
                                       .AspectRatio        = InAspectRatio,
                                       .NearPlane          = 0.1F,
                                       .FarPlane           = 1000.0F };
    SendCameraMessage( FMessage::Make( static_cast<UInt32>( EEngineMessage::CameraSetParams ), InvalidActorID, Params ) );
}

void Lumen::FLuminaMessageHandler::ResetCamera () noexcept
{
    State = FOrbitCameraState{};
    PublishCameraPose();
}

void Lumen::FLuminaMessageHandler::OnRequestExit () noexcept
{
    LUMEN_LOG_VERY_VERBOSE( LogLumina, "OnRequestExit called by FLuminaMessageHandler" );
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnRequestExit();
    }
}

void Lumen::FLuminaMessageHandler::OnQuit () noexcept
{
    LUMEN_LOG_VERY_VERBOSE( LogLumina, "OnQuit called by FLuminaMessageHandler" );
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnQuit();
    }
}

void Lumen::FLuminaMessageHandler::OnOSPaint ( const TSharedRef<FGenericWindow> &InWindow ) noexcept
{
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnOSPaint( InWindow );
    }
}

void Lumen::FLuminaMessageHandler::OnWindowMoved ( const TSharedRef<FGenericWindow> &InWindow ) noexcept
{
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnWindowMoved( InWindow );
    }
}

void Lumen::FLuminaMessageHandler::OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow ) noexcept
{
    const Maths::FVec2i Size = InWindow->GetWindowSize();
    if ( Size.X > 0 and Size.Y > 0 )
    {
        PublishCameraParams( static_cast<Float32>( Size.X ) / static_cast<Float32>( Size.Y ) );
    }
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnWindowResized( InWindow );
    }
}

void Lumen::FLuminaMessageHandler::OnWindowActivationChanged ( const TSharedRef<FGenericWindow> &InWindow ) noexcept
{
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnWindowActivationChanged( InWindow );
    }
}

void Lumen::FLuminaMessageHandler::OnMouseDown ( const EMouseButton::Type InMouseButton ) noexcept
{
    if ( InMouseButton == EMouseButton::Left )
    {
        bDragging = true;
    }
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnMouseDown( InMouseButton );
    }
}

void Lumen::FLuminaMessageHandler::OnMouseUp ( const EMouseButton::Type InMouseButton ) noexcept
{
    if ( InMouseButton == EMouseButton::Left )
    {
        bDragging = false;
    }
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnMouseUp( InMouseButton );
    }
}

void Lumen::FLuminaMessageHandler::OnMouseDoubleClick ( const EMouseButton::Type InMouseButton ) noexcept
{
    if ( InMouseButton == EMouseButton::Left )
    {
        ResetCamera();
    }
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnMouseDoubleClick( InMouseButton );
    }
}

void Lumen::FLuminaMessageHandler::OnMouseMove ( const Int32 InX, const Int32 InY ) noexcept
{
    if ( bDragging )
    {
        const Float32 DeltaX = static_cast<Float32>( InX - LastX );
        const Float32 DeltaY = static_cast<Float32>( InY - LastY );
        if ( DeltaX != 0.0F or DeltaY != 0.0F )
        {
            State = ApplyOrbitDrag( State, DeltaX, DeltaY );
            PublishCameraPose();
        }
    }
    LastX = InX;
    LastY = InY;
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnMouseMove( InX, InY );
    }
}

void Lumen::FLuminaMessageHandler::OnMouseWheel ( const Int32 InDelta ) noexcept
{
    State = ApplyOrbitZoom( State, static_cast<Float32>( InDelta ) );
    PublishCameraPose();
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnMouseWheel( InDelta );
    }
}

void Lumen::FLuminaMessageHandler::OnKeyDown ( const EKeys::Type InKey, const Bool bIsRepeated ) noexcept
{
    if ( InKey == EKeys::R and not bIsRepeated )
    {
        ResetCamera();
    }
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnKeyDown( InKey, bIsRepeated );
    }
}

void Lumen::FLuminaMessageHandler::OnKeyUp ( const EKeys::Type InKey, const Bool bIsRepeated ) noexcept
{
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnKeyUp( InKey, bIsRepeated );
    }
}
