/**
 * @file RHIOpenGLModule.hpp
 * @brief Declares the OpenGL RHI module entry point.
 */

#pragma once

#include <RHIOpenGLExport.hpp>

#include <RHI/Module/IRHIModule.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @brief Creates and initializes the OpenGL RHI module instance.
     * @return A `TExpected` containing the initialized module instance or an error.
     */
    [[nodiscard]] RHIOPENGL_EXPORT TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> CreateRHIOpenGLModule () noexcept;

    /** @brief Creates a module whose probe and devices share the native window's EGL display. */
    [[nodiscard]] RHIOPENGL_EXPORT TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError>
    CreateRHIOpenGLModule ( const FOpenGLNativeWindowDesc &InNativeWindow ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen
