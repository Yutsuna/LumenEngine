///
/// @file Actor.cpp
/// @brief AActor base class implementation for Lumen.
///

#include <Actor/Actor.hpp>

#include "ActorRegistry.hpp"

#include <cassert>
#include <thread>
#include <utility>

namespace
{

thread_local Lumen::AActor *GProcessingActor                    = nullptr;
thread_local Lumen::FActorProcessingLease *GProcessingLeaseHead = nullptr;

class FProcessingScope final
{
public:

    explicit FProcessingScope ( Lumen::AActor *InActor ) noexcept : Previous( GProcessingActor )
    {
        GProcessingActor = InActor;
    }

    ~FProcessingScope () noexcept
    {
        GProcessingActor = Previous;
    }

private:

    Lumen::AActor *Previous = nullptr;
};
} // namespace

///
/// Actor
///

Lumen::AActor::AActor ( const UInt32 InIndex, const USize InMailboxCapacity, const USize InPreallocatedNodes ) noexcept
    : AActor( InIndex, InMailboxCapacity, FMailBox::PoolCapacity, InPreallocatedNodes )
{
    ///
}

Lumen::AActor::AActor ( const UInt32 InIndex,
                        const USize InMailboxCapacity,
                        const USize InPoolCapacity,
                        const USize InPreallocatedNodes ) noexcept
    : Id{ InIndex, 0U }, Mailbox( InMailboxCapacity, InPoolCapacity, InPreallocatedNodes )
{
    ///
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
    if ( GProcessingActor == this )
    {
        return;
    }

    FActorProcessingLease::ResetActorForCurrentThread( this );
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
    if ( GProcessingActor == this )
    {
        return;
    }

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

    return ProcessMailboxInternal( InMaxMessages, false ).ProcessedCount;
}

Lumen::FActorProcessingResult Lumen::AActor::ProcessMailboxDetailed ( const UInt32 InMaxMessages ) noexcept
{
    if ( not TryBeginProcessing() )
    {
        return { .Status         = IsActive() ? EActorProcessingStatus::Busy : EActorProcessingStatus::ActorUnavailable,
                 .ProcessedCount = 0U };
    }

    return ProcessMailboxInternal( InMaxMessages, true );
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

Lumen::FActorProcessingResult Lumen::AActor::ProcessMailboxInternal ( const UInt32 InMaxMessages, const Bool bDetailed ) noexcept
{
    FProcessingScope ProcessingScope( this );
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

    EActorProcessingStatus Status = ProcessedCount == 0U ? EActorProcessingStatus::Empty : EActorProcessingStatus::Processed;
    if ( bDetailed )
    {
        const Bool bIsActive = IsActive();
        if ( not bIsActive )
        {
            Status = EActorProcessingStatus::ActorUnavailable;
        }
        else if ( not Mailbox.IsEmpty() )
        {
            Status = EActorProcessingStatus::WorkRemaining;
        }
    }

    EndProcessing();
    return { .Status = Status, .ProcessedCount = ProcessedCount };
}

Lumen::FActorProcessingLease::~FActorProcessingLease () noexcept
{
    Reset();
}

Lumen::Bool Lumen::FActorProcessingLease::IsValid () const noexcept
{
    return Actor != nullptr;
}

Lumen::FActorProcessingResult Lumen::FActorProcessingLease::ProcessMailbox ( const UInt32 InMaxMessages ) noexcept
{
    AActor *const ProcessingActor = Detach();
    if ( ProcessingActor == nullptr )
    {
        return {};
    }

    const FActorProcessingResult Result = ProcessingActor->ProcessMailboxInternal( InMaxMessages, true );
    ProcessingActor->ReleaseSender();
    return Result;
}

void Lumen::FActorProcessingLease::Reset () noexcept
{
    AActor *const OldActor = Detach();
    if ( OldActor != nullptr )
    {
        OldActor->EndProcessing();
        OldActor->ReleaseSender();
    }
}

void Lumen::FActorProcessingLease::Attach ( AActor *InActor ) noexcept
{
    Actor                = InActor;
    Previous             = GProcessingLeaseHead;
    GProcessingLeaseHead = this;
}

Lumen::AActor *Lumen::FActorProcessingLease::Detach () noexcept
{
    AActor *const OldActor = std::exchange( Actor, nullptr );
    if ( OldActor == nullptr )
    {
        return nullptr;
    }

    FActorProcessingLease *Current = GProcessingLeaseHead;
    if ( Current == this )
    {
        GProcessingLeaseHead = Previous;
    }
    else
    {
        while ( Current != nullptr and Current->Previous != this )
        {
            Current = Current->Previous;
        }

        if ( Current != nullptr )
        {
            Current->Previous = Previous;
        }
    }

    Previous = nullptr;
    return OldActor;
}

void Lumen::FActorProcessingLease::ResetActorForCurrentThread ( AActor *InActor ) noexcept
{
    FActorProcessingLease *Current = GProcessingLeaseHead;
    while ( Current != nullptr )
    {
        FActorProcessingLease *const PreviousLease = Current->Previous;
        if ( Current->Actor == InActor )
        {
            Current->Reset();
        }
        Current = PreviousLease;
    }
}
