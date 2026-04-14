/**
 * @file SceneActor.hpp
 * @brief Declaration of the ASceneActor class.
 */

#pragma once

#include "Actor/Actor.hpp"
#include "Container/Map.hpp"
#include "Graphics/RenderResource.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class ASceneActor
     * @brief The central authority for the scene state.
     * @details Collects transform updates from entities, maintains visibility, and pushes data to the Renderer.
     */
    class LUMEN_ENGINE_API ASceneActor final : public AActor
    {
    public:

        explicit ASceneActor ( const ActorID InId ) noexcept;
        ~ASceneActor () override = default;

    public:

        void Receive ( const FMessage &InMessage ) override;

    private:

        void HandleTick ( const Float64 InDeltaTime ) const;

    private:

        /** Cached draw commands for all renderable entities */
        TMap<ActorID, Renderer::FDrawCommand> PendingDraws;
    };

} // namespace Engine

} // namespace LumenEngine