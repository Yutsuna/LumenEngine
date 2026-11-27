/**
 * @file World.inl
 * @brief Template implementation of the FWorld class.
 */

#pragma once

#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <World/World.hpp>

#include <Logging/Logger.hpp>

#include <exception>

[[noreturn]] inline void Lumen::Internal::TerminateOnSpawnFailure ( const EActorSpawnError InError ) noexcept
{
    LUMEN_LOG_FATAL( LogActor, "Legacy actor spawn failed with error {}.", static_cast<UInt32>( InError ) );
    std::terminate();
}

template <typename ActorType, typename... Args> Lumen::TSharedRef<ActorType> Lumen::FWorld::SpawnActor ( Args &&...InArgs )
{
    TExpected<TSharedRef<ActorType>, EActorSpawnError> Result = TrySpawnActor<ActorType>( std::forward<Args>( InArgs )... );
    if ( not Result ) [[unlikely]]
    {
        Internal::TerminateOnSpawnFailure( Result.Error() );
    }
    return std::move( *Result );
}

template <typename ActorType, typename... Args>
Lumen::TExpected<Lumen::TSharedRef<ActorType>, Lumen::EActorSpawnError> Lumen::FWorld::TrySpawnActor ( Args &&...InArgs ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSpawnError::WorldClosed );
    }

    TExpected<ActorID, EActorSpawnError> AllocatedId = Internal::FActorRegistry::AllocateId();
    if ( not AllocatedId ) [[unlikely]]
    {
        return MakeUnexpected( AllocatedId.Error() );
    }
    const ActorID NewId = *AllocatedId;

    TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::forward<Args>( InArgs )... );
    if ( not NewActor->bRegistered ) [[unlikely]]
    {
        Internal::FActorRegistry::ReleaseId( NewId );
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }

    NewActor->ReserveOutbox( Desc.ActorOutboxCapacity );
    TSharedPtr<AActor> StoredActor             = NewActor;
    TExpected<void, EActorSpawnError> Inserted = InsertSpawnedActor( StoredActor );
    if ( not Inserted )
    {
        return MakeUnexpected( Inserted.Error() );
    }
    return NewActor;
}

template <typename ActorType, typename... Args> Lumen::FActorRef Lumen::FWorld::SpawnActorDeferred ( Args &&...InArgs )
{
    TExpected<FActorRef, EActorSpawnError> Result = TrySpawnActorDeferred<ActorType>( std::forward<Args>( InArgs )... );
    if ( not Result ) [[unlikely]]
    {
        Internal::TerminateOnSpawnFailure( Result.Error() );
    }
    return *Result;
}

template <typename ActorType, typename... Args>
Lumen::TExpected<Lumen::FActorRef, Lumen::EActorSpawnError> Lumen::FWorld::TrySpawnActorDeferred ( Args &&...InArgs ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSpawnError::WorldClosed );
    }

    TExpected<ActorID, EActorSpawnError> AllocatedId = Internal::FActorRegistry::AllocateId();
    if ( not AllocatedId ) [[unlikely]]
    {
        return MakeUnexpected( AllocatedId.Error() );
    }
    const ActorID NewId = *AllocatedId;

    TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::forward<Args>( InArgs )... );
    if ( not NewActor->bRegistered ) [[unlikely]]
    {
        Internal::FActorRegistry::ReleaseId( NewId );
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }

    NewActor->ReserveOutbox( Desc.ActorOutboxCapacity );
    NewActor->SuspendRouting();

    TSharedPtr<AActor> StoredActor = NewActor;
    if ( not QueueDeferredCommand( NewId, FDeferredSpawnCommand{ .Actor = StoredActor } ) ) [[unlikely]]
    {
        return MakeUnexpected( EActorSpawnError::DeferredQueueFull );
    }
    return FActorRef( NewId );
}
