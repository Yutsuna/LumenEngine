/**
 * @file OpenGLCapabilityProbe.hpp
 * @brief Declares adapter identity, capability, and limit discovery from OpenGL.
 */

#pragma once

#include <RHI/Module/RHIAdapterInfo.hpp>

#include <RHIOpenGL/Loader/GLDispatchTable.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @brief Builds one adapter descriptor from a current OpenGL 4.6 core context.
     * @param OutInfo Destination adapter information.
     * @param InDispatch Resolved entry points for the current context.
     * @return Unsupported/Capability when the context is below OpenGL 4.6.
     */
    [[nodiscard]] TExpected<void, RHI::FRHIError> ProbeOpenGLCapabilities ( RHI::FRHIAdapterInfo &OutInfo,
                                                                            const FGLDispatchTable &InDispatch ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen
