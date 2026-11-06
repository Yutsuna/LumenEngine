/**
 * @file ActorTypes.cpp
 * @brief Implementation of actor types.
 */

#include "ActorRegistry.hpp"
#include <Actor/Actor.hpp>

const Lumen::FLogCategory Lumen::LogActor( "Actor" );

Lumen::FActorRef::FActorRef ( ActorID InId ) noexcept : Id( InId )
{
    /* */
}

Lumen::FActorRef::FActorRef ( AActor *InActor ) noexcept : Id( InActor != nullptr ? InActor->GetId() : 0ULL )
{
    /* */
}

Lumen::ActorID Lumen::FActorRef::GetId () const noexcept
{
    return Id;
}

Lumen::Bool Lumen::FActorRef::IsValid () const noexcept
{
    return Id != 0ULL;
}

void Lumen::FActorRef::EnqueueMessage ( const FMessage &InMessage ) const noexcept
{
    Lumen::Internal::FActorRegistry::EnqueueMessage( Id, InMessage );
}
