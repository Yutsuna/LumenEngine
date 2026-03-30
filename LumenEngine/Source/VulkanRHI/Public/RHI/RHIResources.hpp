/**
 * @file RHIResources.hpp
 * @brief Abstract interfaces for all RHI resources (Buffers, Textures, PSOs).
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class IRHIResource
     * @brief Base interface for all GPU resources, enabling deferred deletion.
     */
    class IRHIResource
    {
    public:

        virtual ~IRHIResource () = default;

        /** @brief Immediate release of the resource */
        virtual void Release () = 0;

        /** @brief Defers the release until the GPU is done with the frame */
        virtual void DeferredRelease () = 0;
    };

    /**
     * @class FRHIBuffer
     * @brief Abstract interface for GPU Buffers.
     */
    class FRHIBuffer : public IRHIResource
    {
    public:

        [[nodiscard]] virtual UInt32 GetSize () const = 0;
        virtual void *Map ()                          = 0;
        virtual void Unmap ()                         = 0;
    };

    /**
     * @class FRHIVertexBuffer
     * @brief Abstract interface for Vertex Buffers.
     */
    class FRHIVertexBuffer : public FRHIBuffer
    {
    };

    /**
     * @class FRHIIndexBuffer
     * @brief Abstract interface for Index Buffers.
     */
    class FRHIIndexBuffer : public FRHIBuffer
    {
    };

    /**
     * @class FRHITexture
     * @brief Abstract interface for GPU Textures.
     */
    class FRHITexture : public IRHIResource
    {
    public:

        [[nodiscard]] virtual UInt32 GetWidth () const  = 0;
        [[nodiscard]] virtual UInt32 GetHeight () const = 0;
    };

    /**
     * @struct FRHIGraphicsPipelineStateInitializer
     * @brief Descriptor to create a Pipeline State Object (PSO).
     */
    struct FRHIGraphicsPipelineStateInitializer
    {
        TVector<UInt32> VertexShaderSPIRV;
        TVector<UInt32> FragmentShaderSPIRV;
        Bool bDepthTestEnable = true;
        Bool bBlendEnable     = false;
    };

    /**
     * @class FRHIPipelineState
     * @brief Abstract interface for Pipeline State Objects (PSOs).
     */
    class FRHIPipelineState : public IRHIResource
    {
    public:

        ~FRHIPipelineState () override = default;
    };

} // namespace RHI

} // namespace LumenEngine
