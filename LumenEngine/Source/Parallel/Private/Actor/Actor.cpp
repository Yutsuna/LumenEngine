/**
 * @file Actor.cpp
 * @brief AActor base class implementation for LumenEngine.
 */

#include "Actor/Actor.hpp"

LumenEngine::AActor::AActor ( ActorID InId ) noexcept : Id( InId )
{
    /* CTOR */
}

void LumenEngine::AActor::EnqueueMessage ( FMessage InMessage ) noexcept
{
    Mailbox.Push( std::move( InMessage ) );
}

void LumenEngine::AActor::ProcessMailbox () noexcept
{
    while ( TOptional<FMessage> Message = Mailbox.Pop() )
    {
        Receive( std::move( *Message ) );
    }
}

LumenEngine::ActorID LumenEngine::AActor::GetId () const noexcept
{
    return Id;
}