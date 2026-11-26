/**
 * @file GLLoader.cpp
 * @brief Implements typed resolution of the OpenGL 4.6 core entry points.
 */

#include <RHIOpenGL/Loader/GLLoader.hpp>

#include <RHI/Types/RHIError.hpp>

#include <String/StringView.hpp>
#include <Templates/Optional.hpp>

/**
 * File-local helpers
 */

namespace
{

template <typename TProcedureType>
void ResolveProcedure ( TProcedureType &OutProcedure,
                        const Lumen::RHIOpenGL::IOpenGLPlatformContext &InContext,
                        const Lumen::FStringView InName ) noexcept
{
    if ( const Lumen::TOptional<Lumen::UPtr> Address = InContext.ResolveProcedure( InName ) )
    {
        OutProcedure = reinterpret_cast<TProcedureType>( *Address );
    }
}

} // namespace

/**
 * Public
 */

Lumen::Bool Lumen::RHIOpenGL::FGLDispatchTable::IsComplete () const noexcept
{
    return GetString != nullptr and GetStringi != nullptr and GetIntegerv != nullptr and GetInteger64v != nullptr and
           GetError != nullptr and ClipControl != nullptr and CreateBuffers != nullptr and DeleteBuffers != nullptr and
           NamedBufferStorage != nullptr and MapNamedBufferRange != nullptr and UnmapNamedBuffer != nullptr and
           CopyNamedBufferSubData != nullptr and CreateTextures != nullptr and DeleteTextures != nullptr and
           TextureStorage2D != nullptr and TextureStorage3D != nullptr and TextureStorage2DMultisample != nullptr and
           TextureView != nullptr and CreateSamplers != nullptr and DeleteSamplers != nullptr and SamplerParameteri != nullptr and
           SamplerParameterf != nullptr and CreateShader != nullptr and DeleteShader != nullptr and ShaderSource != nullptr and
           CompileShader != nullptr and GetShaderiv != nullptr and GetShaderInfoLog != nullptr and CreateProgram != nullptr and
           DeleteProgram != nullptr and AttachShader != nullptr and LinkProgram != nullptr and GetProgramiv != nullptr and
           GetProgramInfoLog != nullptr and UseProgram != nullptr and UniformBlockBinding != nullptr and
           ShaderStorageBlockBinding != nullptr and GetProgramResourceIndex != nullptr and ProgramUniform1i != nullptr and
           GetUniformLocation != nullptr and BindBufferBase != nullptr and BindBufferRange != nullptr and
           BindTextureUnit != nullptr and BindSampler != nullptr and MemoryBarrier != nullptr and
           CreateVertexArrays != nullptr and DeleteVertexArrays != nullptr and BindVertexArray != nullptr and
           VertexArrayAttribFormat != nullptr and VertexArrayAttribBinding != nullptr and EnableVertexArrayAttrib != nullptr and
           VertexArrayBindingDivisor != nullptr and VertexArrayVertexBuffer != nullptr and VertexArrayElementBuffer != nullptr and
           CreateFramebuffers != nullptr and DeleteFramebuffers != nullptr and BindFramebuffer != nullptr and
           NamedFramebufferTexture != nullptr and NamedFramebufferTextureLayer != nullptr and
           NamedFramebufferDrawBuffers != nullptr and NamedFramebufferReadBuffer != nullptr and
           CheckNamedFramebufferStatus != nullptr and ClearNamedFramebufferfv != nullptr and
           ClearNamedFramebufferiv != nullptr and ClearNamedFramebufferuiv != nullptr and ClearNamedFramebufferfi != nullptr and
           InvalidateNamedFramebufferData != nullptr and Viewport != nullptr and DepthRangef != nullptr and Scissor != nullptr and
           Enable != nullptr and Disable != nullptr and PixelStorei != nullptr and DrawArrays != nullptr and
           DrawElements != nullptr and DrawArraysIndirect != nullptr and DrawElementsIndirect != nullptr and
           DispatchCompute != nullptr and DispatchComputeIndirect != nullptr and TextureSubImage2D != nullptr and
           CopyImageSubData != nullptr and CreateQueries != nullptr and DeleteQueries != nullptr and BeginQuery != nullptr and
           EndQuery != nullptr and GetQueryObjectui64v != nullptr and GetQueryiv != nullptr and FenceSync != nullptr and
           DeleteSync != nullptr and ClientWaitSync != nullptr and Flush != nullptr and NamedBufferSubData != nullptr and
           BlendColor != nullptr and BlendEquationSeparate != nullptr and BlendFuncSeparate != nullptr and
           PolygonOffset != nullptr and StencilFunc != nullptr and BindBuffer != nullptr and GetBufferSubData != nullptr and
           GetTextureImage != nullptr and BlitFramebuffer != nullptr;
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::LoadGLDispatchTable ( FGLDispatchTable &OutDispatch, const IOpenGLPlatformContext &InContext ) noexcept
{
    ResolveProcedure( OutDispatch.GetString, InContext, "glGetString" );
    ResolveProcedure( OutDispatch.GetStringi, InContext, "glGetStringi" );
    ResolveProcedure( OutDispatch.GetIntegerv, InContext, "glGetIntegerv" );
    ResolveProcedure( OutDispatch.GetInteger64v, InContext, "glGetInteger64v" );
    ResolveProcedure( OutDispatch.GetError, InContext, "glGetError" );
    ResolveProcedure( OutDispatch.ClipControl, InContext, "glClipControl" );
    ResolveProcedure( OutDispatch.GetGraphicsResetStatus, InContext, "glGetGraphicsResetStatus" );
    ResolveProcedure( OutDispatch.FenceSync, InContext, "glFenceSync" );
    ResolveProcedure( OutDispatch.DeleteSync, InContext, "glDeleteSync" );
    ResolveProcedure( OutDispatch.ClientWaitSync, InContext, "glClientWaitSync" );
    ResolveProcedure( OutDispatch.Flush, InContext, "glFlush" );
    ResolveProcedure( OutDispatch.DebugMessageCallback, InContext, "glDebugMessageCallback" );
    ResolveProcedure( OutDispatch.DebugMessageControl, InContext, "glDebugMessageControl" );
    ResolveProcedure( OutDispatch.DebugMessagePushGroup, InContext, "glPushDebugGroup" );
    ResolveProcedure( OutDispatch.DebugMessagePopGroup, InContext, "glPopDebugGroup" );

    ResolveProcedure( OutDispatch.CreateBuffers, InContext, "glCreateBuffers" );
    ResolveProcedure( OutDispatch.DeleteBuffers, InContext, "glDeleteBuffers" );
    ResolveProcedure( OutDispatch.NamedBufferStorage, InContext, "glNamedBufferStorage" );
    ResolveProcedure( OutDispatch.MapNamedBufferRange, InContext, "glMapNamedBufferRange" );
    ResolveProcedure( OutDispatch.UnmapNamedBuffer, InContext, "glUnmapNamedBuffer" );
    ResolveProcedure( OutDispatch.CopyNamedBufferSubData, InContext, "glCopyNamedBufferSubData" );
    ResolveProcedure( OutDispatch.CreateTextures, InContext, "glCreateTextures" );
    ResolveProcedure( OutDispatch.DeleteTextures, InContext, "glDeleteTextures" );
    ResolveProcedure( OutDispatch.TextureStorage2D, InContext, "glTextureStorage2D" );
    ResolveProcedure( OutDispatch.TextureStorage3D, InContext, "glTextureStorage3D" );
    ResolveProcedure( OutDispatch.TextureStorage2DMultisample, InContext, "glTextureStorage2DMultisample" );
    ResolveProcedure( OutDispatch.TextureView, InContext, "glTextureView" );
    ResolveProcedure( OutDispatch.CreateSamplers, InContext, "glCreateSamplers" );
    ResolveProcedure( OutDispatch.DeleteSamplers, InContext, "glDeleteSamplers" );
    ResolveProcedure( OutDispatch.SamplerParameteri, InContext, "glSamplerParameteri" );
    ResolveProcedure( OutDispatch.SamplerParameterf, InContext, "glSamplerParameterf" );

    ResolveProcedure( OutDispatch.CreateShader, InContext, "glCreateShader" );
    ResolveProcedure( OutDispatch.DeleteShader, InContext, "glDeleteShader" );
    ResolveProcedure( OutDispatch.ShaderSource, InContext, "glShaderSource" );
    ResolveProcedure( OutDispatch.CompileShader, InContext, "glCompileShader" );
    ResolveProcedure( OutDispatch.GetShaderiv, InContext, "glGetShaderiv" );
    ResolveProcedure( OutDispatch.GetShaderInfoLog, InContext, "glGetShaderInfoLog" );
    ResolveProcedure( OutDispatch.CreateProgram, InContext, "glCreateProgram" );
    ResolveProcedure( OutDispatch.DeleteProgram, InContext, "glDeleteProgram" );
    ResolveProcedure( OutDispatch.AttachShader, InContext, "glAttachShader" );
    ResolveProcedure( OutDispatch.LinkProgram, InContext, "glLinkProgram" );
    ResolveProcedure( OutDispatch.GetProgramiv, InContext, "glGetProgramiv" );
    ResolveProcedure( OutDispatch.GetProgramInfoLog, InContext, "glGetProgramInfoLog" );
    ResolveProcedure( OutDispatch.UseProgram, InContext, "glUseProgram" );
    ResolveProcedure( OutDispatch.UniformBlockBinding, InContext, "glUniformBlockBinding" );
    ResolveProcedure( OutDispatch.ShaderStorageBlockBinding, InContext, "glShaderStorageBlockBinding" );
    ResolveProcedure( OutDispatch.GetProgramResourceIndex, InContext, "glGetProgramResourceIndex" );
    ResolveProcedure( OutDispatch.ProgramUniform1i, InContext, "glProgramUniform1i" );
    ResolveProcedure( OutDispatch.GetUniformLocation, InContext, "glGetUniformLocation" );
    ResolveProcedure( OutDispatch.BindBufferBase, InContext, "glBindBufferBase" );
    ResolveProcedure( OutDispatch.BindBufferRange, InContext, "glBindBufferRange" );
    ResolveProcedure( OutDispatch.BindTextureUnit, InContext, "glBindTextureUnit" );
    ResolveProcedure( OutDispatch.BindSampler, InContext, "glBindSampler" );
    ResolveProcedure( OutDispatch.MemoryBarrier, InContext, "glMemoryBarrier" );

    ResolveProcedure( OutDispatch.CreateVertexArrays, InContext, "glCreateVertexArrays" );
    ResolveProcedure( OutDispatch.DeleteVertexArrays, InContext, "glDeleteVertexArrays" );
    ResolveProcedure( OutDispatch.BindVertexArray, InContext, "glBindVertexArray" );
    ResolveProcedure( OutDispatch.VertexArrayAttribFormat, InContext, "glVertexArrayAttribFormat" );
    ResolveProcedure( OutDispatch.VertexArrayAttribBinding, InContext, "glVertexArrayAttribBinding" );
    ResolveProcedure( OutDispatch.EnableVertexArrayAttrib, InContext, "glEnableVertexArrayAttrib" );
    ResolveProcedure( OutDispatch.VertexArrayBindingDivisor, InContext, "glVertexArrayBindingDivisor" );
    ResolveProcedure( OutDispatch.VertexArrayVertexBuffer, InContext, "glVertexArrayVertexBuffer" );
    ResolveProcedure( OutDispatch.VertexArrayElementBuffer, InContext, "glVertexArrayElementBuffer" );

    ResolveProcedure( OutDispatch.CreateFramebuffers, InContext, "glCreateFramebuffers" );
    ResolveProcedure( OutDispatch.DeleteFramebuffers, InContext, "glDeleteFramebuffers" );
    ResolveProcedure( OutDispatch.BindFramebuffer, InContext, "glBindFramebuffer" );
    ResolveProcedure( OutDispatch.NamedFramebufferTexture, InContext, "glNamedFramebufferTexture" );
    ResolveProcedure( OutDispatch.NamedFramebufferTextureLayer, InContext, "glNamedFramebufferTextureLayer" );
    ResolveProcedure( OutDispatch.NamedFramebufferDrawBuffers, InContext, "glNamedFramebufferDrawBuffers" );
    ResolveProcedure( OutDispatch.NamedFramebufferReadBuffer, InContext, "glNamedFramebufferReadBuffer" );
    ResolveProcedure( OutDispatch.CheckNamedFramebufferStatus, InContext, "glCheckNamedFramebufferStatus" );
    ResolveProcedure( OutDispatch.ClearNamedFramebufferfv, InContext, "glClearNamedFramebufferfv" );
    ResolveProcedure( OutDispatch.ClearNamedFramebufferiv, InContext, "glClearNamedFramebufferiv" );
    ResolveProcedure( OutDispatch.ClearNamedFramebufferuiv, InContext, "glClearNamedFramebufferuiv" );
    ResolveProcedure( OutDispatch.ClearNamedFramebufferfi, InContext, "glClearNamedFramebufferfi" );
    ResolveProcedure( OutDispatch.InvalidateNamedFramebufferData, InContext, "glInvalidateNamedFramebufferData" );
    ResolveProcedure( OutDispatch.Viewport, InContext, "glViewport" );
    ResolveProcedure( OutDispatch.DepthRangef, InContext, "glDepthRangef" );
    ResolveProcedure( OutDispatch.Scissor, InContext, "glScissor" );
    ResolveProcedure( OutDispatch.Enable, InContext, "glEnable" );
    ResolveProcedure( OutDispatch.Disable, InContext, "glDisable" );
    ResolveProcedure( OutDispatch.PixelStorei, InContext, "glPixelStorei" );

    ResolveProcedure( OutDispatch.DrawArrays, InContext, "glDrawArrays" );
    ResolveProcedure( OutDispatch.DrawElements, InContext, "glDrawElements" );
    ResolveProcedure( OutDispatch.DrawArraysIndirect, InContext, "glDrawArraysIndirect" );
    ResolveProcedure( OutDispatch.DrawElementsIndirect, InContext, "glDrawElementsIndirect" );
    ResolveProcedure( OutDispatch.DispatchCompute, InContext, "glDispatchCompute" );
    ResolveProcedure( OutDispatch.DispatchComputeIndirect, InContext, "glDispatchComputeIndirect" );

    ResolveProcedure( OutDispatch.TextureSubImage2D, InContext, "glTextureSubImage2D" );
    ResolveProcedure( OutDispatch.CopyImageSubData, InContext, "glCopyImageSubData" );
    ResolveProcedure( OutDispatch.CreateQueries, InContext, "glCreateQueries" );
    ResolveProcedure( OutDispatch.DeleteQueries, InContext, "glDeleteQueries" );
    ResolveProcedure( OutDispatch.BeginQuery, InContext, "glBeginQuery" );
    ResolveProcedure( OutDispatch.EndQuery, InContext, "glEndQuery" );
    ResolveProcedure( OutDispatch.GetQueryObjectui64v, InContext, "glGetQueryObjectui64v" );
    ResolveProcedure( OutDispatch.GetQueryiv, InContext, "glGetQueryiv" );

    ResolveProcedure( OutDispatch.NamedBufferSubData, InContext, "glNamedBufferSubData" );
    ResolveProcedure( OutDispatch.BlendColor, InContext, "glBlendColor" );
    ResolveProcedure( OutDispatch.BlendEquationSeparate, InContext, "glBlendEquationSeparate" );
    ResolveProcedure( OutDispatch.BlendFuncSeparate, InContext, "glBlendFuncSeparate" );
    ResolveProcedure( OutDispatch.PolygonOffset, InContext, "glPolygonOffset" );
    ResolveProcedure( OutDispatch.StencilFunc, InContext, "glStencilFunc" );
    ResolveProcedure( OutDispatch.BindBuffer, InContext, "glBindBuffer" );
    ResolveProcedure( OutDispatch.GetBufferSubData, InContext, "glGetBufferSubData" );
    ResolveProcedure( OutDispatch.GetTextureImage, InContext, "glGetTextureImage" );
    ResolveProcedure( OutDispatch.BlitFramebuffer, InContext, "glBlitFramebuffer" );

    if ( not OutDispatch.IsComplete() )
    {
        return Lumen::TUnexpected( Lumen::RHI::MakeRHIUnsupported(
            Lumen::RHI::ERHIErrorCondition::Capability, "The EGL context is missing required OpenGL 4.6 core entry points." ) );
    }
    return {};
}
