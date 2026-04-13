/**
 * @file CameraActor.hpp
 * @brief Declaration of the base FCameraActor class.
 */

#pragma once

#include "Actor/Actor.hpp"
#include "Maths/Camera.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class FCameraActor
     * @brief Abstract base class for all camera actors.
     * @details Encapsulates the Maths::FCamera and provides base message handling.
     */
    class LUMEN_ENGINE_API FCameraActor : public AActor
    {
    public:

        explicit FCameraActor ( const ActorID InId ) noexcept;
        ~FCameraActor () override = default;

    public:

        /** @brief Overrides the base actor receive to route messages. */
        void Receive ( FMessage InMessage ) override;

        /** @brief Retrieve the mathematical camera state. */
        [[nodiscard]] const Maths::FCamera &GetCamera () const noexcept;

    protected:

        /** @brief Called every frame. Must be implemented by derived cameras. */
        virtual void Tick ( const Float64 InDeltaTime ) = 0;

        /** @brief Called for unhandled messages. Can be overridden. */
        virtual void HandleMessage ( const FMessage &InMessage );

    protected:

        Maths::FCamera Camera;

        Maths::FVec3f Position{ 0.0F, 0.0F, 0.0F };
        Float32 Pitch = 0.0F;
        Float32 Yaw   = 0.0F;
    };

} // namespace Engine

} // namespace LumenEngine