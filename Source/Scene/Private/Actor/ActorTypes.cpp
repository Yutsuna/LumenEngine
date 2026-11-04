///
/// @file ActorTypes.cpp
/// @brief Implementation of actor types for parallel processing in Lumen.
///

#include "ActorRegistry.hpp"
#include <Actor/Actor.hpp>

const Lumen::FLogCategory Lumen::LogActor( "Actor" );

Lumen::FActorRef::FActorRef ( const ActorID InId ) noexcept : Id( InId )
{
}

Lumen::FActorRef::FActorRef ( AActor *InActor ) noexcept : Id( InActor != nullptr ? InActor->GetId() : ActorID{} )
{
}

Lumen::ActorID Lumen::FActorRef::GetId () const noexcept
{
    return Id;
}

Lumen::Bool Lumen::FActorRef::IsValid () const noexcept
{
    return Id.IsValid();
}

Lumen::EEnqueueResult Lumen::FActorRef::EnqueueMessage ( const FMessage &InMessage ) const noexcept
{
    return Internal::FActorRegistry::EnqueueMessage( Id, InMessage );
}
