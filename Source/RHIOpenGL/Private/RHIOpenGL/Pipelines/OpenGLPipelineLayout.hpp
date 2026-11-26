/**
 * @file OpenGLPipelineLayout.hpp
 * @brief Declares the immutable OpenGL pipeline layout.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLPipelineLayout
     * @brief Validated binding layout set shared by OpenGL pipelines and binding sets.
     */
    class FOpenGLPipelineLayout final : public RHI::IRHIPipelineLayout, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLPipelineLayout>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHIPipelineLayoutDesc &InDesc,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLPipelineLayout ( TSharedRef<FOpenGLDeviceContext> InContext,
                                const RHI::FRHIPipelineLayoutDesc &InDesc,
                                RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLPipelineLayout () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIPipelineLayoutDesc &GetDesc () const noexcept override;

        /** @return Stable fold of every backend binding layout descriptor hash. */
        [[nodiscard]] UInt64 GetLayoutHash () const noexcept;

        /** @return The context this layout was created for. */
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIPipelineLayoutDesc Desc;
        UInt64 LayoutHash = 0ULL;
    };

} // namespace RHIOpenGL

} // namespace Lumen
