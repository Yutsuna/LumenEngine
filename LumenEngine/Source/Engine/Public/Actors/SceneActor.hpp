/**
 * @file SceneActor.hpp
 * @brief Declaration of the FSceneActor class.
 */

#pragma once

#include "Actor/Actor.hpp"
#include "Container/Map.hpp"
#include "Maths/Matrix.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class FSceneActor
     * @brief The central authority for the scene state. 
     * @details Collects transform updates from entities, maintains visibility, and pushes data to the Renderer.
     */
    class LUMEN_ENGINE_API FSceneActor final : public AActor
    {
    public:

        explicit FSceneActor ( const ActorID InId ) noexcept;
        ~FSceneActor () override = default;

    public:

        void Receive ( FMessage InMessage ) override;

    private:

        void HandleTick ( const Float64 InDeltaTime ) noexcept;
        void HandleTransformUpdate ( const ActorID InSender, const Maths::FMatrix4x4f& InTransform ) noexcept;

    private:

        /** Cached transforms for all renderable entities */
        TMap<ActorID, Maths::FMatrix4x4f> EntityTransforms;
    };

} // namespace Engine

} // namespace LumenEngine