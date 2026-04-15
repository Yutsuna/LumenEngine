/**
 * @file World.cpp
 * @brief Implementation of the FWorld class.
 */

#include "World/World.hpp"
#include "Messages/EngineMessageTypes.hpp"
#include "World/SpatialRegistry.hpp"

#include <algorithm>
#include <execution>

void LumenEngine::Engine::FWorld::Broadcast ( const FMessage &InMessage )
{
    for ( const auto &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( InMessage );
    }
}

void LumenEngine::Engine::FWorld::Tick ( const Float64 InDeltaTime )
{
    const FTickPayload Payload{ .DeltaTime = InDeltaTime };
    const FMessage TickMsg = FMessage::Make( EEngineMessage::Tick, 0ULL, Payload );

    for ( const auto &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( TickMsg );
    }

    using ActorIterator            = TVector<TSharedPtr<AActor>>::iterator;
    const ActorIterator ActorBegin = ActiveActors.begin();
    const ActorIterator ActorEnd   = ActiveActors.end();

    /** NOTE: Core parallel Mailbox evaluation of the Active Actors */
    std::for_each( std::execution::par_unseq, ActorBegin, ActorEnd, [] ( const TSharedPtr<AActor> &InActor ) -> void { InActor->ProcessMailbox(); } );

    /** NOTE: Publish the spatial changes to the lock-free snapshot buffer */
    FSpatialRegistry::Get().Publish();
}
