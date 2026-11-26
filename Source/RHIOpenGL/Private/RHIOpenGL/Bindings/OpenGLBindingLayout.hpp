/**
 * @file OpenGLBindingLayout.hpp
 * @brief Declares the immutable OpenGL binding layout.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Bindings/IRHIBindingLayout.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLBindingLayout
     * @brief Validated descriptor schema shared by OpenGL binding sets and pipelines.
     */
    class FOpenGLBindingLayout final : public RHI::IRHIBindingLayout, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLBindingLayout>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHIBindingLayoutDesc &InDesc,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLBindingLayout ( TSharedRef<FOpenGLDeviceContext> InContext,
                               const RHI::FRHIBindingLayoutDesc &InDesc,
                               RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLBindingLayout () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIBindingLayoutDesc &GetDesc () const noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIBindingLayoutDesc Desc;
    };

} // namespace RHIOpenGL

} // namespace Lumen
