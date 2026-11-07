/**
 * @file ActorRegistry.hpp
 * @brief Internal registry for mapping ActorIDs to AActor pointers.
 */

#pragma once

#include <Actor/ActorTypes.hpp>
#include <SceneExport.hpp>

namespace Lumen
{

namespace Internal
{
    /**
     * @class FActorRegistry
     * @brief Thread-safe registry for actor lookups.
     */
    class SCENE_EXPORT FActorRegistry
    {
    public:

        /** @brief Allocates a new actor ID. */
        static ActorID AllocateId () noexcept;

        /** @brief Registers an actor. Called from AActor constructor. */
        static void Register ( AActor *InActor ) noexcept;

        /** @brief Unregisters an actor. Called from AActor destructor. */
        static void Unregister ( AActor *InActor ) noexcept;

        /** @brief Enqueues a message to the actor with the given ID. */
        static void EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;
    };
} // namespace Internal

} // namespace Lumen
