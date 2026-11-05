///
/// @file ActorRegistry.cpp
/// @brief Implementation of the internal actor registry.
///

#include "ActorRegistry.hpp"
#include "Templates/Map.hpp"

#include <Actor/Actor.hpp>

#include <HAL/Mutex.hpp>
#include <Memory/UniquePtr.hpp>

#include <limits>

///
/// FActorIDHash
///

Lumen::USize Lumen::Internal::FActorRegistry::FActorIDHash::operator()( const ActorID InId ) const noexcept
{
    UInt64 Key = ( static_cast<UInt64>( InId.Generation ) << 32U ) | InId.Index;
    Key        = ( Key ^ ( Key >> 30U ) ) * 0xbf58476d1ce4e5b9ULL;
    Key        = ( Key ^ ( Key >> 27U ) ) * 0x94d049bb133111ebULL;
    return static_cast<USize>( Key ^ ( Key >> 31U ) );
}

///
/// FActorRegistry
///

Lumen::Internal::FActorRegistry::FRegistryState &Lumen::Internal::FActorRegistry::GetState () noexcept
{
    static TUniquePtr<FRegistryState> State = MakeUnique<FRegistryState>();
    return *State;
}

Lumen::EActorRegistrationResult Lumen::Internal::FActorRegistry::Register ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return EActorRegistrationResult::InvalidID;
    }

    FRegistryState &State = GetState();
    TUniqueLock<FSharedMutex> Lock( State.RegistryMutex );

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
        Index = State.NextIndex++;
        if ( State.NextIndex == 0U )
        {
            State.NextIndex = 1U;
        }
    }

    UInt32 &LastGeneration = State.Generations[Index];
    if ( LastGeneration == std::numeric_limits<UInt32>::max() )
    {
        return EActorRegistrationResult::GenerationExhausted;
    }

    const UInt32 Generation = LastGeneration + 1U;
    LastGeneration          = Generation;

    const ActorID NewId{ Index, Generation };
    auto [It, bInserted] = State.Actors.emplace( NewId, InActor );
    if ( not bInserted )
    {
        return EActorRegistrationResult::AlreadyRegistered;
    }

    if ( not InActor->TryActivate() )
    {
        State.Actors.erase( It );
        return EActorRegistrationResult::ActorStopped;
    }
    InActor->Id = NewId;

    return EActorRegistrationResult::Success;
}

void Lumen::Internal::FActorRegistry::Unregister ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    FRegistryState &State = GetState();
    TUniqueLock<FSharedMutex> Lock( State.RegistryMutex );
    const ActorID Id = InActor->Id;
    if ( not Id.IsValid() )
    {
        return;
    }

    using FIterator = TMap<ActorID, AActor *, FActorIDHash>::iterator;
    FIterator It    = State.Actors.find( Id );

    if ( It != State.Actors.end() and It->second == InActor )
    {
        State.Actors.erase( It );
    }
}

Lumen::EEnqueueResult Lumen::Internal::FActorRegistry::EnqueueMessage ( const ActorID InId, const FMessage &InMessage ) noexcept
{
    if ( not InId.IsValid() )
    {
        return EEnqueueResult::ActorUnavailable;
    }

    FRegistryState &State = GetState();
    AActor *Target        = nullptr;
    {
        TSharedLock<FSharedMutex> Lock( State.RegistryMutex );
        auto It = State.Actors.find( InId );
        if ( It != State.Actors.end() and It->second->TryAcquireSender() )
        {
            Target = It->second;
        }
    }

    if ( Target == nullptr )
    {
        return EEnqueueResult::ActorUnavailable;
    }

    /// INFO: The sender pin is acquired under the registry lock, which orders it before Shutdown's exclusive lock and sender
    /// drain.
    const EEnqueueResult Result = Target->EnqueueMessage( InMessage );
    Target->ReleaseSender();
    return Result;
}
