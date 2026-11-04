///
/// @file ActorTypes.cpp
/// @brief Implementation of actor types for parallel processing in Lumen.
///

#include "ActorRegistry.hpp"
#include <Actor/Actor.hpp>

const Lumen::FLogCategory Lumen::LogActor( "Actor" );

Lumen::FActorRef::FActorRef ( ActorID InId ) noexcept : Id( InId )
{
    /// Empty
}

Lumen::FActorRef::FActorRef ( AActor *InActor ) noexcept : Id( InActor != nullptr ? InActor->GetId() : 0ULL )
{
    /// Empty
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
    Internal::FActorRegistry::EnqueueMessage( Id, InMessage );
}
