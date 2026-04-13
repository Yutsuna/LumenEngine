/**
 * @file CameraActor.cpp
 * @brief Implementation of the FCameraActor base class.
 */

#include "Actors/CameraActor.hpp"
#include "Messages/EngineMessageTypes.hpp"

#include "Container/Any.hpp"

LumenEngine::Engine::FCameraActor::FCameraActor ( const ActorID InId ) noexcept : AActor( InId )
{
    /* Ctor */
}

void LumenEngine::Engine::FCameraActor::Receive ( FMessage InMessage )
{
    if ( InMessage.Type == EEngineMessage::Tick )
    {
        const FTickPayload Payload = TAnyCast<FTickPayload>( InMessage.Payload );
        Tick( Payload.DeltaTime );

        Camera.Tick( Payload.DeltaTime );
    }
    else
    {
        HandleMessage( InMessage );
    }
}

void LumenEngine::Engine::FCameraActor::HandleMessage ( const FMessage & /*InMessage*/ )
{
    /** NOTE: Override in derived classes for specific logic (like InputAxis handling) */
}

const LumenEngine::Maths::FCamera &LumenEngine::Engine::FCameraActor::GetCamera () const noexcept
{
    return Camera;
}