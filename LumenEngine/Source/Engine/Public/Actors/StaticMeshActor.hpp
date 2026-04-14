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
    class LUMEN_ENGINE_API AStaticMeshActor : public AActor
    {
    public:

        explicit AStaticMeshActor ( const ActorID InId ) noexcept;
        ~AStaticMeshActor () override = default;

    public:

        void Receive ( const FMessage &InMessage ) override;

        void SetMeshAndShader ( TSharedPtr<Renderer::FRenderMesh> InMesh, TSharedPtr<Renderer::FRenderShader> InShader ) noexcept;
        void SetTransform ( const Maths::FMatrix4x4f &InTransform ) noexcept;
        void SetSceneActor ( const FActorRef &InSceneActor ) noexcept;

    protected:

        TSharedPtr<Renderer::FRenderMesh> Mesh     = nullptr;
        TSharedPtr<Renderer::FRenderShader> Shader = nullptr;
        Maths::FMatrix4x4f Transform               = Maths::FMatrix4x4f::Identity();

        /** @note Use FActorRef instead of TSharedPtr for scene actor relationships to avoid memory cycles. */
        FActorRef SceneActor;
    };

} // namespace Engine

} // namespace LumenEngine