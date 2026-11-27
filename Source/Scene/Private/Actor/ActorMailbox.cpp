/**
 * @file ActorMailbox.cpp
 * @brief Implementation of the bounded FMailBox over Core's preallocated MPSC queue.
 */

#include <Actor/ActorMailbox.hpp>

#include <Logging/Logger.hpp>

#include <Templates/Unexpected.hpp>

#include <utility>

/**
 * Public
 */

Lumen::FMailBox::FMailBox () noexcept : FMailBox( FMailBoxDesc{} )
{
}

Lumen::FMailBox::FMailBox ( const FMailBoxDesc &InDesc ) noexcept
{
    const TExpected<void, EActorMailboxError> Result = Initialize( InDesc.Capacity );

    if ( not Result ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox: failed to reserve {} message slots (error {})", InDesc.Capacity,
                         static_cast<UInt32>( Result.Error() ) );
    }
}

Lumen::Bool Lumen::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    const TExpected<void, EActorSendError> Result = TryPush( InMessage );

    if ( not Result ) [[unlikely]]
    {
        LUMEN_LOG_WARNING( LogActor, "FMailBox::Push: mailbox full, message rejected" );
        return false;
    }

    return true;
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::FMailBox::TryPush ( const FMessage &InMessage ) noexcept
{
    if ( not Queue.IsValid() or not Queue->IsInitialized() ) [[unlikely]]
    {
        SaturationCount.fetch_add( 1ULL, std::memory_order_relaxed );
        return MakeUnexpected( EActorSendError::MailboxFull );
    }

    const TExpected<void, Thread::EMPSCQueueError> Result = Queue->Push( InMessage );

    if ( not Result ) [[unlikely]]
    {
        SaturationCount.fetch_add( 1ULL, std::memory_order_relaxed );
        return MakeUnexpected( EActorSendError::MailboxFull );
    }

    /** Published after the queue link so BeginTick never counts a message that is not yet visible. */
    AcceptedCount.fetch_add( 1ULL, std::memory_order_release );
    return {};
}

Lumen::TOptional<Lumen::FMessage> Lumen::FMailBox::Pop () noexcept
{
    const USize CurrentBoundary = Boundary.load( std::memory_order_relaxed );

    /**
     * INFO: Tick fence — messages accepted after BeginTick belong to the next tick, even when the underlying
     *       queue already exposes them. ConsumedCount is consumer-owned, so this check cannot race.
     */
    if ( CurrentBoundary != NoBoundary and ConsumedCount >= CurrentBoundary )
    {
        return {};
    }

    if ( not Queue.IsValid() ) [[unlikely]]
    {
        return {};
    }

    const TOptional<FMessage> Result = Queue->Pop();

    if ( not Result.HasValue() )
    {
        return {};
    }

    ++ConsumedCount;
    return Result;
}

Lumen::TExpected<void, Lumen::EActorMailboxError> Lumen::FMailBox::Reserve ( const USize InCapacity ) noexcept
{
    /**
     * INFO: Reserve is a serial control operation. Enlarging replaces the queue and transfers pending messages,
     *       so the accepted/consumed counters stay valid across the swap.
     */
    if ( Queue.IsValid() and Queue->GetCapacity() >= InCapacity )
    {
        return {};
    }

    return Initialize( InCapacity );
}

void Lumen::FMailBox::BeginTick () noexcept
{
    Boundary.store( AcceptedCount.load( std::memory_order_acquire ), std::memory_order_relaxed );
}

void Lumen::FMailBox::EndTick () noexcept
{
    Boundary.store( NoBoundary, std::memory_order_relaxed );
}

Lumen::Bool Lumen::FMailBox::IsEmpty () const noexcept
{
    return not Queue.IsValid() or Queue->IsEmpty();
}

Lumen::USize Lumen::FMailBox::GetCapacity () const noexcept
{
    return Queue.IsValid() ? Queue->GetCapacity() : 0ULL;
}

Lumen::UInt64 Lumen::FMailBox::GetSaturationCount () const noexcept
{
    return SaturationCount.load( std::memory_order_relaxed );
}

Lumen::USize Lumen::FMailBox::GetPendingCount () const noexcept
{
    const USize Accepted = AcceptedCount.load( std::memory_order_acquire );
    return Accepted > ConsumedCount ? Accepted - ConsumedCount : 0ULL;
}

/**
 * Private
 */

Lumen::TExpected<void, Lumen::EActorMailboxError> Lumen::FMailBox::Initialize ( const USize InCapacity ) noexcept
{
    TUniquePtr<Thread::TMPSCQueue<FMessage>> NewQueue = MakeUnique<Thread::TMPSCQueue<FMessage>>();

    if ( not NewQueue.IsValid() ) [[unlikely]]
    {
        return MakeUnexpected( EActorMailboxError::OutOfMemory );
    }

    const TExpected<void, Thread::EMPSCQueueError> Result = NewQueue->Initialize( InCapacity );

    if ( not Result ) [[unlikely]]
    {
        const EActorMailboxError Error = Result.Error() == Thread::EMPSCQueueError::OutOfMemory
                                             ? EActorMailboxError::OutOfMemory
                                             : EActorMailboxError::InvalidCapacity;
        return MakeUnexpected( Error );
    }

    if ( Queue.IsValid() )
    {
        while ( const TOptional<FMessage> Item = Queue->Pop() )
        {
            static_cast<void>( NewQueue->Push( *Item ) );
        }
    }

    Queue = std::move( NewQueue );
    return {};
}
