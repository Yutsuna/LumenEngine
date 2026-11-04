///
/// @file ActorMailbox.cpp
/// @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
///

#include <Actor/ActorMailbox.hpp>
#include <Logging/Logger.hpp>

#include <new>

Lumen::FMailBox::FMailBox () noexcept : FreeNodes( 256ULL ), Head( &Stub )
{
    Tail.store( &Stub, std::memory_order_relaxed );
}

Lumen::FMailBox::~FMailBox () noexcept
{
    ///
    /// INFO: Drain remaining nodes so no allocation leaks.
    /// The stub itself lives on the stack and must not be deleted.
    ///
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

    /// INFO: Clear the internal free list cache
    for ( ;; )
    {
        TOptional<FNode *> PooledNode = FreeNodes.Pop();
        if ( not PooledNode.HasValue() )
        {
            break;
        }
        delete *PooledNode;
    }
}

void Lumen::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    FNode *NewNode = nullptr;

    /// INFO: Try taking an existing node lock-free, allocate only if pool is depleted
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
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Push: failed to allocate node for new message" );
        return;
    }

    NewNode->Next.store( nullptr, std::memory_order_relaxed );

    /// INFO: Emplace handles destroy-then-construct in place, no assignment needed.
    NewNode->Message.Emplace( InMessage );

    ///
    /// INFO: exchange is the wait-free heart of Vyukov's algorithm.
    ///   release -> synchronises with the consumer's acquire on Next.
    ///
    FNode *const PrevTail = Tail.exchange( NewNode, std::memory_order_release );

    ///
    /// INFO: This store must be release to synchronize with the consumer's acquire on Next.
    ///
    PrevTail->Next.store( NewNode, std::memory_order_release );
}

Lumen::TOptional<Lumen::FMessage> Lumen::FMailBox::Pop () noexcept
{
    ///
    /// INFO: Head points to the stub (or a previously consumed node acting as stub).
    /// The real next message lives in Head->Next.
    ///
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

    TOptional<FMessage> Result = NextNode->Message;

    /// INFO: The old Head becomes the new stub; delete it unless it is the original stub.
    FNode *const OldHead = Head;
    Head                 = NextNode;

    if ( OldHead != &Stub )
    {
        /// INFO: Release the message before recycling the node.
        OldHead->Message.Reset();

        /// INFO: Return node to pool, fallback to OS deallocation if the queue is full
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
