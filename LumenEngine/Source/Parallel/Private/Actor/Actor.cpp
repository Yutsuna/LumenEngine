/**
 * @file Actor.cpp
 * @brief AActor base class implementation for LumenEngine.
 */

#include "Actor/Actor.hpp"
#include "ActorRegistry.hpp"

LumenEngine::AActor::AActor ( ActorID InId ) noexcept : Id( InId )
{
    LumenEngine::Internal::FActorRegistry::Register( this );
}

LumenEngine::AActor::~AActor () noexcept
{
    LumenEngine::Internal::FActorRegistry::Unregister( this );
}

void LumenEngine::AActor::EnqueueMessage ( const FMessage &InMessage ) noexcept
{
    Mailbox.Push( InMessage );
}

void LumenEngine::AActor::ProcessMailbox () noexcept
{
    while ( TOptional<FMessage> Message = Mailbox.Pop() )
    {
        Receive( *Message );
    }
}

LumenEngine::ActorID LumenEngine::AActor::GetId () const noexcept
{
    return Id;
}

LumenEngine::FActorRef LumenEngine::AActor::GetRef () const noexcept
{
    return FActorRef( Id );
}
