/**
 * @file ActorMailbox.cpp
 * @brief Implementation of the FMailBox class based on the Vyukov MPSC queue.
 */

#include "Actor/ActorMailbox.hpp"
#include "Logging/Logger.hpp"

#include <new>

LumenEngine::FMailBox::FMailBox () noexcept : Head( &Stub )
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
}

void LumenEngine::FMailBox::Push ( FMessage InMessage ) noexcept
{
    FNode *const NewNode = new ( std::nothrow ) FNode();

    if ( NewNode == nullptr )
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Push: failed to allocate node for new message" );
        return;
    }
    NewNode->Message     = std::move( InMessage );

    /**
     * INFO: exchange is the wait-free heart of Vyukov's algorithm.
     *   release -> synchronises with the consumer's acquire on Next.
     *   After this line, PrevTail->Next must still be written
     */
    FNode *const PrevTail = Tail.exchange( NewNode, std::memory_order_release );

    /**
     * INFO: This relaxed store is safe: the consumer will only ever reach
     * PrevTail->Next after it has seen NewNode as the new Tail (via acquire on exchange),
     * which already carries the release above. The gap between exchange and
     * this store is the "transient gap";
     * Pop() handles it by returning nullopt.
     */
    PrevTail->Next.store( NewNode, std::memory_order_relaxed );
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
        LUMEN_LOG_WARNING( LogActor, "FMailBox::Pop: queue is empty or producer is in transient gap" );
        return {};
    }

    if ( not NextNode->Message.has_value() )
    {
        LUMEN_LOG_ERROR( LogActor, "FMailBox::Pop: node has no message (stub leaked?)" );
        return {};
    }

    TOptional<FMessage> Result = std::move( NextNode->Message );

    /** INFO: The old Head becomes the new stub; delete it unless it is the original stub. */
    FNode *const OldHead = Head;
    Head                 = NextNode;

    if ( OldHead != &Stub )
    {
        delete OldHead;
    }

    return Result;
}

LumenEngine::Bool LumenEngine::FMailBox::IsEmpty () const noexcept
{
    return Head->Next.load( std::memory_order_acquire ) == nullptr;
}