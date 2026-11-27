/**
 * @file GLLoader.hpp
 * @brief Declares typed resolution of the OpenGL 4.6 core entry points.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>
#include <RHIOpenGL/Platform/IOpenGLPlatformContext.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @brief Resolves required core functions and optional robustness functions.
     * @param OutDispatch Destination dispatch table.
     * @param InContext Native context that owns the entry points.
     * @return Unsupported/Capability when a required entry point is missing.
     */
    [[nodiscard]] TExpected<void, RHI::FRHIError> LoadGLDispatchTable ( FGLDispatchTable &OutDispatch,
                                                                        const IOpenGLPlatformContext &InContext ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen
