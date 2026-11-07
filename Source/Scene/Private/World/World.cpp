/**
 * @file World.cpp
 * @brief Implementation of the FWorld class.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Actor/Actor.hpp>

#include <Messages/EngineMessageTypes.hpp>

#include <algorithm>
#include <execution>

namespace
{

LUMEN_FORCE_INLINE Lumen::Bool SortActiveActors ( const Lumen::TSharedPtr<Lumen::AActor> &A,
                                                  const Lumen::TSharedPtr<Lumen::AActor> &B ) noexcept
{
    return A->GetTypeID() < B->GetTypeID();
}

} // namespace

void Lumen::FWorld::Broadcast ( const FMessage &InMessage )
{
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( InMessage );
    }
}

void Lumen::FWorld::Tick ( const Float64 InDeltaTime )
{
    const FTickPayload Payload = FTickPayload{ .DeltaTime = InDeltaTime };
    const FMessage TickMsg     = FMessage::Make( EEngineMessage::Tick, 0ULL, Payload );

    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( TickMsg );
    }

    /**
     * NOTE: Instruction Cache Optimization
     * Sort actors by TypeIndex to group similar actor logic together, reducing I-Cache misses
     * when the parallel loop invokes the virtual Receive function.
     */
    if ( bNeedsSorting ) [[unlikely]]
    {
        std::ranges::sort( ActiveActors, &SortActiveActors );
        bNeedsSorting = false;
    }

    using FActorIterator            = TVector<TSharedPtr<AActor>>::iterator;
    const FActorIterator ActorBegin = ActiveActors.begin();
    const FActorIterator ActorEnd   = ActiveActors.end();

    /**
     * NOTE: Core parallel Mailbox evaluation of the Active Actors.
     * Throttled to 100 messages per tick to prevent "Long Tail" latency from busy actors.
     */
    std::for_each( std::execution::par, ActorBegin, ActorEnd, [InDeltaTime] ( const TSharedPtr<AActor> &Actor ) noexcept -> void
    {
        Actor->RunFrame( InDeltaTime, MaxMessagesPerTick );
    } );

    /** NOTE: Publish the spatial changes to the lock-free snapshot buffer */
    FSpatialRegistry::Get().Publish();
}
