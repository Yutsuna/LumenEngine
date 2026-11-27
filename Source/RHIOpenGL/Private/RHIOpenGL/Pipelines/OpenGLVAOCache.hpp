/**
 * @file OpenGLVAOCache.hpp
 * @brief Declares the GL-thread cache of vertex array objects.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Pipelines/RHIVertexLayout.hpp>
#include <RHI/Types/RHIError.hpp>

#include <Templates/Map.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLVAOCache
     * @brief Owns one DSA vertex array object per canonical vertex layout hash.
     */
    class FOpenGLVAOCache final : public FNonCopyable, public FNonMovable
    {
    public:

        explicit FOpenGLVAOCache ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept;

        ~FOpenGLVAOCache () noexcept;

        /** @return A valid VAO for InLayout, created on the GL Executor on a miss. */
        [[nodiscard]] TExpected<GLuint, RHI::FRHIError> GetOrCreate ( const RHI::FRHIVertexLayout &InLayout ) noexcept;

    private:

        TSharedRef<FOpenGLDeviceContext> Context;
        TMap<UInt64, GLuint> VAOs;
    };

} // namespace RHIOpenGL

} // namespace Lumen
