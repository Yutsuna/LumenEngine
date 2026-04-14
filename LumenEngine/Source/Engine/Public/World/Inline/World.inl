/**
 * @file World.inl
 * @brief Implementation of the FWorld class.
 */

#pragma once

#include "World/World.hpp"

template <typename ActorType, typename... Args> LumenEngine::TSharedRef<ActorType> LumenEngine::Engine::FWorld::SpawnActor ( Args &&...InArgs )
{
    ActorID NewId                  = NextActorId++;
    TSharedRef<ActorType> NewActor = MakeShared<ActorType>( NewId, std::forward<Args>( InArgs )... );
    TSharedPtr<AActor> StoredActor = NewActor;

    Actors[NewId] = StoredActor;
    ActiveActors.push_back( StoredActor );
    return NewActor;
}