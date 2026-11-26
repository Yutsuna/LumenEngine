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

        /* Shader, program and resource binding. */
        TGLCreateShader CreateShader                           = nullptr;
        TGLDeleteShader DeleteShader                           = nullptr;
        TGLShaderSource ShaderSource                           = nullptr;
        TGLCompileShader CompileShader                         = nullptr;
        TGLGetShaderiv GetShaderiv                             = nullptr;
        TGLGetShaderInfoLog GetShaderInfoLog                   = nullptr;
        TGLCreateProgram CreateProgram                         = nullptr;
        TGLDeleteProgram DeleteProgram                         = nullptr;
        TGLAttachShader AttachShader                           = nullptr;
        TGLLinkProgram LinkProgram                             = nullptr;
        TGLGetProgramiv GetProgramiv                           = nullptr;
        TGLGetProgramInfoLog GetProgramInfoLog                 = nullptr;
        TGLUseProgram UseProgram                               = nullptr;
        TGLUniformBlockBinding UniformBlockBinding             = nullptr;
        TGLShaderStorageBlockBinding ShaderStorageBlockBinding = nullptr;
        TGLGetProgramResourceIndex GetProgramResourceIndex     = nullptr;
        TGLProgramUniform1i ProgramUniform1i                   = nullptr;
        TGLGetUniformLocation GetUniformLocation               = nullptr;
        TGLBindBufferBase BindBufferBase                       = nullptr;
        TGLBindBufferRange BindBufferRange                     = nullptr;
        TGLBindTextureUnit BindTextureUnit                     = nullptr;
        TGLBindSampler BindSampler                             = nullptr;
        TGLMemoryBarrier MemoryBarrier                         = nullptr;

        /* Vertex array objects. */
        TGLCreateVertexArrays CreateVertexArrays               = nullptr;
        TGLDeleteVertexArrays DeleteVertexArrays               = nullptr;
        TGLBindVertexArray BindVertexArray                     = nullptr;
        TGLVertexArrayAttribFormat VertexArrayAttribFormat     = nullptr;
        TGLVertexArrayAttribBinding VertexArrayAttribBinding   = nullptr;
        TGLEnableVertexArrayAttrib EnableVertexArrayAttrib     = nullptr;
        TGLVertexArrayBindingDivisor VertexArrayBindingDivisor = nullptr;
        TGLVertexArrayVertexBuffer VertexArrayVertexBuffer     = nullptr;
        TGLVertexArrayElementBuffer VertexArrayElementBuffer   = nullptr;

        /* Framebuffer objects, clearing and fixed-function state. */
        TGLCreateFramebuffers CreateFramebuffers                         = nullptr;
        TGLDeleteFramebuffers DeleteFramebuffers                         = nullptr;
        TGLBindFramebuffer BindFramebuffer                               = nullptr;
        TGLNamedFramebufferTexture NamedFramebufferTexture               = nullptr;
        TGLNamedFramebufferTextureLayer NamedFramebufferTextureLayer     = nullptr;
        TGLNamedFramebufferDrawBuffers NamedFramebufferDrawBuffers       = nullptr;
        TGLNamedFramebufferReadBuffer NamedFramebufferReadBuffer         = nullptr;
        TGLCheckNamedFramebufferStatus CheckNamedFramebufferStatus       = nullptr;
        TGLClearNamedFramebufferfv ClearNamedFramebufferfv               = nullptr;
        TGLClearNamedFramebufferiv ClearNamedFramebufferiv               = nullptr;
        TGLClearNamedFramebufferuiv ClearNamedFramebufferuiv             = nullptr;
        TGLClearNamedFramebufferfi ClearNamedFramebufferfi               = nullptr;
        TGLInvalidateNamedFramebufferData InvalidateNamedFramebufferData = nullptr;
        TGLViewport Viewport                                             = nullptr;
        TGLScissor Scissor                                               = nullptr;
        TGLEnable Enable                                                 = nullptr;
        TGLDisable Disable                                               = nullptr;
        TGLPixelStorei PixelStorei                                       = nullptr;

        /* Draw commands. */
        TGLDrawArrays DrawArrays                           = nullptr;
        TGLDrawElements DrawElements                       = nullptr;
        TGLDrawArraysIndirect DrawArraysIndirect           = nullptr;
        TGLDrawElementsIndirect DrawElementsIndirect       = nullptr;
        TGLDispatchCompute DispatchCompute                 = nullptr;
        TGLDispatchComputeIndirect DispatchComputeIndirect = nullptr;

        /* Texture transfers and queries. */
        TGLTextureSubImage2D TextureSubImage2D     = nullptr;
        TGLCopyImageSubData CopyImageSubData       = nullptr;
        TGLCreateQueries CreateQueries             = nullptr;
        TGLDeleteQueries DeleteQueries             = nullptr;
        TGLBeginQuery BeginQuery                   = nullptr;
        TGLEndQuery EndQuery                       = nullptr;
        TGLGetQueryObjectui64v GetQueryObjectui64v = nullptr;
        TGLGetQueryiv GetQueryiv                   = nullptr;

        /** @return True when every required core entry point was resolved. */
        [[nodiscard]] Bool IsComplete () const noexcept;
    };

} // namespace RHIOpenGL

} // namespace Lumen
