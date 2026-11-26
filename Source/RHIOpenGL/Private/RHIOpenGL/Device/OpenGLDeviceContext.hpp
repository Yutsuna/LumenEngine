/**
 * @file OpenGLDeviceContext.hpp
 * @brief Declares shared OpenGL device execution state and status propagation.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

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
     * @class FOpenGLDeviceContext
     * @brief Holds the GL executor, negotiated contract, and monotonic device status.
     */
    class FOpenGLDeviceContext final : public FNonCopyable, public FNonMovable
    {
    public:

        /** @brief Constructs the shared state from an already created GL executor. */
        FOpenGLDeviceContext ( TSharedRef<FOpenGLContext> InContext,
                               RHI::FRHICapabilities InCapabilities,
                               RHI::FRHILimits InLimits ) noexcept;

        /** @brief Creates the worker context from a probed adapter descriptor. */
        [[nodiscard]] static TExpected<TSharedRef<FOpenGLDeviceContext>, RHI::FRHIError>
        Create ( const RHI::FRHIAdapterInfo &InAdapter, Bool bEnableDebug ) noexcept;

    public:

        [[nodiscard]] const FGLDispatchTable &GetDispatch () const noexcept;

        [[nodiscard]] TSharedRef<FOpenGLContext> GetExecutor () const noexcept;

        [[nodiscard]] const RHI::FRHICapabilities &GetCapabilities () const noexcept;

        [[nodiscard]] const RHI::FRHILimits &GetLimits () const noexcept;

        /** @return DeviceLost/DeviceClosed error when operations are unavailable. */
        [[nodiscard]] TOptional<RHI::FRHIError> GetUnavailableError ( FStringView InOperation ) const noexcept;

        void MarkLost () noexcept;
        void MarkClosed () noexcept;

    private:

        TSharedRef<FOpenGLContext> Context;
        RHI::FRHICapabilities Capabilities;
        RHI::FRHILimits Limits;
        TAtomic<RHI::ERHIDeviceStatus> Status{ RHI::ERHIDeviceStatus::Ready };
    };

} // namespace RHIOpenGL

} // namespace Lumen
