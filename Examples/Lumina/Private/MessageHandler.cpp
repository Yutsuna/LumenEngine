/**
 * @file MessageHandler.cpp
 * @brief Implements Lumina input journaling and native resize forwarding.
 */

#include <MessageHandler.hpp>

#include <CameraActor.hpp>
#include <Execution/Renderer.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>
#include <Generic/GenericWindow.hpp>
#include <LogLumina.hpp>
#include <World/World.hpp>

#include <utility>

Lumen::Launch::FScopedMessageHandler
Lumen::InstallLuminaMessageHandler ( Launch::FLaunchApplicationModule &InApplicationModule,
                                     FWorld &InWorld,
                                     FWorldIngressJournal &InJournal,
                                     const ActorID InCamera,
                                     FRenderer &InRenderer ) noexcept
{
    return InApplicationModule.PushMessageHandler<FLuminaMessageHandler>( InWorld, InJournal, InCamera, InRenderer );
}

Lumen::FLuminaMessageHandler::FLuminaMessageHandler ( TSharedPtr<FGenericApplicationMessageHandler> InNextHandler,
                                                      FWorld &InWorld,
                                                      FWorldIngressJournal &InJournal,
                                                      const ActorID InCamera,
                                                      FRenderer &InRenderer ) noexcept
    : NextHandler( std::move( InNextHandler ) ), World( &InWorld ), Journal( &InJournal ), Camera( InCamera ),
      Renderer( &InRenderer )
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

void Lumen::FLuminaMessageHandler::ResetCamera () noexcept
{
    SendCameraMessage( FMessage::Make( static_cast<UInt32>( ELuminaCameraMessage::ResetCamera ), InvalidActorID ) );
}

void Lumen::FLuminaMessageHandler::OnRequestExit () noexcept
{
    if ( NextHandler.IsValid() )
    {
        NextHandler->OnRequestExit();
    }
}

void Lumen::FLuminaMessageHandler::OnQuit () noexcept
{
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
    if ( Renderer != nullptr and Size.X > 0 and Size.Y > 0 )
    {
        const RHI::FRHIExtent3D Extent{ static_cast<UInt32>( Size.X ), static_cast<UInt32>( Size.Y ), 1U };
        if ( not Renderer->RequestResize( Extent ) )
        {
            LUMEN_LOG_WARNING( LogLumina, "The renderer rejected a native resize request." );
        }
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
        const FLuminaOrbitDragMessage Drag{ static_cast<Float32>( InX - LastX ), static_cast<Float32>( InY - LastY ) };
        if ( Drag.DeltaX != 0.0F or Drag.DeltaY != 0.0F )
        {
            SendCameraMessage( FMessage::Make( static_cast<UInt32>( ELuminaCameraMessage::OrbitDrag ), InvalidActorID, Drag ) );
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
    const FLuminaOrbitZoomMessage Zoom{ static_cast<Float32>( InDelta ) };
    SendCameraMessage( FMessage::Make( static_cast<UInt32>( ELuminaCameraMessage::OrbitZoom ), InvalidActorID, Zoom ) );
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
