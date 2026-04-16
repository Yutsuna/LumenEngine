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
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( InMessage );
    }
}

void LumenEngine::Engine::FWorld::Tick ( const Float64 InDeltaTime )
{
    const FTickPayload Payload{ .DeltaTime = InDeltaTime };
    const FMessage TickMsg = FMessage::Make( EEngineMessage::Tick, 0ULL, Payload );

    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( TickMsg );
    }

    /**
     * NOTE: Instruction Cache Optimization
     * Sort actors by TypeIndex to group similar actor logic together, reducing I-Cache misses
     * when the parallel loop invokes the virtual Receive function.
     * Pure C++ automatic sorting via RTTI.
     */
    if ( bNeedsSorting ) [[unlikely]]
    {
        std::ranges::sort( ActiveActors, [] ( const TSharedPtr<AActor> &A, const TSharedPtr<AActor> &B ) { return A->GetTypeIndex() < B->GetTypeIndex(); } );
        bNeedsSorting = false;
    }

    using ActorIterator            = TVector<TSharedPtr<AActor>>::iterator;
    const ActorIterator ActorBegin = ActiveActors.begin();
    const ActorIterator ActorEnd   = ActiveActors.end();

    /**
     * NOTE: Core parallel Mailbox evaluation of the Active Actors.
     * Throttled to 100 messages per tick to prevent "Long Tail" latency from busy actors.
     */
    std::for_each( std::execution::par_unseq, ActorBegin, ActorEnd, [] ( const TSharedPtr<AActor> &InActor ) -> void { InActor->ProcessMailbox( MaxMessagesPerTick ); } );

    /** NOTE: Publish the spatial changes to the lock-free snapshot buffer */
    FSpatialRegistry::Get().Publish();
}
