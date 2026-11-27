/**
 * @file LaunchSurfaceBridge.hpp
 * @brief Declares Launch-side conversion from native window handles to an OpenGL surface bridge.
 */

#pragma once

#include <Generic/GenericNativeWindowHandles.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <RHI/Presentation/IRHISurfaceBridge.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>

#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

namespace Lumen
{

namespace Launch
{

    /**
     * @brief Converts portable native window handles into an OpenGL surface descriptor.
     * @details Window-system identity and the three opaque handle values are copied without
     *          interpretation; the caller owns none of them. Unknown systems and missing window
     *          or display handles have no coherent OpenGL interpretation and yield no value.
     * @param InHandles Portable, non-owning native window handles.
     * @param InExtent Initial drawable extent carried into the descriptor.
     * @return The backend-local descriptor, or nothing when the handles are not usable.
     */
    [[nodiscard]] TOptional<RHIOpenGL::FOpenGLNativeWindowDesc>
    MakeOpenGLNativeWindowDesc ( const FNativeWindowHandles &InHandles, const RHI::FRHIExtent3D &InExtent ) noexcept;

    /**
     * @brief Creates an RHI surface bridge from portable native window handles.
     * @details The mapping failure is reported as an invalid presentation error. Any error from the
     *          OpenGL factory is propagated unchanged: the current device uses a surfaceless EGL
     *          context, so even coherent window handles are rejected as Unsupported.
     * @param InHandles Portable, non-owning native window handles.
     * @param InExtent Initial drawable extent for the surface.
     * @return A surface bridge or a structured RHI error.
     */
    [[nodiscard]] TExpected<TSharedRef<RHI::IRHISurfaceBridge>, RHI::FRHIError>
    CreateSurfaceBridge ( const FNativeWindowHandles &InHandles, const RHI::FRHIExtent3D &InExtent ) noexcept;

} // namespace Launch

} // namespace Lumen
