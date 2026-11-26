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
           SamplerParameterf != nullptr;
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

    if ( not OutDispatch.IsComplete() )
    {
        return Lumen::TUnexpected( Lumen::RHI::MakeRHIUnsupported(
            Lumen::RHI::ERHIErrorCondition::Capability, "The EGL context is missing required OpenGL 4.6 core entry points." ) );
    }
    return {};
}
