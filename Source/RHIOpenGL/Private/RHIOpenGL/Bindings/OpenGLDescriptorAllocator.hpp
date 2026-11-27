/**
 * @file OpenGLDescriptorAllocator.hpp
 * @brief Declares device-scoped descriptor pool tokens and the OpenGL allocator.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Bindings/IRHIDescriptorPoolBackend.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorBase.hpp>

#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLDescriptorPoolBackend
     * @brief Hands out deterministic native pool tokens; OpenGL slots are emulated.
     */
    class FOpenGLDescriptorPoolBackend final : public RHI::IRHIDescriptorPoolBackend, public FNonCopyable, public FNonMovable
    {
    public:

        FOpenGLDescriptorPoolBackend () noexcept           = default;
        ~FOpenGLDescriptorPoolBackend () noexcept override = default;

        [[nodiscard]] TExpected<UInt64, RHI::FRHIError> CreatePool ( UInt32 InCapacity ) noexcept override;

        void ReleasePool ( UInt64 InPool ) noexcept override;

    private:

        UInt64 NextPool = 1ULL;
    };

    /**
     * @class FOpenGLDescriptorAllocator
     * @brief Fixed-range descriptor allocator built on the portable common base.
     */
    class FOpenGLDescriptorAllocator final : public RHI::FRHIDescriptorAllocatorBase, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLDescriptorAllocator>, RHI::FRHIError>
        Create ( const RHI::FRHIDescriptorAllocatorDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLDescriptorAllocator ( const RHI::FRHIDescriptorAllocatorDesc &InDesc,
                                     RHI::IRHIDescriptorPoolBackend &InBackend,
                                     TSharedRef<FOpenGLDescriptorPoolBackend> InBackendOwner,
                                     RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLDescriptorAllocator () noexcept override;

        LUMEN_RHI_OBJECT_DECLARATIONS;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDescriptorPoolBackend> BackendOwner;
    };

} // namespace RHIOpenGL

} // namespace Lumen
