///
/// @file Actor.cpp
/// @brief AActor base class implementation for Lumen.
///

#include <Actor/Actor.hpp>

#include "ActorRegistry.hpp"

Lumen::AActor::AActor ( ActorID InId ) noexcept : Id( InId )
{
    Internal::FActorRegistry::Register( this );
}

Lumen::AActor::~AActor () noexcept
{
    Internal::FActorRegistry::Unregister( this );
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

        const Bool bHasMessage = Message.Inspect( [this] ( const FMessage &InMessage ) { Receive( InMessage ); } ).HasValue();

        if ( not bHasMessage )
        {
            break;
        }

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
