/**
 * @file OpenGLTextureView.cpp
 * @brief Implements texture view creation and immutable parent retention.
 */

#include <RHIOpenGL/Resources/OpenGLTextureView.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Resources/OpenGLTexture.hpp>
#include <RHIOpenGL/Types/OpenGLFormatMapping.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLTextureView>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLTextureView::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                               const RHI::FRHITextureViewDesc &InDesc,
                                               const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    const TOptional<FOpenGLTexture *> Parent = RHI::TryRHIBackendCast<FOpenGLTexture>( *InDesc.Texture );
    if ( not Parent or &( *Parent )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Texture view parent is not an OpenGL texture on this device." ) );
    }

    TSharedRef<FOpenGLTextureView> View =
        MakeShared<FOpenGLTextureView>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );

    const RHI::FRHITextureDesc &ParentDesc = ( *Parent )->GetDesc();
    const Bool bFullRange                  = InDesc.Range.BaseMip == 0U and InDesc.Range.MipCount == ParentDesc.MipCount and
                            InDesc.Range.BaseLayer == 0U and InDesc.Range.LayerCount == ParentDesc.LayerCount and
                            ( InDesc.Format == RHI::ERHIFormat::Unknown or InDesc.Format == ParentDesc.Format );
    if ( bFullRange )
    {
        return View;
    }

    GLenum ViewTarget = GlTexture2D;
    switch ( InDesc.Type )
    {
    case RHI::ERHITextureViewType::Texture1D:
        ViewTarget = GlTexture1D;
        break;
    case RHI::ERHITextureViewType::Texture2D:
        ViewTarget = GlTexture2D;
        break;
    case RHI::ERHITextureViewType::Texture3D:
        ViewTarget = GlTexture3D;
        break;
    case RHI::ERHITextureViewType::TextureCube:
        ViewTarget = GlTextureCubeMap;
        break;
    case RHI::ERHITextureViewType::Texture2DArray:
        ViewTarget = GlTexture2DArray;
        break;
    default:
        break;
    }
    const TOptional<FOpenGLFormatMapping> Mapping =
        MapOpenGLFormat( InDesc.Format == RHI::ERHIFormat::Unknown ? ParentDesc.Format : InDesc.Format );
    if ( not Mapping )
    {
        return TUnexpected(
            RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Resource, "Texture view format has no GL mapping." ) );
    }
    const GLenum InternalFormat = Mapping->InternalFormat;

    const FGLDispatchTable &Dispatch = View->Context->GetDispatch();
    const GLuint SourceTexture       = ( *Parent )->GetTexture();
    GLuint NativeView                = 0U;
    GLenum NativeError               = GlNoError;
    const RHI::FRHIExpected Created  = View->Context->GetExecutor()->ExecuteOnGLThread(
        [&Dispatch, &NativeView, &NativeError, ViewTarget, SourceTexture, InternalFormat, &InDesc] () noexcept
    {
        Dispatch.CreateTextures( ViewTarget, 1, &NativeView );
        if ( NativeView == 0U )
        {
            return;
        }
        Dispatch.TextureView( NativeView, ViewTarget, SourceTexture, InternalFormat, InDesc.Range.BaseMip, InDesc.Range.MipCount,
                               InDesc.Range.BaseLayer, InDesc.Range.LayerCount );
        NativeError = Dispatch.GetError();
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( NativeError != GlNoError or NativeView == 0U )
    {
        if ( NativeView != 0U )
        {
            ( void )View->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeView] () noexcept
            {
                Dispatch.DeleteTextures( 1, &NativeView );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Resource, "OpenGL refused the texture view." ) );
    }
    View->View      = NativeView;
    View->bOwnsView = true;
    return View;
}

Lumen::RHIOpenGL::FOpenGLTextureView::FOpenGLTextureView ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                           const RHI::FRHITextureViewDesc &InDesc,
                                                           const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLTextureView>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ), Parent( InDesc.Texture )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLTextureView::~FOpenGLTextureView () noexcept
{
    if ( not bOwnsView )
    {
        return;
    }
    const GLuint NativeView = View;
    const RHI::FRHITimelineValue Required{ Context->GetExecutor()->GetCompletedTimeline() };
    const TSharedRef<FOpenGLContext> Executor = Context->GetExecutor();
    const FGLDispatchTable *Dispatch          = &Context->GetDispatch();
    ( void )Context->GetExecutor()->DeferRelease( Required, [Executor, Dispatch, NativeView] () noexcept
    {
        ( void )Executor->ExecuteOnGLThread( [Dispatch, NativeView] () noexcept
        {
            Dispatch->DeleteTextures( 1, &NativeView );
        } );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLTextureView );

const Lumen::RHI::FRHITextureViewDesc &Lumen::RHIOpenGL::FOpenGLTextureView::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::RHI::IRHITexture> Lumen::RHIOpenGL::FOpenGLTextureView::GetTexture () const noexcept
{
    return Parent;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLTextureView::GetView () const noexcept
{
    if ( View != 0U )
    {
        return View;
    }
    const TOptional<FOpenGLTexture *> Texture = RHI::TryRHIBackendCast<FOpenGLTexture>( *Parent );
    return Texture ? ( *Texture )->GetTexture() : 0U;
}

Lumen::RHIOpenGL::GLenum Lumen::RHIOpenGL::FOpenGLTextureView::GetTarget () const noexcept
{
    switch ( Desc.Type )
    {
    case RHI::ERHITextureViewType::Texture1D:
        return GlTexture1D;
    case RHI::ERHITextureViewType::Texture3D:
        return GlTexture3D;
    case RHI::ERHITextureViewType::TextureCube:
        return GlTextureCubeMap;
    case RHI::ERHITextureViewType::Texture2DArray:
        return GlTexture2DArray;
    default:
        return GlTexture2D;
    }
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLTextureView::GetDeviceContext () const noexcept
{
    return *Context;
}
