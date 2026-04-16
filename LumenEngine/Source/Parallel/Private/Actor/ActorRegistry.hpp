/**
 * @file ActorRegistry.hpp
 * @brief Internal registry for mapping ActorIDs to AActor pointers.
 */

#pragma once

#include "Actor/ActorTypes.hpp"
#include "Container/Map.hpp"
#include "HAL/SharedMutex.hpp"

namespace LumenEngine
{

namespace Internal
{
    /**
     * @class FActorRegistry
     * @brief Thread-safe registry for actor lookups.
     */
    class FActorRegistry
    {
    public:

        /** @brief Registers an actor. Called from AActor constructor. */
        static void Register ( AActor *InActor ) noexcept;

        /** @brief Unregisters an actor. Called from AActor destructor. */
        static void Unregister ( AActor *InActor ) noexcept;

        /** @brief Enqueues a message to the actor with the given ID. */
        static void EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;

    private:

        static FSharedMutex RegistryMutex;
        static TMap<ActorID, AActor *> Actors;
    };
} // namespace Internal

} // namespace LumenEngine
