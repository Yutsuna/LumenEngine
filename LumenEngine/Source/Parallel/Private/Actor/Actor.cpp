/**
 * @file Actor.cpp
 * @brief FActor base class implementation for LumenEngine.
 */

#include "Actor/Actor.hpp"

LumenEngine::FActor::FActor( ActorID InId ) noexcept
    : Id( InId )
{
    /* CTOR */
}

void LumenEngine::FActor::EnqueueMessage( FMessage InMessage ) noexcept
{
    Mailbox.Push( std::move( InMessage ) );
}

void LumenEngine::FActor::ProcessMailbox() noexcept
{
    while ( TOptional<FMessage> Message = Mailbox.Pop() )
    {
        Receive( std::move( *Message ) );
    }
}

LumenEngine::ActorID LumenEngine::FActor::GetId() const noexcept
{
    return Id;
}