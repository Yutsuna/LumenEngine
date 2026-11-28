/**
 * @file OpenGLSurfaceBridge.hpp
 * @brief Declares the EGL native-window surface bridge.
 */
#pragma once

#include <RHI/Presentation/IRHISurfaceBridge.hpp>
#include <RHIOpenGL/Device/OpenGLContext.hpp>
#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLSurfaceBridge
     * @brief Owns native surface requests and confines them to the device's GL Executor.
     */
    class FOpenGLSurfaceBridge final : public RHI::IRHISurfaceBridge
    {
    public:
        explicit FOpenGLSurfaceBridge ( const FOpenGLNativeWindowDesc &InDesc ) noexcept;

        void AttachContext ( TSharedRef<FOpenGLContext> InContext ) noexcept;
        void SetExtent ( const RHI::FRHIExtent3D &InExtent ) noexcept;

        [[nodiscard]] TExpected<RHI::FRHINativeSurface, RHI::FRHIError>
        CreateNativeSurface ( UPtr InBackendHandle ) noexcept override;
        void DestroyNativeSurface ( const RHI::FRHINativeSurface &InSurface ) noexcept override;
        [[nodiscard]] RHI::FRHIExpected MakeCurrent ( const RHI::FRHINativeSurface &InSurface ) noexcept override;
        [[nodiscard]] RHI::FRHIExpected Present ( const RHI::FRHINativeSurface &InSurface, Int32 InSwapInterval ) noexcept override;
        [[nodiscard]] TExpected<RHI::FRHIExtent3D, RHI::FRHIError> GetExtent () const noexcept override;
        [[nodiscard]] TOptional<UPtr> ResolveProcedure ( FStringView InName ) noexcept override;

    private:
        FOpenGLNativeWindowDesc Desc;
        TSharedPtr<FOpenGLContext> Context;
        TAtomic<UInt32> Width;
        TAtomic<UInt32> Height;
    };

} // namespace RHIOpenGL

} // namespace Lumen
