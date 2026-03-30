/**
 * @file RHICommandList.hpp
 * @brief Abstract interface for recording graphics and compute commands.
 */

#pragma once

#include "Container/SharedPtr.hpp"
#include "CoreTypes.hpp"
#include "RHIResources.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class FRHICommandList
     * @brief High-level command list for recording GPU operations safely.
     */
    class FRHICommandList
    {
    public:

        virtual ~FRHICommandList () = default;

    public:

        /** Begins recording commands */
        virtual void Begin () = 0;

        /** Ends recording commands */
        virtual void End () = 0;

        /** Binds a Pipeline State Object (Tracks state to avoid redundant binds) */
        virtual void SetPipelineState ( const TSharedRef<FRHIPipelineState> &InPipelineState ) = 0;

        /** Binds a Vertex Buffer */
        virtual void SetVertexBuffer ( const TSharedRef<FRHIVertexBuffer> &InVertexBuffer, UInt32 InBindingIndex = 0 ) = 0;

        /** Issues a draw call */
        virtual void DrawPrimitive ( UInt32 InVertexCount, UInt32 InInstanceCount, UInt32 InFirstVertex, UInt32 InFirstInstance ) = 0;

        /** Starts a Render Pass (simplified for this architectural example) */
        virtual void BeginRenderPass ( const TSharedRef<FRHITexture> &InRenderTarget ) = 0;
        virtual void EndRenderPass ()                                                  = 0;
    };

} // namespace RHI

} // namespace LumenEngine
