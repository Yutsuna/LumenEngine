///
/// @file ActorRegistry.cpp
/// @brief Implementation of the internal actor registry.
///

#include "ActorRegistry.hpp"

#include <Actor/Actor.hpp>

#include <HAL/Mutex.hpp>

#include <limits>

namespace Lumen
{

FSharedMutex Internal::FActorRegistry::RegistryMutex;
TMap<ActorID, AActor *, Internal::FActorRegistry::FActorIDHash>Internal::FActorRegistry::Actors;
TMap<UInt32, UInt32> Internal::FActorRegistry::Generations;
UInt32 Internal::FActorRegistry::NextIndex = 1U;

EActorRegistrationResult Internal::FActorRegistry::Register ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return EActorRegistrationResult::InvalidID;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );

    const UInt32 InitialState = InActor->Lifecycle.load( std::memory_order_acquire );
    if ( ( InitialState & AActor::ShutdownFlag ) != 0U )
    {
        return EActorRegistrationResult::ActorStopped;
    }
    if ( InitialState != 0U )
    {
        return EActorRegistrationResult::AlreadyRegistered;
    }

    UInt32 Index = InActor->Id.Index;
    if ( Index == 0U )
    {
        Index = NextIndex++;
        if ( NextIndex == 0U )
        {
            NextIndex = 1U;
        }
    }

    UInt32 &LastGeneration = Generations[Index];
    if ( LastGeneration == std::numeric_limits<UInt32>::max() )
    {
        return EActorRegistrationResult::GenerationExhausted;
    }

    const UInt32 Generation = LastGeneration + 1U;
    LastGeneration          = Generation;

    const ActorID NewId{ Index, Generation };
    auto [It, bInserted] = Actors.emplace( NewId, InActor );
    if ( not bInserted )
    {
        return EActorRegistrationResult::AlreadyRegistered;
    }

    InActor->Id = NewId;
    if ( not InActor->TryActivate() )
    {
        Actors.erase( It );
        return EActorRegistrationResult::ActorStopped;
    }

    return EActorRegistrationResult::Success;
}

void Internal::FActorRegistry::Unregister ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr || not InActor->GetId().IsValid() )
    {
        return;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );
    auto It = Actors.find( InActor->GetId() );
    if ( It != Actors.end() && It->second == InActor )
    {
        Actors.erase( It );
    }
}

EEnqueueResult Internal::FActorRegistry::EnqueueMessage ( const ActorID InId, const FMessage &InMessage ) noexcept
{
    if ( not InId.IsValid() )
    {
        return EEnqueueResult::ActorUnavailable;
    }

    AActor *Target = nullptr;
    {
        TSharedLock<FSharedMutex> Lock( RegistryMutex );
        auto It = Actors.find( InId );
        if ( It != Actors.end() && It->second->TryAcquireSender() )
        {
            Target = It->second;
        }
    }

    if ( Target == nullptr )
    {
        return EEnqueueResult::ActorUnavailable;
    }

    /// INFO: The sender pin keeps the actor alive after releasing the registry lock; Shutdown waits for it before destruction.
    const EEnqueueResult Result = Target->EnqueueMessage( InMessage );
    Target->ReleaseSender();
    return Result;
}

}
