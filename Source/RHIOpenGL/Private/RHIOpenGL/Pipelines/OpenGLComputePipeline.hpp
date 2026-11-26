/**
 * @file OpenGLComputePipeline.hpp
 * @brief Declares the immutable OpenGL compute pipeline.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLComputePipeline
     * @brief Linked GLSL compute program plus canonical compatibility metadata.
     */
    class FOpenGLComputePipeline final : public RHI::IRHIPipeline, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLComputePipeline>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHIComputePipelineDesc &InDesc,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLComputePipeline ( TSharedRef<FOpenGLDeviceContext> InContext,
                                 const RHI::FRHIComputePipelineDesc &InDesc,
                                 RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLComputePipeline () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] RHI::ERHIPipelineType GetType () const noexcept override;

        [[nodiscard]] const RHI::FRHIPipelineCompatibility &GetCompatibility () const noexcept override;

        /** @return The linked native program object. */
        [[nodiscard]] GLuint GetProgram () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIComputePipelineDesc Desc;
        GLuint Program = 0U;
        RHI::FRHIPipelineCompatibility Compatibility;
    };

} // namespace RHIOpenGL

} // namespace Lumen
