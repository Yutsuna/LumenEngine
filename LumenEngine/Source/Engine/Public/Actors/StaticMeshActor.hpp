/**
 * @file StaticMeshActor.hpp
 * @brief Declaration of the AStaticMeshActor class.
 */

#pragma once

#include "Actors/SpatialActor.hpp"
#include "Container/SharedPtr.hpp"

#include "Graphics/Resources/RenderMesh.hpp"
#include "Graphics/Resources/RenderShader.hpp"

#include "Maths/Matrix.hpp"

namespace LumenEngine
{

namespace Engine
{

    /**
     * @class AStaticMeshActor
     * @brief An actor representing a 3D static mesh in the scene.
     */
    class LUMEN_ENGINE_API AStaticMeshActor : public ASpatialActor
    {
    public:

        explicit AStaticMeshActor ( const ActorID InId ) noexcept;
        ~AStaticMeshActor () noexcept override = default;

    public:

        void Receive ( const FMessage &InMessage ) override;

        void SetMeshAndShader ( RHI::FMeshHandle InMesh, RHI::FPipelineHandle InShader ) noexcept;
        void SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept;
        void SetSceneActor ( const FActorRef &InSceneActor ) noexcept;

    protected:

        RHI::FMeshHandle Mesh;
        RHI::FPipelineHandle Shader;
        Maths::FMatrix4x4f Transform = Maths::FMatrix4x4f::Identity();

        FActorRef SceneActor;
    };

} // namespace Engine

} // namespace LumenEngine
