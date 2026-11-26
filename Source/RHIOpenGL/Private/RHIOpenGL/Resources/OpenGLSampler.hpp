/**
 * @file OpenGLSampler.hpp
 * @brief Declares immutable OpenGL sampler objects.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLSampler
     * @brief An immutable DSA sampler created on the GL Executor.
     */
    class FOpenGLSampler final : public RHI::IRHISampler, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLSampler>, RHI::FRHIError> Create (
            TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHISamplerDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLSampler ( TSharedRef<FOpenGLDeviceContext> InContext,
                         const RHI::FRHISamplerDesc &InDesc,
                         RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLSampler () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHISamplerDesc &GetDesc () const noexcept override;

        [[nodiscard]] GLuint GetSampler () const noexcept;

        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHISamplerDesc Desc;
        GLuint Sampler = 0U;
    };

} // namespace RHIOpenGL

} // namespace Lumen
