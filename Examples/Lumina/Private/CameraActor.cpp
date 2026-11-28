/**
 * @file CameraActor.cpp
 * @brief Implements actor-local camera input and transform publication.
 */

#include <CameraActor.hpp>

#include <Maths/Matrix.hpp>

Lumen::ALuminaCameraActor::ALuminaCameraActor ( const ActorID InId, FSpatialRegistry &InRegistry ) noexcept
    : AActor( InId ), Registry( &InRegistry )
{
    /* */
}

void Lumen::ALuminaCameraActor::Receive ( const FMessage &InMessage ) noexcept
{
    switch ( static_cast<ELuminaCameraMessage>( InMessage.Type ) )
    {
    case ELuminaCameraMessage::OrbitDrag:
    {
        const FLuminaOrbitDragMessage Drag = InMessage.GetPayload<FLuminaOrbitDragMessage>();
        State = ApplyOrbitDrag( State, Drag.DeltaX, Drag.DeltaY );
        PublishTransform();
        break;
    }
    case ELuminaCameraMessage::OrbitZoom:
    {
        const FLuminaOrbitZoomMessage Zoom = InMessage.GetPayload<FLuminaOrbitZoomMessage>();
        State = ApplyOrbitZoom( State, Zoom.Delta );
        PublishTransform();
        break;
    }
    case ELuminaCameraMessage::ResetCamera:
        State = FOrbitCameraState{};
        PublishTransform();
        break;
    default:
        break;
    }
}

void Lumen::ALuminaCameraActor::OnBeginPlay () noexcept
{
    PublishTransform();
}

void Lumen::ALuminaCameraActor::PublishTransform () noexcept
{
    if ( Registry != nullptr )
    {
        Registry->UpdateTransform( GetId(), Maths::FMat4x4f::Translate( ComputeOrbitCamera( State ).Position ) );
    }
}
