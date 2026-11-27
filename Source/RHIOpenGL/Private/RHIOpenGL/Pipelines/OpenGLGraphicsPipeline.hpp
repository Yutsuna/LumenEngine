/**
 * @file OpenGLGraphicsPipeline.hpp
 * @brief Declares the immutable OpenGL graphics pipeline.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Pipelines/OpenGLVAOCache.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    class FOpenGLPipelineLayout;

    /**
     * @class FOpenGLGraphicsPipeline
     * @brief Linked GLSL program plus canonical graphics compatibility metadata.
     */
    class FOpenGLGraphicsPipeline final : public RHI::IRHIPipeline, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLGraphicsPipeline>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHIGraphicsPipelineDesc &InDesc,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLGraphicsPipeline ( TSharedRef<FOpenGLDeviceContext> InContext,
                                  const RHI::FRHIGraphicsPipelineDesc &InDesc,
                                  RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLGraphicsPipeline () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] RHI::ERHIPipelineType GetType () const noexcept override;

        [[nodiscard]] const RHI::FRHIPipelineCompatibility &GetCompatibility () const noexcept override;

        /** @return The linked native program object. */
        [[nodiscard]] GLuint GetProgram () const noexcept;

        /** @return The cached vertex array object for this pipeline's vertex layout. */
        [[nodiscard]] GLuint GetVertexArray () const noexcept;

        /** @return The immutable creation descriptor, including dynamic state flags. */
        [[nodiscard]] const RHI::FRHIGraphicsPipelineDesc &GetDesc () const noexcept;

        /** @return The native pipeline layout, or empty when the layout is foreign. */
        [[nodiscard]] TOptional<FOpenGLPipelineLayout *> GetNativeLayout () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIGraphicsPipelineDesc Desc;
        GLuint Program     = 0U;
        GLuint VertexArray = 0U;
        RHI::FRHIPipelineCompatibility Compatibility;
        FOpenGLVAOCache VAOCache;
    };

} // namespace RHIOpenGL

} // namespace Lumen
