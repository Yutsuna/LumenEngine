/**
 * @file GLDispatchTable.hpp
 * @brief Declares the typed OpenGL 4.6 entry points used by RHIOpenGL.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @struct FGLDispatchTable
     * @brief Required core entry points plus optional robustness/debug entry points.
     */
    struct FGLDispatchTable
    {
        TGLGetString GetString                           = nullptr;
        TGLGetStringi GetStringi                         = nullptr;
        TGLGetIntegerv GetIntegerv                       = nullptr;
        TGLGetInteger64v GetInteger64v                   = nullptr;
        TGLGetError GetError                             = nullptr;
        TGLClipControl ClipControl                       = nullptr;
        TGLGetGraphicsResetStatus GetGraphicsResetStatus = nullptr;

        TGLCreateBuffers CreateBuffers                             = nullptr;
        TGLDeleteBuffers DeleteBuffers                             = nullptr;
        TGLNamedBufferStorage NamedBufferStorage                   = nullptr;
        TGLMapNamedBufferRange MapNamedBufferRange                 = nullptr;
        TGLUnmapNamedBuffer UnmapNamedBuffer                       = nullptr;
        TGLCopyNamedBufferSubData CopyNamedBufferSubData           = nullptr;
        TGLCreateTextures CreateTextures                           = nullptr;
        TGLDeleteTextures DeleteTextures                           = nullptr;
        TGLTextureStorage2D TextureStorage2D                       = nullptr;
        TGLTextureStorage3D TextureStorage3D                       = nullptr;
        TGLTextureStorage2DMultisample TextureStorage2DMultisample = nullptr;
        TGLTextureView TextureView                                 = nullptr;
        TGLCreateSamplers CreateSamplers                           = nullptr;
        TGLDeleteSamplers DeleteSamplers                           = nullptr;
        TGLSamplerParameteri SamplerParameteri                     = nullptr;
        TGLSamplerParameterf SamplerParameterf                     = nullptr;

        /** @return True when every required core entry point was resolved. */
        [[nodiscard]] Bool IsComplete () const noexcept;
    };

} // namespace RHIOpenGL

} // namespace Lumen
