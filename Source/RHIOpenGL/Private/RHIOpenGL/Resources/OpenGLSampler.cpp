/**
 * @file OpenGLSampler.cpp
 * @brief Implements immutable OpenGL sampler state creation.
 */

#include <RHIOpenGL/Resources/OpenGLSampler.hpp>

#include <RHI/Types/RHIError.hpp>

/**
 * File-local helpers
 */

namespace
{

[[nodiscard]] Lumen::RHIOpenGL::GLenum MapFilter ( const Lumen::RHI::ERHISamplerFilter InFilter ) noexcept
{
    return InFilter == Lumen::RHI::ERHISamplerFilter::Nearest ? Lumen::RHIOpenGL::GlNearest : Lumen::RHIOpenGL::GlLinear;
}

[[nodiscard]] Lumen::RHIOpenGL::GLenum MapAddress ( const Lumen::RHI::ERHISamplerAddressMode InMode ) noexcept
{
    using Lumen::RHI::ERHISamplerAddressMode;
    switch ( InMode )
    {
    case ERHISamplerAddressMode::MirroredRepeat:
        return Lumen::RHIOpenGL::GlMirroredRepeat;
    case ERHISamplerAddressMode::ClampToEdge:
        return Lumen::RHIOpenGL::GlClampToEdge;
    case ERHISamplerAddressMode::ClampToBorder:
        return Lumen::RHIOpenGL::GlClampToBorder;
    case ERHISamplerAddressMode::Repeat:
        return Lumen::RHIOpenGL::GlRepeat;
    default:
        return Lumen::RHIOpenGL::GlRepeat;
    }
}

[[nodiscard]] Lumen::RHIOpenGL::GLenum MapCompare ( const Lumen::RHI::ERHICompareFunction InFunction ) noexcept
{
    using Lumen::RHI::ERHICompareFunction;
    switch ( InFunction )
    {
    case ERHICompareFunction::Never:
        return Lumen::RHIOpenGL::GlNever;
    case ERHICompareFunction::Less:
        return Lumen::RHIOpenGL::GlLess;
    case ERHICompareFunction::Equal:
        return Lumen::RHIOpenGL::GlEqual;
    case ERHICompareFunction::LessEqual:
        return Lumen::RHIOpenGL::GlLessEqual;
    case ERHICompareFunction::Greater:
        return Lumen::RHIOpenGL::GlGreater;
    case ERHICompareFunction::NotEqual:
        return Lumen::RHIOpenGL::GlNotEqual;
    case ERHICompareFunction::GreaterEqual:
        return Lumen::RHIOpenGL::GlGreaterEqual;
    case ERHICompareFunction::Always:
        return Lumen::RHIOpenGL::GlAlways;
    default:
        return Lumen::RHIOpenGL::GlAlways;
    }
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLSampler>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLSampler::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                           const RHI::FRHISamplerDesc &InDesc,
                                           const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    if ( InDesc.MaxAnisotropy > 1U and
         not InContext->GetCapabilities().Supports( RHI::ERHICapabilityFlags::AnisotropicFiltering ) )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Resource, "Anisotropic filtering is not available." ) );
    }

    TSharedRef<FOpenGLSampler> Sampler =
        MakeShared<FOpenGLSampler>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    const FGLDispatchTable &Dispatch = Sampler->Context->GetDispatch();
    GLuint NativeSampler             = 0U;
    GLenum NativeError               = GlNoError;
    const RHI::FRHIExpected Created =
        Sampler->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeSampler, &NativeError, &InDesc] () noexcept
    {
        Dispatch.CreateSamplers( 1, &NativeSampler );
        if ( NativeSampler == 0U )
        {
            return;
        }
        Dispatch.SamplerParameteri( NativeSampler, GlTextureMinFilter, static_cast<GLint>( MapFilter( InDesc.MinFilter ) ) );
        Dispatch.SamplerParameteri( NativeSampler, GlTextureMagFilter, static_cast<GLint>( MapFilter( InDesc.MagFilter ) ) );
        Dispatch.SamplerParameteri( NativeSampler, GlTextureWrapS, static_cast<GLint>( MapAddress( InDesc.AddressU ) ) );
        Dispatch.SamplerParameteri( NativeSampler, GlTextureWrapT, static_cast<GLint>( MapAddress( InDesc.AddressV ) ) );
        Dispatch.SamplerParameteri( NativeSampler, GlTextureWrapR, static_cast<GLint>( MapAddress( InDesc.AddressW ) ) );
        if ( InDesc.MaxAnisotropy > 1U )
        {
            Dispatch.SamplerParameterf( NativeSampler, GlTextureMaxAnisotropy, static_cast<GLfloat>( InDesc.MaxAnisotropy ) );
        }
        if ( InDesc.CompareFunction.HasValue() )
        {
            Dispatch.SamplerParameteri( NativeSampler, GlTextureCompareMode, static_cast<GLint>( GlCompareRefToTexture ) );
            Dispatch.SamplerParameteri( NativeSampler, GlTextureCompareFunc,
                                        static_cast<GLint>( MapCompare( *InDesc.CompareFunction ) ) );
        }
        NativeError = Dispatch.GetError();
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( NativeError != GlNoError or NativeSampler == 0U )
    {
        if ( NativeSampler != 0U )
        {
            ( void )Sampler->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeSampler] () noexcept
            {
                Dispatch.DeleteSamplers( 1, &NativeSampler );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Resource, "OpenGL refused sampler creation." ) );
    }
    Sampler->Sampler = NativeSampler;
    return Sampler;
}

Lumen::RHIOpenGL::FOpenGLSampler::FOpenGLSampler ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                   const RHI::FRHISamplerDesc &InDesc,
                                                   const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLSampler>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLSampler::~FOpenGLSampler () noexcept
{
    if ( Sampler == 0U )
    {
        return;
    }
    const GLuint NativeSampler       = Sampler;
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeSampler] () noexcept
    {
        Dispatch.DeleteSamplers( 1, &NativeSampler );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLSampler );

const Lumen::RHI::FRHISamplerDesc &Lumen::RHIOpenGL::FOpenGLSampler::GetDesc () const noexcept
{
    return Desc;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLSampler::GetSampler () const noexcept
{
    return Sampler;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLSampler::GetDeviceContext () const noexcept
{
    return *Context;
}
