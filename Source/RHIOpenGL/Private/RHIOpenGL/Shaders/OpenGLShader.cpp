/**
 * @file OpenGLShader.cpp
 * @brief Implements GLSL 460 shader compilation on the GL Executor.
 */

#include <RHIOpenGL/Shaders/OpenGLShader.hpp>

#include <RHI/Types/RHIError.hpp>

/**
 * File-local helpers
 */

namespace
{

[[nodiscard]] Lumen::RHIOpenGL::GLenum MapShaderStage ( const Lumen::RHI::ERHIShaderStage InStage ) noexcept
{
    switch ( InStage )
    {
    case Lumen::RHI::ERHIShaderStage::Vertex:
        return Lumen::RHIOpenGL::GlVertexShader;
    case Lumen::RHI::ERHIShaderStage::Pixel:
        return Lumen::RHIOpenGL::GlFragmentShader;
    case Lumen::RHI::ERHIShaderStage::Compute:
        return Lumen::RHIOpenGL::GlComputeShader;
    default:
        return 0U;
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLShader>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLShader::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                          const RHI::FRHIShaderDesc &InDesc,
                                          const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    if ( not InContext->GetCapabilities().Supports( InDesc.Format ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Shader, "Shader format is not supported by OpenGL." ) );
    }
    if ( MapShaderStage( InDesc.Stage ) == 0U or InDesc.Bytecode.empty() )
    {
        return TUnexpected( RHI::MakeRHIInvalidShader( "Shader stage is unavailable or GLSL source is empty." ) );
    }

    TSharedRef<FOpenGLShader> Shader =
        MakeShared<FOpenGLShader>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    const FGLDispatchTable &Dispatch = Shader->Context->GetDispatch();
    const GLenum NativeStage         = MapShaderStage( InDesc.Stage );
    const AnsiChar *SourceText       = reinterpret_cast<const AnsiChar *>( Shader->OwnedSource.data() );
    const GLint SourceLength         = static_cast<GLint>( Shader->OwnedSource.size() );
    GLuint NativeShader              = 0U;
    GLint CompileStatus              = 0;
    GLint InfoLength                 = 0;
    TVector<AnsiChar> InfoLog;
    GLenum NativeError = GlNoError;
    const RHI::FRHIExpected Compiled =
        Shader->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeShader, &NativeStage, &SourceText, &SourceLength,
                                                            &CompileStatus, &InfoLength, &InfoLog, &NativeError] () noexcept
    {
        NativeShader = Dispatch.CreateShader( NativeStage );
        if ( NativeShader == 0U )
        {
            NativeError = Dispatch.GetError();
            return;
        }
        Dispatch.ShaderSource( NativeShader, 1, &SourceText, &SourceLength );
        Dispatch.CompileShader( NativeShader );
        Dispatch.GetShaderiv( NativeShader, GlCompileStatus, &CompileStatus );
        if ( CompileStatus == 0 )
        {
            Dispatch.GetShaderiv( NativeShader, GlInfoLogLength, &InfoLength );
            InfoLog.resize( static_cast<USize>( InfoLength > 0 ? InfoLength : 1 ) + 1U, '\0' );
            Dispatch.GetShaderInfoLog( NativeShader, static_cast<GLsizei>( InfoLog.size() - 1U ), nullptr, InfoLog.data() );
        }
        NativeError = Dispatch.GetError();
    } );
    LUMEN_TRY_EXPECT_ERROR( Compiled );
    if ( NativeError != GlNoError or NativeShader == 0U or CompileStatus == 0 )
    {
        const FString Message = InfoLog.empty() ? FString( "OpenGL refused shader compilation." ) : FString( InfoLog.data() );
        if ( NativeShader != 0U )
        {
            ( void )Shader->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeShader] () noexcept
            {
                Dispatch.DeleteShader( NativeShader );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Shader, Message ) );
    }
    Shader->Shader = NativeShader;
    return Shader;
}

Lumen::RHIOpenGL::FOpenGLShader::FOpenGLShader ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                 const RHI::FRHIShaderDesc &InDesc,
                                                 const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLShader>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      OwnedSource( InDesc.Bytecode.begin(), InDesc.Bytecode.end() )
{
    Desc.Bytecode = TSpan<const Byte>( OwnedSource.data(), OwnedSource.size() );
}

Lumen::RHIOpenGL::FOpenGLShader::~FOpenGLShader () noexcept
{
    if ( Shader == 0U )
    {
        return;
    }
    const GLuint NativeShader = Shader;
    const RHI::FRHITimelineValue Required{ Context->GetExecutor()->GetCompletedTimeline() };
    const TSharedRef<FOpenGLContext> Executor = Context->GetExecutor();
    const FGLDispatchTable *Dispatch          = &Context->GetDispatch();
    ( void )Context->GetExecutor()->DeferRelease( Required, [Executor, Dispatch, NativeShader] () noexcept
    {
        ( void )Executor->ExecuteOnGLThread( [Dispatch, NativeShader] () noexcept
        {
            Dispatch->DeleteShader( NativeShader );
        } );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLShader );

const Lumen::RHI::FRHIShaderDesc &Lumen::RHIOpenGL::FOpenGLShader::GetDesc () const noexcept
{
    return Desc;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLShader::GetShader () const noexcept
{
    return Shader;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLShader::GetDeviceContext () const noexcept
{
    return *Context;
}
