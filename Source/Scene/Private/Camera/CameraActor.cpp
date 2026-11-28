/**
 * @file CameraActor.cpp
 * @brief Implements the generic camera message adapter.
 */

#include <Camera/CameraActor.hpp>

#include <Maths/Macros.hpp>
#include <Messages/EngineMessageTypes.hpp>
#include <Templates/Unexpected.hpp>

/**
 * Public
 */

Lumen::ACameraActor::ACameraActor ( const ActorID InId,
                                    FSpatialRegistry &InRegistry,
                                    FCameraComponentRegistry &InCameras ) noexcept
    : AActor( InId ), Registry( &InRegistry ), Cameras( &InCameras )
{
    /* */
}

void Lumen::ACameraActor::Receive ( const FMessage &InMessage ) noexcept
{
    switch ( static_cast<EEngineMessage::Type>( InMessage.Type ) )
    {

    case EEngineMessage::CameraSetPose:
    {
        PublishPose( InMessage.GetPayload<FCameraPosePayload>().WorldTransform );
        break;
    }

    case EEngineMessage::CameraSetLookAt:
    {
        const FCameraLookAtPayload LookAt = InMessage.GetPayload<FCameraLookAtPayload>();
        const Maths::FVec3f Forward       = ( LookAt.Target - LookAt.Eye ).Normalize();
        const Maths::FVec3f Right         = LookAt.Up.Cross( Forward ).Normalize();
        const Maths::FVec3f Up            = Forward.Cross( Right );

        /** INFO: LookAt stores R, U, F as rows. The rigid inverse stores them as columns plus Eye in column 3. */
        Maths::FMat4x4f WorldTransform = Maths::FMat4x4f::Identity();
        WorldTransform[0].Data[0]      = Right.X;
        WorldTransform[0].Data[1]      = Right.Y;
        WorldTransform[0].Data[2]      = Right.Z;
        WorldTransform[1].Data[0]      = Up.X;
        WorldTransform[1].Data[1]      = Up.Y;
        WorldTransform[1].Data[2]      = Up.Z;
        WorldTransform[2].Data[0]      = Forward.X;
        WorldTransform[2].Data[1]      = Forward.Y;
        WorldTransform[2].Data[2]      = Forward.Z;
        WorldTransform[3].Data[0]      = LookAt.Eye.X;
        WorldTransform[3].Data[1]      = LookAt.Eye.Y;
        WorldTransform[3].Data[2]      = LookAt.Eye.Z;

        PublishPose( WorldTransform );
        break;
    }
    case EEngineMessage::CameraSetParams:
    {
        const FCameraParamsPayload Params = InMessage.GetPayload<FCameraParamsPayload>();
        const Float32 FovRadians          = Params.FieldOfViewDegrees * Maths::DegToRad<Float32>;
        const Maths::FMat4x4f Projection =
            Maths::FMat4x4f::Perspective( FovRadians, Params.AspectRatio, Params.NearPlane, Params.FarPlane );
        static_cast<void>( Cameras->TrySetProjection( GetId(), Projection ) );
        static_cast<void>( Cameras->TrySetClipping( GetId(), Params.NearPlane, Params.FarPlane, FRenderViewport{} ) );
        break;
    }
    case EEngineMessage::CameraSetActive:
    {
        const FCameraActivePayload Active = InMessage.GetPayload<FCameraActivePayload>();
        const ERenderCameraFlags Flags    = Active.bActive ? ERenderCameraFlags::Active : ERenderCameraFlags::None;
        static_cast<void>( Cameras->TrySetFlags( GetId(), Flags ) );
        break;
    }
    default:
        break;
    }
}

void Lumen::ACameraActor::PublishPose ( const Maths::FMat4x4f &InWorldTransform ) noexcept
{
    const Maths::FVec3f Eye     = { InWorldTransform[3].Data[0], InWorldTransform[3].Data[1], InWorldTransform[3].Data[2] };
    const Maths::FVec3f Forward = { InWorldTransform[2].Data[0], InWorldTransform[2].Data[1], InWorldTransform[2].Data[2] };
    const Maths::FVec3f Up      = { InWorldTransform[1].Data[0], InWorldTransform[1].Data[1], InWorldTransform[1].Data[2] };
    const Maths::FMat4x4f View  = Maths::FMat4x4f::LookAt( Eye, Eye + Forward, Up );

    Registry->UpdateTransform( GetId(), InWorldTransform );
    static_cast<void>( Cameras->TrySetView( GetId(), View ) );
}
