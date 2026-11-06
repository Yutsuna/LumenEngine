/**
 * @file World.hpp
 * @brief Declaration of the FWorld class.
 */

#pragma once

#include <SceneExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>

#include <Templates/Map.hpp>
#include <Templates/Vector.hpp>

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

    /** @brief Spawns an actor of type T and returns it */
    template <typename ActorType, typename... Args> TSharedRef<ActorType> SpawnActor ( Args &&...InArgs );

    /** @brief Broadcasts a message to all actors */
    void Broadcast ( const FMessage &InMessage );

    /** @brief Core loop: Ticks logic and processes all mailboxes in parallel */
    void Tick ( const Float64 InDeltaTime );

public:

    /** Maximum messages an actor can process per tick to prevent "Long Tail" latency */
    static constexpr UInt32 MaxMessagesPerTick = 100U;

private:

    TMap<ActorID, TSharedPtr<AActor>> Actors;
    TVector<TSharedPtr<AActor>> ActiveActors;
    ActorID NextActorId = 1000ULL;
    Bool bNeedsSorting   = false;
};

} // namespace Lumen

#include "Inline/World.inl"
