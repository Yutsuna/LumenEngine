/**
 * @file OpenGLStagingAllocator.hpp
 * @brief Declares persistent upload/readback rings for OpenGL staging buffers.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIRingAllocator.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Resources/OpenGLBuffer.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLUploadAllocator
     * @brief A ring over a persistently mapped, coherent upload buffer.
     */
    class FOpenGLUploadAllocator final : public RHI::IRHIUploadAllocator, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLUploadAllocator>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 UInt64 InCapacity,
                 RHI::FRHIObjectId InId,
                 RHI::FRHIObjectId InBufferId ) noexcept;

    public:

        FOpenGLUploadAllocator ( TSharedRef<FOpenGLBuffer> InBuffer, RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLUploadAllocator () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TExpected<RHI::FRHIUploadAllocation, RHI::FRHIError>
        Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Free ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

    private:

        [[nodiscard]] Bool Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLBuffer> Buffer;
        RHI::FRHIRingAllocator Ring;
    };

    /**
     * @class FOpenGLReadbackAllocator
     * @brief A ring over a persistently mapped, coherent readback buffer.
     */
    class FOpenGLReadbackAllocator final : public RHI::IRHIReadbackAllocator, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLReadbackAllocator>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 UInt64 InCapacity,
                 RHI::FRHIObjectId InId,
                 RHI::FRHIObjectId InBufferId ) noexcept;

        FOpenGLReadbackAllocator ( TSharedRef<FOpenGLBuffer> InBuffer, RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLReadbackAllocator () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TExpected<RHI::FRHIAllocation, RHI::FRHIError>
        Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept override;

        [[nodiscard]] TExpected<RHI::FRHIReadbackAllocation, RHI::FRHIError>
        Map ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        GetBuffer ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Free ( const RHI::FRHIAllocation &InAllocation ) noexcept override;

    private:

        [[nodiscard]] Bool Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLBuffer> Buffer;
        RHI::FRHIRingAllocator Ring;
    };

} // namespace RHIOpenGL

} // namespace Lumen
