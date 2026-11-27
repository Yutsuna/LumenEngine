/**
 * @file OpenGLShader.hpp
 * @brief Declares the GLSL 460 source shader compiled on the GL Executor.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <Templates/Vector.hpp>

#include <RHI/Shaders/IRHIShader.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLShader
     * @brief Owns a stable copy of GLSL source and its compiled shader object.
     */
    class FOpenGLShader final : public RHI::IRHIShader, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLShader>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHIShaderDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLShader ( TSharedRef<FOpenGLDeviceContext> InContext,
                        const RHI::FRHIShaderDesc &InDesc,
                        RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLShader () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIShaderDesc &GetDesc () const noexcept override;

        /** @return The compiled native shader object. */
        [[nodiscard]] GLuint GetShader () const noexcept;

        /** @return The context this shader was compiled on. */
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIShaderDesc Desc;
        TVector<Byte> OwnedSource;
        GLuint Shader = 0U;
    };

} // namespace RHIOpenGL

} // namespace Lumen
