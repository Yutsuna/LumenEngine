/**
 * @file ActorMailbox.cpp
 * @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
 */

#include <Actor/ActorMailbox.hpp>
#include <Logging/Logger.hpp>

#include <new>

/**
 * Public
 */

Lumen::FMailBox::FMailBox () noexcept : FreeNodes( 32ULL ), Head( &Stub )
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

Lumen::Bool Lumen::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    /**
     * INFO: Try taking an existing node lock-free; recover with an allocation
     *       only when the free-list pool is depleted. The result stays inside
     *       TOptional end-to-end — no null-pointer sentinel is materialized.
     */
    const TOptional<FNode *> Node = FreeNodes.Pop().OrElse( [] () noexcept -> TOptional<FNode *>
    {
        if ( FNode *const Allocated = new ( std::nothrow ) FNode() )
        {
            return TOptional<FNode *>( Allocated );
        }
        return TOptional<FNode *>{};
    } );

    if ( not Node.HasValue() ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Push: failed to allocate node for new message" );
        return false;
    }

    FNode *const NewNode = *Node;

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
    return true;
}

Lumen::TOptional<Lumen::FMessage> Lumen::FMailBox::Pop () noexcept
{
    /**
     * INFO: Tick fence: if the consumer has caught up to the boundary frozen
     *       at BeginTick, everything still in the queue was enqueued DURING
     *       the parallel phase and belongs to the next tick.
     */
    if ( Boundary != nullptr and Head == Boundary )
    {
        return {};
    }

    FNode *const NextNode = Head->Next.load( std::memory_order_acquire );

    if ( NextNode == nullptr )
    {
        return {};
    }

    if ( not NextNode->Message.HasValue() ) [[unlikely]]
    {
        FNode *const OldHead = std::exchange( Head, NextNode );
        RecycleHead( OldHead );
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Pop: message-less non-stub node (advanced to avoid wedge)" );
        return {};
    }

    TOptional<FMessage> Result = std::move( NextNode->Message );

    FNode *const OldHead = std::exchange( Head, NextNode );
    RecycleHead( OldHead );

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

void Lumen::FMailBox::BeginTick () noexcept
{
    Boundary = Tail.load( std::memory_order_acquire );
}

void Lumen::FMailBox::EndTick () noexcept
{
    Boundary = nullptr;
}

Lumen::Bool Lumen::FMailBox::IsEmpty () const noexcept
{
    return Head->Next.load( std::memory_order_acquire ) == nullptr;
}

/*
 * Private
 */

void Lumen::FMailBox::RecycleHead ( FNode *const InOldHead ) noexcept
{
    if ( InOldHead == &Stub ) [[unlikely]]
    {
        return;
    }

    InOldHead->Message.Reset();

    /** Return node to pool, fallback to OS deallocation if the queue is full */
    if ( not FreeNodes.Push( InOldHead ) )
    {
        delete InOldHead;
    }
}
