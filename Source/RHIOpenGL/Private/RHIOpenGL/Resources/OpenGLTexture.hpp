/**
 * @file OpenGLTexture.hpp
 * @brief Declares immutable-storage OpenGL textures with placement bookkeeping.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    class FOpenGLHeap;

    /**
     * @class FOpenGLTexture
     * @brief An RHI texture using glTextureStorage and a stable GL target.
     */
    class FOpenGLTexture final : public RHI::IRHITexture, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLTexture>, RHI::FRHIError> Create (
            TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHITextureDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLTexture>, RHI::FRHIError>
        CreatePlaced ( TSharedRef<FOpenGLDeviceContext> InContext,
                       const RHI::FRHITextureDesc &InDesc,
                       TSharedRef<RHI::IRHIHeap> InHeap,
                       const RHI::FRHIAllocation &InAllocation,
                       RHI::FRHIObjectId InId ) noexcept;

        /** @brief Exact CPU-side byte requirement for an immutable texture. */
        [[nodiscard]] static TOptional<UInt64> ComputeTextureRequirements ( const RHI::FRHITextureDesc &InDesc,
                                                                            UInt64 InMinAlignment ) noexcept;

        /** @brief Unaligned byte requirement used to validate a placed allocation. */
        [[nodiscard]] static TOptional<UInt64> ComputeTightTextureBytes ( const RHI::FRHITextureDesc &InDesc ) noexcept;

        FOpenGLTexture ( TSharedRef<FOpenGLDeviceContext> InContext,
                         const RHI::FRHITextureDesc &InDesc,
                         RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLTexture () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIResourceRequirements &GetRequirements () const noexcept override;
        [[nodiscard]] RHI::ERHIResourceUsage GetUsage () const noexcept override;
        [[nodiscard]] RHI::ERHIResourceState GetInitialState () const noexcept override;
        [[nodiscard]] RHI::FRHIResourceLifetime GetLifetime () const noexcept override;
        [[nodiscard]] const RHI::FRHITextureDesc &GetDesc () const noexcept override;

    public:

        [[nodiscard]] GLuint GetTexture () const noexcept;
        [[nodiscard]] GLenum GetTarget () const noexcept;
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        [[nodiscard]] RHI::FRHIExpected AllocateStorage () noexcept;
        [[nodiscard]] static RHI::FRHIExpected ValidateLimits ( const FOpenGLDeviceContext &InContext,
                                                                const RHI::FRHITextureDesc &InDesc ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHITextureDesc Desc;
        RHI::FRHIResourceRequirements Requirements;
        TOptional<TSharedRef<RHI::IRHIHeap>> ParentHeap;
        GLuint Texture         = 0U;
        GLenum Target          = 0U;
        UInt64 PlacementOffset = 0ULL;
    };

} // namespace RHIOpenGL

} // namespace Lumen
