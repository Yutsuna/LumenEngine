/**
 * @file StaticMeshActor.hpp
 * @brief Declaration of the AStaticMeshActor class.
 */

#pragma once

#include "Actor/Actor.hpp"
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
    class LUMEN_ENGINE_API AStaticMeshActor final : public AActor
    {
    public:

        explicit AStaticMeshActor ( const ActorID InId ) noexcept;
        ~AStaticMeshActor () override = default;

    public:

        void Receive ( FMessage InMessage ) override;

        void SetMeshAndShader ( Renderer::FRenderMesh *InMesh, Renderer::FRenderShader *InShader ) noexcept;
        void SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept;
        void SetSceneActor ( const TSharedPtr<AActor> &InSceneActor ) noexcept;

    private:

        Renderer::FRenderMesh *Mesh     = nullptr;
        Renderer::FRenderShader *Shader = nullptr;
        Maths::FMatrix4x4f Transform    = Maths::FMatrix4x4f::Identity();

        TSharedPtr<AActor> SceneActor;
    };

} // namespace Engine

} // namespace LumenEngine