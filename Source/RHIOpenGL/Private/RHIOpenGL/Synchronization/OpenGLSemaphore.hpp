/**
 * @file OpenGLSemaphore.hpp
 * @brief Declares CPU timeline and binary semaphores for the OpenGL backend.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHISemaphoreDesc.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/ConditionVariable.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLSemaphore
     * @brief Timeline value with a condition variable, or a one-shot binary signal.
     * @details Signal and wait execute entirely on caller threads; no GL calls are made.
     *          Binary waits consume the signal and reset the value to zero for reuse.
     */
    class FOpenGLSemaphore final : public RHI::IRHISemaphore, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLSemaphore>, RHI::FRHIError> Create (
            TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHISemaphoreDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLSemaphore ( TSharedRef<FOpenGLDeviceContext> InContext,
                           const RHI::FRHISemaphoreDesc &InDesc,
                           RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLSemaphore () noexcept override = default;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHISemaphoreDesc &GetDesc () const noexcept override;
        [[nodiscard]] UInt64 GetValue () const noexcept override;
        RHI::FRHIExpected Signal ( UInt64 InValue ) noexcept override;
        RHI::FRHIExpected Wait ( UInt64 InValue, Float64 InTimeoutSeconds ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHISemaphoreDesc Desc;
        TAtomic<UInt64> Value{ 0ULL };
        HAL::FPlatformMutex Mutex;
        FConditionVariable Condition;
    };

} // namespace RHIOpenGL

} // namespace Lumen
