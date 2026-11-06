/**
 * @file Actor.cpp
 * @brief AActor base class implementation for Lumen.
 */

#include <Actor/Actor.hpp>
#include "ActorRegistry.hpp"

Lumen::AActor::AActor ( ActorID InId ) noexcept : Id( InId )
{
    Lumen::Internal::FActorRegistry::Register( this );
}

Lumen::AActor::~AActor () noexcept
{
    Lumen::Internal::FActorRegistry::Unregister( this );
}

void Lumen::AActor::EnqueueMessage ( const FMessage &InMessage ) noexcept
{
    Mailbox.Push( InMessage );
}

void Lumen::AActor::ProcessMailbox ( const UInt32 InMaxMessages ) noexcept
{
    UInt32 ProcessedCount = 0U;

    while ( TOptional<FMessage> Message = Mailbox.Pop() )
    {
        Receive( *Message );
        ++ProcessedCount;

        if ( InMaxMessages > 0U and ProcessedCount >= InMaxMessages )
        {
            break;
        }
    }
}

Lumen::ActorID Lumen::AActor::GetId () const noexcept
{
    return Id;
}

Lumen::FActorRef Lumen::AActor::GetRef () const noexcept
{
    return FActorRef( Id );
}

Lumen::FMailBox &Lumen::AActor::GetMailbox () noexcept
{
    return Mailbox;
}
