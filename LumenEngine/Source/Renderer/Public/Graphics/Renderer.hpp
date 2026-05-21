/**
 * @file Renderer.hpp
 * @brief High-level Renderer module orchestrating the Render Graph and Features.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/String.hpp"
#include "Container/UniquePtr.hpp"
#include "Container/Vector.hpp"

#include "Graphics/RenderFeature.hpp"
#include "Graphics/RenderResource.hpp"
#include "Graphics/Resources/RenderShader.hpp"

#include "Maths/Vertex.hpp"

#include "Thread/TripleBuffer.hpp"

namespace LumenEngine
{

class FGenericWindow;

namespace RHI
{
    class IRHI;
}

namespace Compiler
{
    class FShaderCompiler;
}

namespace Renderer
{

    class LUMEN_ENGINE_API FRenderer final
    {
    public:

        FRenderer () noexcept;
        ~FRenderer () noexcept;

    public:

        /**
         * @brief Initializes the renderer.
         * @param InRHI The injected Render Hardware Interface.
         * @param InWindow The main window.
         */
        void Initialize ( TUniquePtr<RHI::IRHI> InRHI, const TSharedRef<FGenericWindow> &InWindow );

        void Shutdown () noexcept;

        /** @brief Submits a render packet from the Game Thread. */
        void SubmitRenderPacket ( const FRenderPacket &InPacket );

        /** @brief Submits global data from the Game Thread (Camera, Time). */
        void SubmitGlobalUniforms ( const RHI::FGlobalUniformData &InUniforms );

        /** @brief Executes the render graph for the frame. */
        void RenderFrame ();

    public:

        /**
         * @brief Creates a new mesh with the given vertices and indices.
         * @param InVertices The vertices of the mesh.
         * @param InIndices The indices of the mesh.
         * @return A strongly typed handle to the created mesh.
         */
        [[nodiscard]] RHI::FMeshHandle CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices );

        /**
         * @brief Creates a new graphics pipeline with the given vertex and fragment shaders.
         * @param InVertexPath The path to the vertex shader file.
         * @param InFragmentPath The path to the fragment shader file.
         * @return A strongly typed handle to the created pipeline.
         */
        [[nodiscard]] RHI::FPipelineHandle CreatePipeline ( const FString &InVertexPath, const FString &InFragmentPath );

        /**
         * @brief Creates a new material.
         * @param InShader The shader to use.
         * @return A strongly typed handle to the created pipeline.
         */
        [[nodiscard]] RHI::FPipelineHandle CreateMaterial ( const TSharedPtr<FRenderShader> &InShader );

    private:

        TUniquePtr<RHI::IRHI> RHI;
        TUniquePtr<Compiler::FShaderCompiler> ShaderCompiler;
        TVector<TUniquePtr<IRenderFeature>> Features;

        Parallel::TTripleBuffer<FRenderPacket> RenderBuffer;
        Parallel::TTripleBuffer<RHI::FGlobalUniformData> GlobalUniformBuffer;
    };

    extern LUMEN_ENGINE_API TUniquePtr<FRenderer> GRenderer;

} // namespace Renderer

} // namespace LumenEngine
