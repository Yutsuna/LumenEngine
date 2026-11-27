/**
 * @file Actor.cpp
 * @brief AActor base class implementation for Lumen.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <World/WorldTypes.hpp>

namespace
{

thread_local Lumen::FActorEmitContext *GCurrentEmitContext = nullptr;

}

Lumen::AActor::AActor ( const ActorID InId ) noexcept : Id( InId )
{
    bRegistered = Internal::FActorRegistry::Register( this );
}

Lumen::AActor::~AActor () noexcept
{
    Retire();
}

void Lumen::AActor::OnBeginPlay () noexcept
{
}

void Lumen::AActor::OnEndPlay () noexcept
{
}

void Lumen::AActor::OnTick ( const Float64 LUMEN_UNUSED InDeltaTime ) noexcept
{
}

void Lumen::AActor::OnFixedTick ( const Float64 LUMEN_UNUSED InDeltaTime ) noexcept
{
}

Lumen::EActorTickPhase Lumen::AActor::GetTickPhase () const noexcept
{
    return EActorTickPhase::Update;
}

void Lumen::AActor::RunFrame ( const Float64 InDeltaTime, const UInt32 InMaxMessages ) noexcept
{
    if ( Lifecycle.load( std::memory_order_acquire ) == EActorLifecycle::Spawning )
    {
        BeginPlay();
    }

    if ( Lifecycle.load( std::memory_order_acquire ) != EActorLifecycle::Active )
    {
        return;
    }

    ProcessMailbox( InMaxMessages );
    if ( GetTickPhase() == EActorTickPhase::FixedUpdate )
    {
        OnFixedTick( InDeltaTime );
    }
    else
    {
        OnTick( InDeltaTime );
    }
}

Lumen::Bool Lumen::AActor::EnqueueMessage ( const FMessage &InMessage ) noexcept
{
    const EActorLifecycle State = Lifecycle.load( std::memory_order_acquire );
    if ( State == EActorLifecycle::PendingKill or State == EActorLifecycle::Retired )
    {
        return false;
    }
    return Mailbox.Push( InMessage );
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::AActor::TrySendMessage ( const ActorID InTarget,
                                                                               const FMessage &InMessage ) noexcept
{
    const EActorLifecycle State = Lifecycle.load( std::memory_order_acquire );
    if ( State == EActorLifecycle::PendingKill or State == EActorLifecycle::Retired )
    {
        Internal::FActorRegistry::RecordDeadLetter( EActorSendError::ActorRetiring, InTarget, InMessage );
        return MakeUnexpected( EActorSendError::ActorRetiring );
    }

    if ( GCurrentEmitContext == &EmitContext )
    {
        TExpected<void, EActorSendError> Result = EmitContext.Emit( InTarget, InMessage );
        if ( not Result )
        {
            Internal::FActorRegistry::RecordDeadLetter( Result.Error(), InTarget, InMessage, EmitContext.GetFrameIndex(),
                                                        EmitContext.GetEmissions().size() );
        }
        return Result;
    }

    FMessage RoutedMessage = InMessage;
    RoutedMessage.Sender   = Id;
    return Internal::FActorRegistry::TryEnqueueMessage( InTarget, RoutedMessage );
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

Lumen::EActorLifecycle Lumen::AActor::GetLifecycle () const noexcept
{
    return Lifecycle.load( std::memory_order_acquire );
}

Lumen::FMailBox &Lumen::AActor::GetMailbox () noexcept
{
    return Mailbox;
}

void Lumen::AActor::BeginPlay () noexcept
{
    EActorLifecycle Expected = EActorLifecycle::Spawning;
    if ( not Lifecycle.compare_exchange_strong( Expected, EActorLifecycle::Active, std::memory_order_acq_rel,
                                                std::memory_order_acquire ) )
    {
        return;
    }

    bBegunPlay = true;
    if ( bRegistered )
    {
        Internal::FActorRegistry::OpenRouting( this );
    }
    OnBeginPlay();
}

void Lumen::AActor::SuspendRouting () noexcept
{
    if ( bRegistered )
    {
        Internal::FActorRegistry::CloseRouting( this );
    }
}

void Lumen::AActor::MarkPendingKill () noexcept
{
    EActorLifecycle State = Lifecycle.load( std::memory_order_acquire );
    while ( State == EActorLifecycle::Spawning or State == EActorLifecycle::Active )
    {
        if ( Lifecycle.compare_exchange_weak( State, EActorLifecycle::PendingKill, std::memory_order_acq_rel,
                                              std::memory_order_acquire ) )
        {
            break;
        }
    }

    if ( bRegistered )
    {
        Internal::FActorRegistry::CloseRouting( this );
    }
}

void Lumen::AActor::EndPlay () noexcept
{
    if ( bBegunPlay and not bEndedPlay )
    {
        bEndedPlay = true;
        OnEndPlay();
    }
}

void Lumen::AActor::Retire () noexcept
{
    if ( Lifecycle.load( std::memory_order_acquire ) == EActorLifecycle::Retired )
    {
        return;
    }

    MarkPendingKill();
    EndPlay();

    if ( bRegistered )
    {
        Internal::FActorRegistry::Unregister( this );
        bRegistered = false;
    }
    Lifecycle.store( EActorLifecycle::Retired, std::memory_order_release );
}

void Lumen::AActor::ReserveOutbox ( const USize InCapacity )
{
    EmitContext.Reserve( InCapacity );
}

void Lumen::AActor::RunWorldFrame ( const FWorldFrameContext &InContext,
                                    const Float64 InDeltaTime,
                                    const UInt32 InMaxMessages ) noexcept
{
    if ( Lifecycle.load( std::memory_order_acquire ) != EActorLifecycle::Active )
    {
        return;
    }

    EmitContext.Begin( Id, InContext.FrameIndex );
    FActorEmitContext *const PreviousContext = GCurrentEmitContext;
    GCurrentEmitContext                      = &EmitContext;

    ProcessMailbox( InMaxMessages );
    if ( InContext.Phase == EActorTickPhase::FixedUpdate )
    {
        OnFixedTick( InDeltaTime );
    }
    else
    {
        OnTick( InDeltaTime );
    }

    GCurrentEmitContext = PreviousContext;
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

Lumen::FActorEmitContext *Lumen::AActor::GetCurrentEmitContext () noexcept
{
    return GCurrentEmitContext;
}
