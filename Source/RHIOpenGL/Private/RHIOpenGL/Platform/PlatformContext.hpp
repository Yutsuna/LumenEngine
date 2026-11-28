/**
 * @file PlatformContext.hpp
 * @brief Selects the native OpenGL context provider for the host platform.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #include <RHIOpenGL/Platform/Windows/WGLPlatformContext.hpp>
#elif defined( LUMEN_PLATFORM_LINUX ) or defined( LUMEN_PLATFORM_MACOS )
    #include <RHIOpenGL/Platform/Linux/EGLPlatformContext.hpp>
#endif

namespace Lumen
{

namespace RHIOpenGL
{

#if defined( LUMEN_PLATFORM_WINDOWS )
    using FPlatformContext = FWGLPlatformContext;
#elif defined( LUMEN_PLATFORM_LINUX ) or defined( LUMEN_PLATFORM_MACOS )
    using FPlatformContext = FEGLPlatformContext;
#endif

} // namespace RHIOpenGL

} // namespace Lumen
