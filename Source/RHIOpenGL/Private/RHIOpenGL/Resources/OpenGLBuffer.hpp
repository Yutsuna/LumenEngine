/**
 * @file OpenGLBuffer.hpp
 * @brief Declares dedicated and placed OpenGL buffers with persistent mappings.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>

#include <Templates/Optional.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Memory/OpenGLHeap.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLBuffer
     * @brief A buffer backed by its own GL buffer or by a range of an RHI heap.
     */
    class FOpenGLBuffer final : public RHI::IRHIBuffer, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHIBufferDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLBuffer>, RHI::FRHIError>
        CreatePlaced ( TSharedRef<FOpenGLDeviceContext> InContext,
                       const RHI::FRHIBufferDesc &InDesc,
                       TSharedRef<RHI::IRHIHeap> InHeap,
                       const RHI::FRHIAllocation &InAllocation,
                       RHI::FRHIObjectId InId ) noexcept;

        FOpenGLBuffer ( TSharedRef<FOpenGLDeviceContext> InContext,
                        const RHI::FRHIBufferDesc &InDesc,
                        RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLBuffer () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIResourceRequirements &GetRequirements () const noexcept override;
        [[nodiscard]] RHI::ERHIResourceUsage GetUsage () const noexcept override;
        [[nodiscard]] RHI::ERHIResourceState GetInitialState () const noexcept override;
        [[nodiscard]] RHI::FRHIResourceLifetime GetLifetime () const noexcept override;
        [[nodiscard]] const RHI::FRHIBufferDesc &GetDesc () const noexcept override;
        [[nodiscard]] TExpected<TSpan<Byte>, RHI::FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
        void Unmap () noexcept override;

    public:

        [[nodiscard]] GLuint GetGLBuffer () const noexcept;
        [[nodiscard]] UInt64 GetGLBufferOffset () const noexcept;
        [[nodiscard]] Byte *GetMappedBase () const noexcept;
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        [[nodiscard]] RHI::FRHIExpected AllocateStorage () noexcept;
        [[nodiscard]] static RHI::FRHIExpected ValidatePlacement ( const FOpenGLDeviceContext &InContext,
                                                                   const RHI::FRHIBufferDesc &InDesc,
                                                                   RHI::IRHIHeap &InHeap,
                                                                   const RHI::FRHIAllocation &InAllocation ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIBufferDesc Desc;
        RHI::FRHIResourceRequirements Requirements;
        TOptional<TSharedRef<RHI::IRHIHeap>> ParentHeap;
        UInt64 HeapOffset = 0ULL;
        GLuint Buffer     = 0U;
        Byte *Mapped      = nullptr;
        Bool bIsMapped    = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen
