/**
 * @file OpenGLFormatMapping.hpp
 * @brief Maps portable RHI formats onto OpenGL 4.6 sized internal formats.
 */

#pragma once

#include <RHI/Types/RHIFormat.hpp>

#include <RHIOpenGL/Loader/GLSpec.hpp>

#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @struct FOpenGLFormatMapping
     * @brief Sized internal format plus a conservative upload/download description.
     */
    struct FOpenGLFormatMapping
    {
        GLenum InternalFormat = 0U;
        GLenum ExternalFormat = 0U;
        GLenum Type           = 0U;
        GLint Components      = 1;
        Bool bCompressed      = false;
        Bool bDepthStencil    = false;
    };

    /**
     * @brief Maps a portable RHI format onto a sized OpenGL internal format.
     * @return The GL mapping, or an empty optional for formats without a GL 4.6 mapping.
     */
    [[nodiscard]] constexpr TOptional<FOpenGLFormatMapping> MapOpenGLFormat ( const RHI::ERHIFormat InFormat ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen

#include "Inline/OpenGLFormatMapping.inl"
