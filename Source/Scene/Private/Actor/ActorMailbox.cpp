/**
 * @file ActorMailbox.cpp
 * @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
 */

#include <Actor/ActorMailbox.hpp>
#include <Logging/Logger.hpp>

#include <new>

Lumen::FMailBox::FMailBox () noexcept : FreeNodes( 256ULL ), Head( &Stub )
{
    Tail.store( &Stub, std::memory_order_relaxed );
}

Lumen::FMailBox::~FMailBox () noexcept
{
    /**
     * INFO: Drain remaining nodes so no allocation leaks.
     * The stub itself lives on the stack and must not be deleted.
     */
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

    /** Clear the internal free list cache */
    for ( ;; )
    {
        TOptional<FNode *> Pooled = FreeNodes.Pop();

        if ( not Pooled.HasValue() )
        {
            break;
        }

        delete *Pooled;
    }
}

void Lumen::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    /**
     * INFO: Try taking an existing node lock-free, allocate only if pool is depleted.
     * OrElse performs the allocation as a monadic recovery when Pop() is empty.
     */
    FNode *const NewNode = FreeNodes.Pop()
                               .OrElse(
                                   [] () noexcept -> TOptional<FNode *>
                                   {
                                       if ( FNode *const Allocated = new ( std::nothrow ) FNode() )
                                       {
                                           return TOptional<FNode *>( Allocated );
                                       }
                                       return TOptional<FNode *>{};
                                   } )
                               .ValueOr( nullptr );

    if ( NewNode == nullptr )
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Push: failed to allocate node for new message" );
        return;
    }

    NewNode->Next.store( nullptr, std::memory_order_relaxed );
    NewNode->Message.Emplace( InMessage );

    /**
     * INFO: the exchange synchronises with the consumer's acquire on Next
     */
    FNode *const PrevTail = Tail.exchange( NewNode, std::memory_order_acq_rel );

    /**
     * INFO: this store must be release to synchronize with the consumer's acquire on Next
     */
    PrevTail->Next.store( NewNode, std::memory_order_release );
}

Lumen::TOptional<Lumen::FMessage> Lumen::FMailBox::Pop () noexcept
{
    /**
     * INFO: Head points to the stub (or a previously consumed node acting as stub).
     * The real next message lives in Head->Next.
     */
    FNode *const NextNode = Head->Next.load( std::memory_order_acquire );

    if ( NextNode == nullptr )
    {
        return {};
    }

    if ( not NextNode->Message.HasValue() )
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Pop: node has no message (stub leaked?)" );
        return {};
    }

    /** INFO: Transfer ownership of the message out of the node into the result. */
    TOptional<FMessage> Result = std::move( NextNode->Message );

    /** INFO: The old Head becomes the new stub; delete it unless it is the original stub. */
    FNode *const OldHead = Head;
    Head                 = NextNode;

    if ( OldHead != &Stub )
    {
        OldHead->Message.Reset();

        /** Return node to pool, fallback to OS deallocation if the queue is full */
        if ( not FreeNodes.Push( OldHead ) )
        {
            delete OldHead;
        }
    }

    return Result;
}

void Lumen::FMailBox::Reserve ( USize InCapacity ) noexcept
{
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
}

Lumen::Bool Lumen::FMailBox::IsEmpty () const noexcept
{
    return Head->Next.load( std::memory_order_acquire ) == nullptr;
}
