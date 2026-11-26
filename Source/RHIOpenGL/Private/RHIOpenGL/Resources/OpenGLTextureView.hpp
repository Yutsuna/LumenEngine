/**
 * @file OpenGLTextureView.hpp
 * @brief Declares OpenGL texture views with parent-texture retention.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLTextureView
     * @brief A glTextureView object, or a full-range alias of its parent.
     */
    class FOpenGLTextureView final : public RHI::IRHITextureView, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLTextureView>, RHI::FRHIError> Create (
            TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHITextureViewDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLTextureView ( TSharedRef<FOpenGLDeviceContext> InContext,
                             const RHI::FRHITextureViewDesc &InDesc,
                             RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLTextureView () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHITextureViewDesc &GetDesc () const noexcept override;
        [[nodiscard]] TSharedPtr<RHI::IRHITexture> GetTexture () const noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHITextureViewDesc Desc;
        TSharedPtr<RHI::IRHITexture> Parent;
        GLuint View    = 0U;
        Bool bOwnsView = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen
