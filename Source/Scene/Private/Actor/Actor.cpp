/**
 * @file Actor.cpp
 * @brief AActor base class implementation for Lumen.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Core/CoreDefinitions.hpp>
#include <Logging/Logger.hpp>

Lumen::AActor::AActor ( ActorID InId ) noexcept : Id( InId )
{
    Lumen::Internal::FActorRegistry::Register( this );
}

Lumen::AActor::~AActor () noexcept
{
    Lumen::Internal::FActorRegistry::Unregister( this );
}

void Lumen::AActor::OnTick ( const Float64 LUMEN_UNUSED InDeltaTime ) noexcept
{
    /* */
}

void Lumen::AActor::RunFrame ( const Float64 InDeltaTime, const UInt32 InMaxMessages ) noexcept
{
    ProcessMailbox( InMaxMessages );
    OnTick( InDeltaTime );
}

Lumen::Bool Lumen::AActor::EnqueueMessage ( const FMessage &InMessage ) noexcept
{
    return Mailbox.Push( InMessage );
}

void Lumen::AActor::BeginTick () noexcept
{
    Mailbox.BeginTick();
}

void Lumen::AActor::EndTick () noexcept
{
    Mailbox.EndTick();
}

Lumen::ActorID Lumen::AActor::GetId () const noexcept
{
    return Id;
}

Lumen::FActorRef Lumen::AActor::GetRef () const noexcept
{
    return FActorRef{ Id };
}

/**
 * Protected
 */

Lumen::FMailBox &Lumen::AActor::GetMailbox () noexcept
{
    return Mailbox;
}

/**
 * Private
 */

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
