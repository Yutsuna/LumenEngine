/**
 * @file ActorRegistry.cpp
 * @brief Implementation of the internal actor registry.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Map.hpp>
#include <Templates/Pair.hpp>

#include <HAL/Mutex.hpp>
#include <HAL/SharedMutex.hpp>

#include <Logging/Logger.hpp>

namespace
{

Lumen::FSharedMutex RegistryMutex;
Lumen::TMap<Lumen::ActorID, Lumen::AActor *> Actors;

} // namespace

Lumen::ActorID Lumen::Internal::FActorRegistry::AllocateId () noexcept
{
    static TAtomic<ActorID> Next{ 1ULL };
    return Next.fetch_add( 1, std::memory_order_relaxed );
}

void Lumen::Internal::FActorRegistry::Register ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );

    using FActorMap                    = TMap<ActorID, AActor *>;
    using FActorIt                     = typename FActorMap::iterator;
    const TPair<FActorIt, Bool> Result = Actors.try_emplace( InActor->GetId(), InActor );

    if ( not Result.second )
    {
        LUMEN_LOG_ERROR( LogActor, "FActorRegistry::Register: duplicate ActorID '{}'", InActor->GetId() );
    }
}

void Lumen::Internal::FActorRegistry::Unregister ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    TUniqueLock<FSharedMutex> Lock( RegistryMutex );

    using FActorMap   = TMap<ActorID, AActor *>;
    using FActorIt    = typename FActorMap::iterator;
    const FActorIt It = Actors.find( InActor->GetId() );

    if ( It != Actors.end() and It->second == InActor )
    {
        Actors.erase( InActor->GetId() );
    }
}

void Lumen::Internal::FActorRegistry::EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept
{
    TSharedLock<FSharedMutex> Lock( RegistryMutex );

    using FActorMap = TMap<ActorID, AActor *>;
    using FActorIt  = typename FActorMap::iterator;
    FActorIt It     = Actors.find( InId );

    if ( It != Actors.end() )
    {
        It->second->EnqueueMessage( InMessage );
    }
}
