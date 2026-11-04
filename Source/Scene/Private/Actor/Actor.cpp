///
/// @file Actor.cpp
/// @brief AActor base class implementation for Lumen.
///

#include <Actor/Actor.hpp>

#include "ActorRegistry.hpp"

#include <thread>

Lumen::AActor::AActor ( const UInt32 InIndex, const USize InMailboxCapacity, const USize InPreallocatedNodes ) noexcept
    : Id{ InIndex, 0U }, Mailbox( InMailboxCapacity, InPreallocatedNodes )
{
}

Lumen::AActor::~AActor () noexcept
{
    Shutdown();
}

Lumen::EActorRegistrationResult Lumen::AActor::Initialize () noexcept
{
    return Internal::FActorRegistry::Register( this );
}

void Lumen::AActor::Shutdown () noexcept
{
    Lifecycle.fetch_or( ShutdownFlag, std::memory_order_acq_rel );
    Lifecycle.fetch_and( ~ActiveFlag, std::memory_order_acq_rel );

    Internal::FActorRegistry::Unregister( this );
    Mailbox.Close();

    while ( ActiveSenders.load( std::memory_order_acquire ) != 0U ||
            ( Lifecycle.load( std::memory_order_acquire ) & ProcessingFlag ) != 0U )
    {
        std::this_thread::yield();
    }
}

void Lumen::AActor::Destroy () noexcept
{
    Shutdown();
    delete this;
}

Lumen::EEnqueueResult Lumen::AActor::EnqueueMessage ( const FMessage &InMessage ) noexcept
{
    return Mailbox.Push( InMessage );
}

Lumen::UInt32 Lumen::AActor::ProcessMailbox ( const UInt32 InMaxMessages ) noexcept
{
    if ( not TryBeginProcessing() )
    {
        return 0U;
    }

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

    EndProcessing();
    return ProcessedCount;
}

Lumen::ActorID Lumen::AActor::GetId () const noexcept
{
    return Id;
}

Lumen::FActorRef Lumen::AActor::GetRef () const noexcept
{
    return FActorRef( Id );
}

Lumen::Bool Lumen::AActor::IsActive () const noexcept
{
    const UInt32 State = Lifecycle.load( std::memory_order_acquire );
    return ( State & ActiveFlag ) != 0U and ( State & ShutdownFlag ) == 0U;
}

Lumen::FMailBox &Lumen::AActor::GetMailbox () noexcept
{
    return Mailbox;
}

Lumen::Bool Lumen::AActor::TryActivate () noexcept
{
    UInt32 Expected = 0U;
    return Lifecycle.compare_exchange_strong( Expected, ActiveFlag, std::memory_order_acq_rel );
}

Lumen::Bool Lumen::AActor::TryBeginProcessing () noexcept
{
    UInt32 State = Lifecycle.load( std::memory_order_acquire );
    for ( ;; )
    {
        if ( ( State & ActiveFlag ) == 0U or ( State & ShutdownFlag ) != 0U or ( State & ProcessingFlag ) != 0U )
        {
            return false;
        }

        if ( Lifecycle.compare_exchange_weak( State, State | ProcessingFlag, std::memory_order_acq_rel ) )
        {
            return true;
        }
    }
}

void Lumen::AActor::EndProcessing () noexcept
{
    Lifecycle.fetch_and( ~ProcessingFlag, std::memory_order_release );
}

Lumen::Bool Lumen::AActor::TryAcquireSender () noexcept
{
    if ( not IsActive() )
    {
        return false;
    }

    ActiveSenders.fetch_add( 1U, std::memory_order_acq_rel );
    if ( not IsActive() )
    {
        ReleaseSender();
        return false;
    }

    return true;
}

void Lumen::AActor::ReleaseSender () noexcept
{
    ActiveSenders.fetch_sub( 1U, std::memory_order_release );
}
