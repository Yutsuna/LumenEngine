/**
 * @file ActorMailbox.cpp
 * @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
 */

#include "Actor/ActorMailbox.hpp"
#include "Logging/Logger.hpp"

#include <new>

LumenEngine::FMailBox::FMailBox () noexcept : FreeNodes( 256ULL ), Head( &Stub )
{
    Tail.store( &Stub, std::memory_order_relaxed );
}

LumenEngine::FMailBox::~FMailBox () noexcept
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
    while ( TOptional<FNode *> PooledNode = FreeNodes.Pop() )
    {
        delete *PooledNode;
    }
}

void LumenEngine::FMailBox::Push ( const FMessage &InMessage ) noexcept
{
    FNode *NewNode = nullptr;

    /** Try taking an existing node lock-free, allocate only if pool is depleted */
    if ( TOptional<FNode *> PooledNode = FreeNodes.Pop() )
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
    NewNode->Message = InMessage;

    /**
     * INFO: exchange is the wait-free heart of Vyukov's algorithm.
     *   release -> synchronises with the consumer's acquire on Next.
     */
    FNode *const PrevTail = Tail.exchange( NewNode, std::memory_order_release );

    /**
     * INFO: This store must be release to synchronize with the consumer's acquire on Next.
     */
    PrevTail->Next.store( NewNode, std::memory_order_release );
}

LumenEngine::TOptional<LumenEngine::FMessage> LumenEngine::FMailBox::Pop () noexcept
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

    if ( not NextNode->Message.has_value() )
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Pop: node has no message (stub leaked?)" );
        return {};
    }

    TOptional<FMessage> Result = NextNode->Message;

    /** INFO: The old Head becomes the new stub; delete it unless it is the original stub. */
    FNode *const OldHead = Head;
    Head                 = NextNode;

    if ( OldHead != &Stub )
    {
        OldHead->Message.reset();

        /** Return node to pool, fallback to OS deallocation if the queue is full */
        if ( not FreeNodes.Push( OldHead ) )
        {
            delete OldHead;
        }
    }

    return Result;
}

LumenEngine::Bool LumenEngine::FMailBox::IsEmpty () const noexcept
{
    return Head->Next.load( std::memory_order_acquire ) == nullptr;
}