///
/// @file ActorMailbox.cpp
/// @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
///

#include <Actor/ActorMailbox.hpp>
#include <Logging/Logger.hpp>

#include <new>
#include <thread>

Lumen::FMailBox::FMailBox ( const USize InMaxMessages, const USize InPreallocatedNodes ) noexcept
    : FreeNodes( PoolCapacity ), Head( &Stub ), MaxMessages( InMaxMessages )
{
    Tail.store( &Stub, std::memory_order_relaxed );
    Reserve( InPreallocatedNodes );
}

Lumen::FMailBox::~FMailBox () noexcept
{
    Close();
}

Lumen::EEnqueueResult Lumen::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    if ( not TryBeginOperation() )
    {
        return EEnqueueResult::Closed;
    }

    if ( MaxMessages != 0ULL )
    {
        USize Count = MessageCount.load( std::memory_order_relaxed );
        for ( ;; )
        {
            if ( Count >= MaxMessages )
            {
                EndOperation();
                return EEnqueueResult::Full;
            }

            if ( MessageCount.compare_exchange_weak( Count, Count + 1ULL, std::memory_order_relaxed ) )
            {
                break;
            }
        }
    }

    FNode *NewNode = nullptr;
    if ( TOptional<FNode *> PooledNode = FreeNodes.Pop(); PooledNode.HasValue() )
    {
        NewNode = *PooledNode;
    }
    else
    {
        NewNode = new ( std::nothrow ) FNode();
    }

    if ( NewNode == nullptr )
    {
        if ( MaxMessages != 0ULL )
        {
            MessageCount.fetch_sub( 1ULL, std::memory_order_relaxed );
        }
        EndOperation();
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Push: failed to allocate node for new message" );
        return EEnqueueResult::AllocationFailure;
    }

    NewNode->Next.store( nullptr, std::memory_order_relaxed );
    NewNode->Message.Emplace( InMessage );

    /// The acquire half synchronizes with the previous producer's exchange, publishing that node's initialization.
    /// The release half passes the initialized node to the next producer. The consumer never accesses an unlinked tail.
    FNode *const PrevTail = Tail.exchange( NewNode, std::memory_order_acq_rel );

    /// This release publishes the complete node and links it to the consumer's acquire load below.
    /// Until this store, Head stops at PrevTail, so no consumer can recycle PrevTail during the store.
    PrevTail->Next.store( NewNode, std::memory_order_release );

    EndOperation();
    return EEnqueueResult::Success;
}

Lumen::TOptional<Lumen::FMessage> Lumen::FMailBox::Pop () noexcept
{
    if ( not TryBeginOperation() )
    {
        return {};
    }

    FNode *const NextNode = Head->Next.load( std::memory_order_acquire );
    if ( NextNode == nullptr )
    {
        EndOperation();
        return {};
    }

    if ( not NextNode->Message.HasValue() )
    {
        EndOperation();
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Pop: node has no message (stub leaked?)" );
        return {};
    }

    TOptional<FMessage> Result = NextNode->Message;

    FNode *const OldHead = Head;
    Head                 = NextNode;

    if ( OldHead != &Stub )
    {
        OldHead->Message.Reset();

        /// Reserve and Pop are serialized pool producers; message producers and the consumer may concurrently pop nodes.
        if ( not FreeNodes.Push( OldHead ) )
        {
            delete OldHead;
        }
    }

    if ( MaxMessages != 0ULL )
    {
        MessageCount.fetch_sub( 1ULL, std::memory_order_relaxed );
    }
    EndOperation();
    return Result;
}

void Lumen::FMailBox::Reserve ( const USize InCapacity ) noexcept
{
    if ( not TryBeginOperation() )
    {
        return;
    }

    for ( USize Index = 0ULL; Index < InCapacity; ++Index )
    {
        FNode *const NewNode = new ( std::nothrow ) FNode();
        if ( NewNode == nullptr )
        {
            break;
        }

        if ( not FreeNodes.Push( NewNode ) )
        {
            delete NewNode;
            break;
        }
    }

    EndOperation();
}

void Lumen::FMailBox::Close () noexcept
{
    TUniqueLock<FMutex> Lock( CloseMutex );
    if ( bResourcesReleased )
    {
        return;
    }

    OperationState.fetch_or( ClosedFlag, std::memory_order_acq_rel );
    while ( ( OperationState.load( std::memory_order_acquire ) & ~ClosedFlag ) != 0U )
    {
        std::this_thread::yield();
    }

    /// Closed prevents new operations and the wait above includes both node publication and consumer recycling.
    /// The linked list and the free list are therefore exclusively owned here.
    FNode *CurrentNode = Head;
    while ( CurrentNode != nullptr )
    {
        FNode *const NextNode = CurrentNode->Next.load( std::memory_order_relaxed );
        if ( CurrentNode != &Stub )
        {
            delete CurrentNode;
        }
        CurrentNode = NextNode;
    }

    for ( ;; )
    {
        TOptional<FNode *> PooledNode = FreeNodes.Pop();
        if ( not PooledNode.HasValue() )
        {
            break;
        }
        delete *PooledNode;
    }

    Head = &Stub;
    Stub.Next.store( nullptr, std::memory_order_relaxed );
    Tail.store( &Stub, std::memory_order_relaxed );
    MessageCount.store( 0ULL, std::memory_order_relaxed );
    bResourcesReleased = true;
}

Lumen::Bool Lumen::FMailBox::IsEmpty () const noexcept
{
    if ( not TryBeginOperation() )
    {
        return true;
    }

    const Bool bIsEmpty = Head->Next.load( std::memory_order_acquire ) == nullptr;
    EndOperation();
    return bIsEmpty;
}

Lumen::Bool Lumen::FMailBox::IsClosed () const noexcept
{
    return ( OperationState.load( std::memory_order_acquire ) & ClosedFlag ) != 0U;
}

Lumen::Bool Lumen::FMailBox::TryBeginOperation () const noexcept
{
    UInt32 State = OperationState.load( std::memory_order_relaxed );
    while ( ( State & ClosedFlag ) == 0U )
    {
        if ( OperationState.compare_exchange_weak( State, State + OperationIncrement, std::memory_order_acq_rel ) )
        {
            return true;
        }
    }

    return false;
}

void Lumen::FMailBox::EndOperation () const noexcept
{
    OperationState.fetch_sub( OperationIncrement, std::memory_order_release );
}
