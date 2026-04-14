/**
 * @file World.hpp
 * @brief Declaration of the FWorld class.
 */

#pragma once

#include "Actor/Actor.hpp"

#include "Container/Map.hpp"
#include "Container/SharedPtr.hpp"
#include "Container/Vector.hpp"

namespace LumenEngine
{

namespace Engine
{

    class LUMEN_ENGINE_API FWorld : public FNonCopyable
    {
    public:

        FWorld () noexcept          = default;
        virtual ~FWorld () noexcept = default;

        /** @brief Spawns an actor of type T and returns it */
        template <typename ActorType, typename... Args> TSharedRef<ActorType> SpawnActor ( Args &&...InArgs );

        /** @brief Broadcasts a message to all actors */
        void Broadcast ( const FMessage &InMessage );

        /** @brief Core loop: Ticks logic and processes all mailboxes in parallel */
        void Tick ( const Float64 InDeltaTime );

    private:

        TMap<ActorID, TSharedPtr<AActor>> Actors;
        TVector<TSharedPtr<AActor>> ActiveActors;
        ActorID NextActorId = 1000ULL;
    };

} // namespace Engine

} // namespace LumenEngine

#include "Inline/World.inl"