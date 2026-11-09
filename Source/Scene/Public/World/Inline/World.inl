/**
 * @file World.inl
 * @brief Implementation of the FWorld class.
 */

#pragma once

#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <World/World.hpp>

#include <Logging/Logger.hpp>

/**
 * Internal
 */

[[noreturn]] void Lumen::Internal::TerminateOnRegistryExhaustion ()
{
    LUMEN_LOG_FATAL( LogActor, "FActorRegistry::AllocateId returned InvalidActorID" );
    std::terminate(); //<< NOTE: noreturn because FATAL already kills the engine.
}

/**
 * Public
 */

template <typename ActorType, typename... Args> Lumen::TSharedRef<ActorType> Lumen::FWorld::SpawnActor ( Args &&...InArgs )
{
    const ActorID NewId = Internal::FActorRegistry::AllocateId();

    if ( NewId == InvalidActorID ) [[unlikely]]
    {
        Internal::TerminateOnRegistryExhaustion();
    }

    TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::forward<Args>( InArgs )... );
    TSharedPtr<AActor> StoredActor = NewActor;

    Actors[NewId] = StoredActor;
    ActiveActors.push_back( StoredActor );
    bNeedsSorting = true;
    return NewActor;
}

template <typename ActorType, typename... Args> Lumen::FActorRef Lumen::FWorld::SpawnActorDeferred ( Args &&...InArgs )
{
    const ActorID NewId = Internal::FActorRegistry::AllocateId();
    if ( NewId == InvalidActorID ) [[unlikely]]
    {
        Internal::TerminateOnRegistryExhaustion();
    }

    FDeferredCommand Command = { .OwnedId = NewId,
                                 .Execute = [NewId, ... Captured = std::forward<Args>( InArgs )] ( FWorld &World ) mutable -> void
    {
        TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::move( Captured )... );
        TSharedPtr<AActor> StoredActor = NewActor;
        World.Actors[NewId]            = StoredActor;
        World.ActiveActors.push_back( StoredActor );
        World.bNeedsSorting = true;
    } };

    {
        TUniqueLock<FMutex> Lock( DeferredMutex );
        DeferredCommands.push_back( std::move( Command ) );
    }

    return FActorRef( NewId );
}
