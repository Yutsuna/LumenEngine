/**
 * @file RHIOpenGLSurfaceBridge.hpp
 * @brief Declares the native-window factory for the OpenGL surface bridge boundary.
 */

#pragma once

#include <RHIOpenGLExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Presentation/IRHISurfaceBridge.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @enum EOpenGLNativeWindowSystem
     * @brief Window-system ABI used to interpret opaque OpenGL surface handles.
     */
    enum class EOpenGLNativeWindowSystem : UInt8
    {
        Unknown,
        X11,
        Wayland
    };

    /**
     * @struct FOpenGLNativeWindowDesc
     * @brief Backend-local, non-owning native handles used to request an OpenGL surface bridge.
     * @details This value deliberately mirrors only scalar data. Launch performs the conversion from
     *          its window abstraction, so RHIOpenGL has no AppCore or SDL dependency.
     */
    struct FOpenGLNativeWindowDesc
    {
        EOpenGLNativeWindowSystem WindowSystem = EOpenGLNativeWindowSystem::Unknown;
        UPtr NativeWindow                      = 0U;
        UPtr NativeDisplay                     = 0U;
        UPtr NativeWindowAttachment            = 0U;
        RHI::FRHIExtent3D Extent;
    };

    /**
     * @brief Creates a surface bridge from non-owning native window handles.
     * @details The returned bridge starts detached and is attached to the device GL Executor when
     *          its first swapchain is created. The module and device must receive the same native
     *          descriptor so probing, context creation and presentation share one EGLDisplay.
     *          Wayland requires NativeWindowAttachment to carry its native EGL window.
     * @return A detached surface bridge or an invalid-argument error for incoherent handles.
     */
    [[nodiscard]] RHIOPENGL_EXPORT TExpected<TSharedRef<RHI::IRHISurfaceBridge>, RHI::FRHIError>
    CreateRHIOpenGLSurfaceBridge ( const FOpenGLNativeWindowDesc &InDesc ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen
