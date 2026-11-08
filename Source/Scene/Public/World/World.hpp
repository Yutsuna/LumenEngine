/**
 * @file World.hpp
 * @brief Declaration of the FWorld class.
 */

#pragma once

#include <SceneExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <Templates/Function.hpp>
#include <Templates/Map.hpp>
#include <Templates/Vector.hpp>

#include <HAL/Mutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <Actor/ActorTypes.hpp>

namespace Lumen
{

/**
 * @class FWorld
 * @brief Manages the world state, including spawning and ticking actors.
 */
class SCENE_EXPORT FWorld : public FNonCopyable
{
public:

    FWorld () noexcept          = default;
    virtual ~FWorld () noexcept = default;

public:

    /** @brief Spawns an actor of type ActorType and returns it */
    template <typename ActorType, typename... Args> [[nodiscard]] TSharedRef<ActorType> SpawnActor ( Args &&...InArgs );

    /**
     * @brief Queues actor construction for the next serial phase.
     * @details Safe to call from Receive() during the parallel phase.
     *          The returned FActorRef carries a valid ActorID immediately,
     *          but the does not exist (and cannot receive messages) until
     *          the next Tick applies the deferred command.
     */
    template <typename ActorType, typename... Args> [[nodiscard]] FActorRef SpawnActorDeferred ( Args &&...InArgs );

    /** @brief Removes an actor from the world. Auto-defers during the parallel phase. */
    void DestroyActor ( ActorID InId );

    /** @brief Broadcasts a message to all actors. Auto-defers during parallel phase. */
    void Broadcast ( const FMessage &InMessage );

    /** @brief Core loop: Ticks logic and processes all mailboxes in parallel */
    void Tick ( Float64 InDeltaTime );

    /** @returns True while the parallel phase is active. */
    [[nodiscard]] Bool IsInParallelPhase () const noexcept
    {
        return bInParallelPhase;
    }

public:

    /** Maximum messages an actor can process per tick to prevent "Long Tail" latency */
    static constexpr UInt32 MaxMessagesPerTick = 100U;

private:

    void ApplyDeferredCommands () noexcept;
    void DestroyActorImpl ( ActorID InId ) noexcept;

private:

    TMap<ActorID, TSharedPtr<AActor>> Actors;
    TVector<TSharedPtr<AActor>> ActiveActors;

    mutable FMutex DeferredMutex;
    TVector<TFunction<void( FWorld & )>> DeferredCommands;

    Bool bNeedsSorting    = false;
    Bool bInParallelPhase = false;
};

namespace Internal
{

    /** @brief Registry exhausted; caller gets a null-owner shared ref. */
    [[noreturn]] inline void TerminateOnRegistryExhaustion ();

} // namespace Internal

} // namespace Lumen

#include "Inline/World.inl"
