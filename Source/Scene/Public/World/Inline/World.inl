/**
 * @file World.inl
 * @brief Implementation of the FWorld class.
 */

#pragma once

#include <Actor/ActorRegistry.hpp>
#include <World/World.hpp>

template <typename ActorType, typename... Args> Lumen::TSharedRef<ActorType> Lumen::FWorld::SpawnActor ( Args &&...InArgs )
{
    ActorID NewId                  = Internal::FActorRegistry::AllocateId();
    TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::forward<Args>( InArgs )... );
    TSharedPtr<AActor> StoredActor = NewActor;

    Actors[NewId] = StoredActor;
    ActiveActors.push_back( StoredActor );
    bNeedsSorting = true;
    return NewActor;
}
