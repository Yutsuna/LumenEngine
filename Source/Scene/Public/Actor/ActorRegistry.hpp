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
     * @note Sends are lock-free; spawn/despawn take a short lock.
     */
    class SCENE_EXPORT FActorRegistry
    {
    public:

        /** @brief Allocates a fresh ActorID. Safe from any thread. */
        [[nodiscard]] static ActorID AllocateId () noexcept;

        /** @brief Total number of messages dropped because the target actor was dead. */
        [[nodiscard]] static UInt64 GetDeadLetterCount () noexcept;

        /** @brief Registers the actor in its slot. Serial phase only. */
        static void Register ( AActor *InActor ) noexcept;

        /** @brief Nulls the slot and bumps its generation. Serial phase only. */
        static void Unregister ( AActor *InActor ) noexcept;

        /** @brief Lock-free lookup + deliver. */
        static void EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;

        /** @brief Retires an ID that was never bound to an actor (deferred spawn cancelled). */
        static void ReleaseId ( ActorID InId ) noexcept;
    };

} // namespace Internal

} // namespace Lumen
