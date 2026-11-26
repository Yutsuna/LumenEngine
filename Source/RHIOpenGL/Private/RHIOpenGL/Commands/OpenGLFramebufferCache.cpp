/**
 * @file OpenGLFramebufferCache.cpp
 * @brief Implements GL-thread framebuffer creation, reuse and deletion.
 */

#include <RHIOpenGL/Commands/OpenGLFramebufferCache.hpp>

#include <Algo/Any.hpp>
#include <Algo/Find.hpp>
#include <Algo/ForEach.hpp>

#include <Memory/Span.hpp>

#include <RHI/Types/RHIError.hpp>

/**
 * File-local helpers
 */

namespace
{

constexpr Lumen::RHIOpenGL::GLenum GlDepthComponent16      = 0x81A5U;
constexpr Lumen::RHIOpenGL::GLenum GlDepthComponent24      = 0x81A6U;
constexpr Lumen::RHIOpenGL::GLenum GlDepthComponent32Float = 0x8CACU;
constexpr Lumen::RHIOpenGL::GLenum GlStencilIndex8         = 0x8D48U;

} // namespace

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLFramebufferCache::FOpenGLFramebufferCache ( TSharedRef<FOpenGLDeviceContext> InContext ) noexcept
    : Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLFramebufferCache::~FOpenGLFramebufferCache () noexcept
{
    Lumen::TVector<Lumen::RHIOpenGL::GLuint> Framebuffers;
    Algo::ForEach( Entries,
                   [] ( const FCacheEntry &InEntry ) noexcept -> Bool
    {
        return InEntry.Framebuffer != 0U;
    }, [&Framebuffers] ( const FCacheEntry &InEntry ) noexcept
    {
        Framebuffers.push_back( InEntry.Framebuffer );
    } );
    if ( Framebuffers.empty() )
    {
        return;
    }
    const Lumen::RHIOpenGL::FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, Framebuffers] () noexcept
    {
        Dispatch.DeleteFramebuffers( static_cast<Lumen::RHIOpenGL::GLsizei>( Framebuffers.size() ), Framebuffers.data() );
    } );
}

Lumen::TExpected<Lumen::RHIOpenGL::GLuint, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLFramebufferCache::GetOrCreate ( const FOpenGLRenderingPayload &InDesc ) noexcept
{
    if ( const FCacheEntry *Found = Algo::FindIf( Entries, [&InDesc] ( const FCacheEntry &InEntry ) noexcept -> Bool
    {
        return InEntry.Framebuffer != 0U and Matches( InEntry, InDesc );
    } ) )
    {
        return Found->Framebuffer;
    }

    const UInt32 ColorCount                            = GetColorCount( InDesc );
    const Lumen::RHIOpenGL::FGLDispatchTable &Dispatch = Context->GetDispatch();
    Lumen::RHIOpenGL::GLuint NativeFramebuffer         = 0U;
    Lumen::RHIOpenGL::GLenum NativeStatus              = GlNone;
    const RHI::FRHIExpected Created =
        Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &InDesc, ColorCount, &NativeFramebuffer, &NativeStatus] () noexcept
    {
        Dispatch.CreateFramebuffers( 1, &NativeFramebuffer );
        if ( NativeFramebuffer == 0U )
        {
            NativeStatus = GlNone;
            return;
        }

        Lumen::RHIOpenGL::GLenum DrawBuffers[MaxOpenGLCommandColorAttachments] = {};
        for ( UInt32 Index = 0U; Index < ColorCount and Index < MaxOpenGLCommandColorAttachments; ++Index )
        {
            const GLenum Attachment = static_cast<GLenum>( GlColorAttachment0 + Index );
            Dispatch.NamedFramebufferTexture( NativeFramebuffer, Attachment, InDesc.ColorViews[Index], 0 );
            DrawBuffers[Index] = Attachment;
        }
        Dispatch.NamedFramebufferDrawBuffers( NativeFramebuffer, static_cast<GLsizei>( ColorCount ), DrawBuffers );
        if ( InDesc.DepthView != 0U and InDesc.bHasDepthStencil )
        {
            Dispatch.NamedFramebufferTexture( NativeFramebuffer, SelectDepthAttachment( InDesc.DepthFormat ), InDesc.DepthView,
                                              0 );
        }
        NativeStatus = Dispatch.CheckNamedFramebufferStatus( NativeFramebuffer, GlDrawFramebuffer );
        if ( NativeStatus != GlFramebufferComplete )
        {
            Dispatch.DeleteFramebuffers( 1, &NativeFramebuffer );
            NativeFramebuffer = 0U;
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Created );
    if ( NativeFramebuffer == 0U )
    {
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Resource, "OpenGL framebuffer is incomplete." ) );
    }

    FCacheEntry Entry;
    Entry.Desc        = InDesc;
    Entry.Framebuffer = NativeFramebuffer;
    Entry.bSrgb       = false;
    const Lumen::TSpan<const Lumen::RHIOpenGL::GLenum> FormatSpan( InDesc.ColorFormats, ColorCount );
    Entry.bSrgb = Algo::AnyOf( FormatSpan, [] ( const Lumen::RHIOpenGL::GLenum InFormat ) noexcept -> Bool
    {
        return IsSrgbFormat( InFormat );
    } );
    Entries.push_back( Entry );
    return NativeFramebuffer;
}

