/**
 * @file OpenGLDeviceContext.hpp
 * @brief Declares shared OpenGL device execution state and status propagation.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Debug/IRHIDebugSink.hpp>
#include <RHI/Device/RHIDeviceStatus.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Device/OpenGLContext.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @struct FOpenGLNativeLimits
     * @brief Backend-local binding capacities queried once on the GL Executor.
     * @details These are not public RHI limits: pipeline layouts validate their flat
     *          unit assignment against these native capacities.
     */
    struct FOpenGLNativeLimits
    {
        GLint MaxUniformBufferBindings       = 0;
        GLint MaxShaderStorageBufferBindings = 0;
        GLint MaxCombinedTextureImageUnits   = 0;
        GLint MaxImageUnits                  = 0;
    };

    /**
     * @class FOpenGLDeviceContext
     * @brief Holds the GL executor, negotiated contract, and monotonic device status.
     */
    class FOpenGLDeviceContext final : public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Constructs the shared state from an already created GL executor. */
        FOpenGLDeviceContext ( TSharedRef<FOpenGLContext> InContext,
                               RHI::FRHICapabilities InCapabilities,
                               RHI::FRHILimits InLimits,
                               FOpenGLNativeLimits InNativeLimits ) noexcept;

        /** @brief Creates the worker context from a probed adapter descriptor. */
        [[nodiscard]] static TExpected<TSharedRef<FOpenGLDeviceContext>, RHI::FRHIError>
        Create ( const RHI::FRHIAdapterInfo &InAdapter,
                 Bool bEnableDebug,
                 RHI::IRHIDebugSink *InDebugSink,
                 const FOpenGLNativeWindowDesc *InNativeWindow = nullptr ) noexcept;

    public:

        [[nodiscard]] const FGLDispatchTable &GetDispatch () const noexcept;

        [[nodiscard]] TSharedRef<FOpenGLContext> GetExecutor () const noexcept;

        /** @return Address of the executor-owned leaf state used by host-side waits. */
        [[nodiscard]] FOpenGLWaitState *GetWaitState () const noexcept;

        /** @return True when status is not Ready or the executor observed a reset. */
        [[nodiscard]] Bool IsUnavailable () const noexcept;

        [[nodiscard]] const RHI::FRHICapabilities &GetCapabilities () const noexcept;

        [[nodiscard]] const RHI::FRHILimits &GetLimits () const noexcept;

        /** @return Native binding capacities cached from the GL Executor. */
        [[nodiscard]] const FOpenGLNativeLimits &GetNativeLimits () const noexcept;

        /** @return DeviceLost/DeviceClosed error when operations are unavailable. */
        [[nodiscard]] TOptional<RHI::FRHIError> GetUnavailableError ( FStringView InOperation ) const noexcept;

        void MarkLost () noexcept;
        void MarkClosed () noexcept;
        void DetachDebugSink () noexcept;

        /** @return True when the GL executor observed a graphics reset. */
        [[nodiscard]] Bool IsDeviceLost () const noexcept;

    private:

        TSharedRef<FOpenGLContext> Context;
        RHI::FRHICapabilities Capabilities;
        RHI::FRHILimits Limits;
        FOpenGLNativeLimits NativeLimits;
        mutable TAtomic<RHI::ERHIDeviceStatus> Status{ RHI::ERHIDeviceStatus::Ready };
    };

} // namespace RHIOpenGL

} // namespace Lumen
