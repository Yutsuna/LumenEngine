/**
 * @file CameraActor.cpp
 * @brief Implementation of the ACameraActor base class.
 */

#include "Actors/CameraActor.hpp"
#include "Messages/EngineMessageTypes.hpp"

LumenEngine::Engine::ACameraActor::ACameraActor ( const ActorID InId ) noexcept : ASpatialActor( InId )
{
    /* Ctor */
}

void LumenEngine::Engine::ACameraActor::Receive ( const FMessage &InMessage )
{
    if ( InMessage.Type == EEngineMessage::Tick )
    {
        const FTickPayload &Payload = InMessage.GetPayload<FTickPayload>();

        Tick( Payload.DeltaTime );
        Camera.Tick( Payload.DeltaTime );
    }
    else
    {
        HandleMessage( InMessage );
    }
}

void LumenEngine::Engine::ACameraActor::HandleMessage ( const FMessage & /*InMessage*/ )
{
    /** NOTE: Override in derived classes for specific logic */
}

const LumenEngine::Maths::FCamera &LumenEngine::Engine::ACameraActor::GetCamera () const noexcept
{
    return Camera;
}
