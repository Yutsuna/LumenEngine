/**
 * @file IOpenGLPlatformContext.hpp
 * @brief Declares the platform-neutral native OpenGL context seam.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Loader/GLSpec.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class IOpenGLPlatformContext
     * @brief Owns a native headless context and resolves its OpenGL entry points.
     * @details The context is created and made current by the thread that calls
     *          Initialize; no other thread may use it concurrently.
     */
    class IOpenGLPlatformContext
    {
    public:

        virtual ~IOpenGLPlatformContext () noexcept = default;

        /** @brief Creates a 4.6 core context and makes it current on this thread. */
        [[nodiscard]] virtual TExpected<void, RHI::FRHIError> Initialize ( Bool bEnableDebug ) noexcept = 0;

        /** @brief Releases the current context and destroys all native objects. */
        virtual void Shutdown () noexcept = 0;

        /** @brief Restores the context as current on this thread. */
        [[nodiscard]] virtual TExpected<void, RHI::FRHIError> MakeCurrent () noexcept = 0;

        /** @return Address of a named OpenGL entry point, or empty when unavailable. */
        [[nodiscard]] virtual TOptional<UPtr> ResolveProcedure ( FStringView InName ) const noexcept = 0;
    };

} // namespace RHIOpenGL

} // namespace Lumen
