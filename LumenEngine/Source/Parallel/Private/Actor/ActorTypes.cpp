/**
 * @file ActorTypes.cpp
 * @brief Implementation of actor types for parallel processing in Lumen Engine.
 */

#include "Actor/Actor.hpp"
#include "ActorRegistry.hpp"

const LumenEngine::FLogCategory LumenEngine::LogActor( "Actor" );

LumenEngine::FActorRef::FActorRef ( ActorID InId ) noexcept : Id( InId )
{
    /* */
}

LumenEngine::FActorRef::FActorRef ( AActor *InActor ) noexcept : Id( InActor != nullptr ? InActor->GetId() : 0ULL )
{
    /* */
}

LumenEngine::ActorID LumenEngine::FActorRef::GetId () const noexcept
{
    return Id;
}

LumenEngine::Bool LumenEngine::FActorRef::IsValid () const noexcept
{
    return Id != 0ULL;
}

void LumenEngine::FActorRef::EnqueueMessage ( const FMessage &InMessage ) const noexcept
{
    LumenEngine::Internal::FActorRegistry::EnqueueMessage( Id, InMessage );
}