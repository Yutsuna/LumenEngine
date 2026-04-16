/**
 * @file ActorRegistry.cpp
 * @brief Implementation of the internal actor registry.
 */

#include "ActorRegistry.hpp"
#include "Actor/Actor.hpp"

#include "HAL/Mutex.hpp"

LumenEngine::FSharedMutex LumenEngine::Internal::FActorRegistry::RegistryMutex;
LumenEngine::TMap<LumenEngine::ActorID, LumenEngine::AActor *> LumenEngine::Internal::FActorRegistry::Actors;

void LumenEngine::Internal::FActorRegistry::Register ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );
    Actors[InActor->GetId()] = InActor;
}

void LumenEngine::Internal::FActorRegistry::Unregister ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );
    Actors.erase( InActor->GetId() );
}

void LumenEngine::Internal::FActorRegistry::EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept
{
    TSharedLock<FSharedMutex> Lock( RegistryMutex );

    using ActorMap = TMap<ActorID, AActor *>;
    using ActorIt  = typename ActorMap::iterator;
    ActorIt It     = Actors.find( InId );

    if ( It != Actors.end() )
    {
        It->second->EnqueueMessage( InMessage );
    }
}