void Lumen::RHIOpenGL::FOpenGLFramebufferCache::NotifyEndRendering ( const GLuint InFramebuffer, const Bool InbDiscard ) noexcept
{
    Lumen::RHIOpenGL::GLenum Attachments[MaxOpenGLCommandColorAttachments + 1U] = {};
    Lumen::UInt32 AttachmentCount                                               = 0U;
    if ( InbDiscard )
    {
        for ( const FCacheEntry &Entry : Entries )
        {
            if ( Entry.Framebuffer != InFramebuffer )
            {
                continue;
            }
            const UInt32 ColorCount = GetColorCount( Entry.Desc );
            for ( UInt32 Index = 0U; Index < ColorCount and AttachmentCount < MaxOpenGLCommandColorAttachments; ++Index )
            {
                Attachments[AttachmentCount++] = static_cast<GLenum>( GlColorAttachment0 + Index );
            }
            if ( Entry.Desc.DepthView != 0U and Entry.Desc.bHasDepthStencil and
                 AttachmentCount < MaxOpenGLCommandColorAttachments + 1U )
            {
                Attachments[AttachmentCount++] = SelectDepthAttachment( Entry.Desc.DepthFormat );
            }
            break;
        }
    }

    const Lumen::RHIOpenGL::FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, InFramebuffer, Attachments, AttachmentCount] () noexcept
    {
        if ( AttachmentCount > 0U )
        {
            Dispatch.InvalidateNamedFramebufferData( InFramebuffer, static_cast<GLsizei>( AttachmentCount ), Attachments );
        }
        Dispatch.BindFramebuffer( GlDrawFramebuffer, 0U );
    } );
}

/**
 * Private
 */

Lumen::UInt32 Lumen::RHIOpenGL::FOpenGLFramebufferCache::GetColorCount ( const FOpenGLRenderingPayload &InDesc ) noexcept
{
    const UInt32 Declared = InDesc.ColorAttachmentCount != 0U ? InDesc.ColorAttachmentCount : InDesc.Count;
    return Declared < MaxOpenGLCommandColorAttachments ? Declared : MaxOpenGLCommandColorAttachments;
}

Lumen::RHIOpenGL::GLenum Lumen::RHIOpenGL::FOpenGLFramebufferCache::SelectDepthAttachment ( const GLenum InFormat ) noexcept
{
    switch ( InFormat )
    {
    case GlStencilIndex8:
        return GlStencilAttachment;
    case GlDepthComponent16:
    case GlDepthComponent24:
    case GlDepthComponent32Float:
        return GlDepthAttachment;
    default:
        return GlDepthStencilAttachment;
    }
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLFramebufferCache::IsSrgbFormat ( const GLenum InFormat ) noexcept
{
    return InFormat == GlSrgb8Alpha8;
}

Lumen::Bool Lumen::RHIOpenGL::FOpenGLFramebufferCache::Matches ( const FCacheEntry &InEntry,
                                                                 const FOpenGLRenderingPayload &InDesc ) noexcept
{
    if ( GetColorCount( InEntry.Desc ) != GetColorCount( InDesc ) or InEntry.Desc.DepthView != InDesc.DepthView or
         InEntry.Desc.DepthFormat != InDesc.DepthFormat or InEntry.Desc.bHasDepthStencil != InDesc.bHasDepthStencil or
         InEntry.Desc.Area.Position.X != InDesc.Area.Position.X or InEntry.Desc.Area.Position.Y != InDesc.Area.Position.Y or
         InEntry.Desc.Area.Size.Width != InDesc.Area.Size.Width or InEntry.Desc.Area.Size.Height != InDesc.Area.Size.Height )
    {
        return false;
    }
    Bool bSrgb = false;
    for ( UInt32 Index = 0U; Index < MaxOpenGLCommandColorAttachments; ++Index )
    {
        if ( InEntry.Desc.ColorViews[Index] != InDesc.ColorViews[Index] or
             InEntry.Desc.ColorFormats[Index] != InDesc.ColorFormats[Index] )
        {
            return false;
        }
        bSrgb = bSrgb or IsSrgbFormat( InDesc.ColorFormats[Index] );
    }
    return InEntry.bSrgb == bSrgb;
}
