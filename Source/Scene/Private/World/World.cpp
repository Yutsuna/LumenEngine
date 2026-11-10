/**
 * @file World.cpp
 * @brief Implementation of the FWorld class.
 */

#include <World/World.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Algo/Sort.hpp>

#include <Actor/Actor.hpp>

#include <execution>

/**
 * Private
 */

namespace
{

LUMEN_FORCE_INLINE Lumen::Bool SortActiveActors ( const Lumen::TSharedPtr<Lumen::AActor> &InActorA,
                                                  const Lumen::TSharedPtr<Lumen::AActor> &InActorB ) noexcept
{
    const Lumen::USize ActorIDA = InActorA->GetTypeID();
    const Lumen::USize ActorIDB = InActorB->GetTypeID();

    return ActorIDA != ActorIDB ? ActorIDA < ActorIDB : InActorA->GetId() < InActorB->GetId();
}

} // namespace

/**
 * Public
 */

Lumen::FWorld::~FWorld () noexcept
{
    TUniqueLock<FMutex> Lock( DeferredMutex );

    for ( const FDeferredCommand &Command : DeferredCommands )
    {
        if ( Command.OwnedId != InvalidActorID )
        {
            Internal::FActorRegistry::ReleaseId( Command.OwnedId );
        }
    }
}

void Lumen::FWorld::Broadcast ( const FMessage &InMessage )
{
    if ( bInParallelPhase )
    {
        FDeferredCommand Command = { .OwnedId = 0UL,
                                     .Execute = [Msg = InMessage] ( FWorld &World ) -> void
        {
            World.Broadcast( Msg );
        } };

        TUniqueLock<FMutex> Lock( DeferredMutex );
        DeferredCommands.push_back( std::move( Command ) );
        return;
    }

    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EnqueueMessage( InMessage );
    }
}

void Lumen::FWorld::DestroyActor ( ActorID InId )
{
    if ( bInParallelPhase )
    {
        FDeferredCommand Command = { .OwnedId = InId,
                                     .Execute = [InId] ( FWorld &World ) -> void
        {
            World.DestroyActorImpl( InId );
        } };

        TUniqueLock<FMutex> Lock( DeferredMutex );
        DeferredCommands.push_back( std::move( Command ) );
        return;
    }

    DestroyActorImpl( InId );
}

void Lumen::FWorld::Tick ( const Float64 InDeltaTime )
{
    /** NOTE: Apply deferred structural commands serially. */
    ApplyDeferredCommands();

    /** NOTE: Re-sort only after a structural change. */
    if ( bNeedsSorting ) [[unlikely]]
    {
        Lumen::Algo::Sort( ActiveActors, &SortActiveActors );
        bNeedsSorting = false;
    }

    /**
     * NOTE: Freeze the mailbox tick boundary for every actor.
     *       Any message enqueued during the parallel phase is deferred to
     *       the next frame (deterministic per-producer delivery order).
     */
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->BeginTick();
    }

    /** NOTE: Parallel RunFrame: process mailbox, then OnTick. */
    bInParallelPhase = true;

    using FActiveActorIterator            = decltype( ActiveActors )::iterator;
    const FActiveActorIterator ActorBegin = ActiveActors.begin();
    const FActiveActorIterator ActorEnd   = ActiveActors.end();

    std::for_each( std::execution::par, ActorBegin, ActorEnd, [InDeltaTime] ( const TSharedPtr<AActor> &Actor ) noexcept -> void
    {
        Actor->RunFrame( InDeltaTime, MaxMessagesPerTick );
    } );

    bInParallelPhase = false;

    /** NOTE: Clear the fences so external RunFrame calls behave as before. */
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->EndTick();
    }

    /** NOTE: Publish the spatial snapshot. */
    FSpatialRegistry::Get().Publish();
}

Lumen::Bool Lumen::FWorld::IsInParallelPhase () const noexcept
{
    return bInParallelPhase;
}

/**
 * Private
 */

void Lumen::FWorld::ApplyDeferredCommands () noexcept
{
    /**
     * NOTE: Swap the queue out first so commands that themselves enqueue
     *       deferred work (DestroyActor from a deferred Broadcast handler, ...)
     *       land in a fresh queue and get applied next tick. Prevents
     *       unbounded recursion inside a single serial phase.
     */
    decltype( DeferredCommands ) Batch;
    {
        TUniqueLock<FMutex> Lock( DeferredMutex );
        Batch = std::move( DeferredCommands );
        DeferredCommands.clear();
    }

    for ( FDeferredCommand &Command : Batch )
    {
        Command.Execute( *this );
    }
}

void Lumen::FWorld::DestroyActorImpl ( ActorID InId ) noexcept
{
    using FActorIterator    = decltype( Actors )::iterator;
    const FActorIterator It = Actors.find( InId );
    if ( It == Actors.end() )
    {
        return;
    }

    /**
     * NOTE: Unregister the spatial entity before the actor leaves the world
     *       containers, so FSpatialRegistry never observes a stale id on
     *       the next Publish.
     */
    FSpatialRegistry::Get().UnregisterSpatialEntity( InId );

    Actors.erase( It );

    using FActiveActorIterator = decltype( ActiveActors )::iterator;
    for ( FActiveActorIterator VIt = ActiveActors.begin(); VIt != ActiveActors.end(); ++VIt )
    {
        if ( ( *VIt )->GetId() == InId )
        {
            ActiveActors.erase( VIt );
            break;
        }
    }

    bNeedsSorting = true;
}
