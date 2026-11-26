/**
 * @file OpenGLHeap.hpp
 * @brief Declares an RHI heap backed by one OpenGL buffer object.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>

#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLHeap
     * @brief Device-local storage or a persistently mapped CPU-visible arena.
     */
    class FOpenGLHeap final : public RHI::IRHIHeap, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLHeap>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHIHeapDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLHeap () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIHeapDesc &GetDesc () const noexcept override;
        [[nodiscard]] TExpected<TSpan<Byte>, RHI::FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
        void Unmap () noexcept override;

    public:

        [[nodiscard]] GLuint GetBuffer () const noexcept;
        [[nodiscard]] Byte *GetMappedBase () const noexcept;
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

        FOpenGLHeap ( TSharedRef<FOpenGLDeviceContext> InContext,
                      const RHI::FRHIHeapDesc &InDesc,
                      RHI::FRHIObjectId InId ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIHeapDesc Desc;
        GLuint Buffer = 0U;
        Byte *Mapped  = nullptr;
        Bool bMapped  = false;
    };

} // namespace RHIOpenGL

} // namespace Lumen
