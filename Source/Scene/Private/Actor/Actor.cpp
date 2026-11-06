/**
 * @file Actor.cpp
 * @brief AActor base class implementation for Lumen.
 */

#include "ActorRegistry.hpp"
#include <Actor/Actor.hpp>

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

    while ( InMaxMessages == 0U or ProcessedCount < InMaxMessages )
    {
        TOptional<FMessage> Message = Mailbox.Pop();

        if ( not Message.HasValue() )
        {
            break;
        }

        Receive( *Message );
        ++ProcessedCount;
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
